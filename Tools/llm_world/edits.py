"""Bounded, declarative edits. Nothing touches disk until every operation validates."""
from copy import deepcopy

from .catalog import LAYERS, WorldError, integer
from .storage import MapData, inside, map_id, rect


def fields(value, allowed, required=()):
    if not isinstance(value, dict):
        raise WorldError("Expected an object")
    extra, missing = set(value) - set(allowed), set(required) - set(value)
    if extra or missing:
        raise WorldError(f"Unknown fields: {sorted(extra)}; missing fields: {sorted(missing)}")


def line(points):
    """Inclusive grid line, with orthogonal steps even along diagonal segments."""
    if not points:
        raise WorldError("A path needs at least one point")
    yield points[0]
    for (x, y), (X, Y) in zip(points, points[1:]):
        dx, dy = abs(X - x), abs(Y - y)
        sx, sy = (1 if X > x else -1), (1 if Y > y else -1)
        ix = iy = 0
        while ix < dx or iy < dy:
            if ix < dx and (iy == dy or (ix + .5) * dy <= (iy + .5) * dx):
                x += sx; ix += 1
            else:
                y += sy; iy += 1
            yield x, y


def prefab_list():
    return [
        dict(id="building", fields=["rect", "roof", "wall", "roof_height"],
             description="Roof above facade; tile references choose materials. Cut the doorway separately."),
        dict(id="room", fields=["rect", "floor", "wall", "wall_height"],
             description="Floor with blocked perimeter and upper wall; cut exits separately."),
        dict(id="vertical", fields=["at", "tiles"],
             description="Top-to-bottom pieces, with each tile's own render layer; useful for beds and shelves."),
        dict(id="table_setting", fields=["at", "base", "top"],
             description="Blocked table in decoration, surface prop in foreground on the same cell."),
        dict(id="custom", fields=["at", "cells"],
             description="Reusable relative [dx,dy,tile] cells, with explicit layer/tint in tile objects."),
    ]


class Patch:
    def __init__(self, world, metadata, document):
        fields(document, ("version", "expected_revision", "scope", "protect", "frames", "anchors", "operations", "require_routes"), ("version", "scope", "operations"))
        if type(document["version"]) is not int or document["version"] != 1:
            raise WorldError("Patch version must be 1")
        if not isinstance(document["operations"], list) or not isinstance(document["scope"], list) or not document["scope"]:
            raise WorldError("Patch needs an operations array and a non-empty scope array")
        self.world, self.metadata, self.doc = world, metadata, document
        self.scopes, self.protected = [], []
        for entries, target in ((document["scope"], self.scopes), (document.get("protect", []), self.protected)):
            for entry in entries:
                fields(entry, ("map", "rect", "layers"), ("map", "rect"))
                layers = entry.get("layers", list(LAYERS))
                if not isinstance(layers, list) or not layers or any(l not in LAYERS for l in layers):
                    raise WorldError("scope/protect layers must name ground, decoration or foreground")
                target.append(dict(map=map_id(entry["map"]), rect=rect(entry["rect"]), layers=layers))
        self.frames = document.get("frames", {})
        self.anchors = document.get("anchors", {})
        for frame in self.frames.values():
            fields(frame, ("map", "origin"), ("map", "origin"))
            map_id(frame["map"]); self.xy(frame["origin"])
        self.routes = deepcopy(document.get("require_routes", []))
        if not isinstance(self.routes, list):
            raise WorldError("require_routes must be an array")
        self.touched = set()

    @staticmethod
    def xy(value):
        if not isinstance(value, (list, tuple)) or len(value) != 2:
            raise WorldError("A coordinate needs [x,y]")
        return tuple(integer(v, "coordinate") for v in value)

    def context(self, op):
        frame = self.frames.get(op.get("frame"))
        if "frame" in op and frame is None:
            raise WorldError(f"Unknown frame: {op['frame']}")
        id = op.get("map", frame["map"] if frame else None)
        if frame and id != frame["map"]:
            raise WorldError("Operation map and frame disagree")
        return self.world.map(id), self.xy(frame["origin"]) if frame else (0, 0)

    def point(self, value, m, origin=(0, 0)):
        if isinstance(value, str):
            if value not in self.anchors:
                raise WorldError(f"Unknown anchor: {value}")
            entry = self.anchors[value]
            fields(entry, ("map", "frame", "at"), ("at",))
            am, offset = self.context(entry)
            if am.id != m.id:
                raise WorldError(f"Anchor {value} belongs to {am.id}, not {m.id}")
            if isinstance(entry["at"], str):
                raise WorldError("Anchors must contain coordinates, not other anchors")
            return self.point(entry["at"], m, offset)
        x, y = self.xy(value)
        return m.point([x + origin[0], y + origin[1]])

    def rectangle(self, value, m, origin):
        x, y, w, h = rect(value)
        return m.bounds([x + origin[0], y + origin[1], w, h])

    def allow(self, m, x, y, layer=None):
        m.point([x, y])
        layers = (layer,) if layer else LAYERS
        for l in layers:
            if not any(s["map"] == m.id and l in s["layers"] and inside(s["rect"], x, y) for s in self.scopes):
                raise WorldError(f"{m.id} ({x},{y})/{l}: outside declared edit scope")
            if any(s["map"] == m.id and l in s["layers"] and inside(s["rect"], x, y) for s in self.protected):
                raise WorldError(f"{m.id} ({x},{y})/{l}: inside a protected rectangle")

    def put(self, m, x, y, tile, layer=None):
        layer = layer or tile.layer
        self.allow(m, x, y, layer)
        if tile and self.world.catalog.composite(tile):
            w, h = self.world.catalog.composite(tile)
            if x < w - 1 or y < h - 1:
                raise WorldError(f"{tile.key} at {x},{y} would extend outside the map")
        m.set(layer, x, y, tile)
        self.touched.add((m.id, layer, x, y))

    def fill(self, m, bounds, tile, layer=None):
        x, y, w, h = bounds
        for Y in range(y, y + h):
            for X in range(x, x + w):
                self.put(m, X, Y, tile, layer)

    def endpoint(self, value):
        fields(value, ("map", "frame", "at"), ("at",))
        m, origin = self.context(value)
        x, y = self.point(value["at"], m, origin)
        self.allow(m, x, y)
        return dict(map=m.id, x=x, y=y)

    def old_position(self, p):
        self.allow(self.world.map(p["map"]), p["x"], p["y"])

    def execute(self):
        expected = self.doc.get("expected_revision")
        if expected is not None and expected != self.world.revision:
            raise WorldError(f"Stale revision: expected {expected}, current {self.world.revision}")
        for i, op in enumerate(self.doc["operations"]):
            try:
                self.operation(op)
            except (WorldError, KeyError, TypeError, AttributeError) as exc:
                raise WorldError(f"Operation {i} ({op.get('op', '?') if isinstance(op, dict) else '?'}): {exc}") from exc
        for scope in self.scopes + self.protected:
            self.world.map(scope["map"]).bounds(scope["rect"])
        # Native loading silently removes crowded large trees. Reject that data loss.
        for id, layer, x, y in self.touched:
            m = self.world.map(id)
            t = m.get(layer, x, y)
            if t and self.world.catalog.composite(t) == (2, 2):
                for X in (x - 1, x + 1):
                    other = m.view().get("decoration", X, y)
                    raw = m.get("decoration", X, y)
                    if any(v and self.world.catalog.composite(v) == (2, 2) for v in (other, raw)):
                        raise WorldError(f"{id}: large tree anchors at ({x},{y}) and ({X},{y}) would be culled on native loading")
        for id, layer, x, y in self.touched:
            m = self.world.map(id)
            if m.get(layer, x, y) != m.view().get(layer, x, y):
                raise WorldError(f"{id} ({x},{y})/{layer}: native normalization would replace this edit; add a normalize operation before editing legacy composite layers")
        routes = []
        for req in self.routes:
            fields(req, ("map", "frame", "from", "to", "rect", "interact"), ("from", "to"))
            m, origin = self.context(req)
            resolved = dict(map=m.id, **{k: list(self.point(req[k], m, origin)) for k in ("from", "to")}, interact=req.get("interact", False))
            if type(resolved["interact"]) is not bool:
                raise WorldError("Route interact must be boolean")
            if "rect" in req:
                resolved["rect"] = self.rectangle(req["rect"], m, origin)
            routes.append(resolved)
        return routes

    def operation(self, op):
        name = op["op"]
        common = ("op", "map", "frame")
        cat, world = self.world.catalog, self.world
        if name == "new_map":
            fields(op, ("op", "id", "name", "width", "height", "indoor"), ("id", "name", "width", "height", "indoor"))
            id = map_id(op["id"])
            if id in world.maps:
                raise WorldError(f"Map already exists: {id}")
            m = MapData.create(id, op["name"], op["width"], op["height"], op["indoor"], cat)
            # Creating a map claims its complete dimensions, even if initially empty.
            for y in range(m.height):
                for x in range(m.width):
                    self.allow(m, x, y)
            path = f"Maps/{id}.json"
            if world.path(path).exists():
                raise WorldError(f"Unregistered file already exists: {path}")
            world.maps[id], world.paths[id] = m, path
            world.doc["maps"].append(path)
            return
        if name in ("entity_move", "object_add", "start_move"):
            fields(op, ("op", "group", "id", "template", "position", "appearance"), ("position",))
            p = self.endpoint(op["position"])
            entities = world.doc["entities"]
            if name == "start_move":
                self.old_position(world.doc["start"]); world.doc["start"] = p
                return
            id = op["id"]
            if not isinstance(id, str) or not id:
                raise WorldError("Entity ID must be a non-empty string")
            group = "objects" if name == "object_add" else op["group"]
            if group not in ("npcs", "objects", "shards"):
                raise WorldError("Entity group must be npcs, objects or shards")
            rows = entities.setdefault(group, [])
            existing = next((e for e in rows if e["id"] == id), None)
            if name == "object_add":
                if id in self.metadata.objects(world) or existing or op.get("template") not in self.metadata.groups["templates"]:
                    raise WorldError("Object needs a unique ID and a real Lua template")
                entities.setdefault("object_definitions", []).append(dict(id=id, template=op["template"]))
            elif not existing:
                known = self.metadata.objects(world) if group == "objects" else self.metadata.groups[group]
                if id not in known:
                    raise WorldError(f"No Lua metadata exists for {group}/{id}")
            if existing:
                self.old_position(existing); existing.update(p)
            else:
                rows.append(dict(id=id, **p))
            if "appearance" in op:
                if group != "objects":
                    raise WorldError("Only objects support native appearance overrides")
                overrides = entities.setdefault("object_appearances", [])
                overrides[:] = [v for v in overrides if v["id"] != id]
                if op["appearance"] is not None:
                    overrides.append(dict(id=id, appearance=op["appearance"]))
            return
        if name in ("portal_add", "entrance"):
            if name == "portal_add":
                fields(op, ("op", "from", "to", "appearance"), ("from", "to"))
                ends = [self.endpoint(op[k]) for k in ("from", "to")]
                portal = dict(zip(("from", "to"), ends))
                if op.get("appearance"):
                    portal["appearance"] = op["appearance"]
                world.doc["portals"].append(portal)
            else:
                fields(op, ("op", "outside_door", "inside_arrival", "inside_exit", "outside_return", "appearance", "exit_appearance"),
                       ("outside_door", "inside_arrival", "inside_exit", "outside_return", "appearance"))
                a, b, c, d = [self.endpoint(op[k]) for k in ("outside_door", "inside_arrival", "inside_exit", "outside_return")]
                if a["map"] != d["map"] or b["map"] != c["map"] or a["map"] == b["map"]:
                    raise WorldError("An entrance must pair an exterior and a separate interior map")
                if not op["appearance"]:
                    raise WorldError("An entrance needs a visible outside appearance")
                world.doc["portals"].append(dict(appearance=op["appearance"], **{"from": a, "to": b}))
                back = {"from": c, "to": d}
                if op.get("exit_appearance"):
                    back["appearance"] = op["exit_appearance"]
                world.doc["portals"].append(back)
                for start, goal, interact in ((d, a, True), (b, c, bool(op.get("exit_appearance")))):
                    self.routes.append(dict(map=start["map"], **{"from": [start["x"], start["y"]], "to": [goal["x"], goal["y"]]}, interact=interact))
            return
        if name == "portal_remove":
            fields(op, ("op", "index"), ("index",))
            i = integer(op["index"], "portal index", 0, len(world.doc["portals"]) - 1)
            for p in world.doc["portals"][i].values():
                if isinstance(p, dict): self.old_position(p)
            del world.doc["portals"][i]
            return
        m, origin = self.context(op)
        if name == "normalize":
            fields(op, common)
            view = m.view()
            for layer in LAYERS:
                for p, index in enumerate(view.grids[layer]):
                    tile = view.palette[index]
                    x, y = p % m.width, p // m.width
                    if m.get(layer, x, y) != tile:
                        self.put(m, x, y, tile, layer)
        elif name in ("fill", "erase"):
            fields(op, common + ("rect", "tile", "layer"), ("rect", "tile") if name == "fill" else ("rect", "layer"))
            tile = cat.parse(op["tile"], layer=op.get("layer")) if name == "fill" else None
            self.fill(m, self.rectangle(op["rect"], m, origin), tile, tile.layer if tile else op["layer"])
        elif name in ("path", "road_network"):
            fields(op, common + ("points", "paths", "tile", "width", "shoulder", "shoulder_width", "clip"), ("tile",))
            paths = op["paths"] if name == "road_network" else [op["points"]]
            if type(op.get("clip", False)) is not bool:
                raise WorldError("Path clip must be boolean")
            width = integer(op.get("width", 1), "path width", 1, 101)
            shoulder_width = integer(op.get("shoulder_width", 1), "shoulder width", 0, 50)
            lines = [list(line([self.point(p, m, origin) for p in points])) for points in paths]
            passes = [(cat.parse(op["shoulder"]), width + 2 * shoulder_width)] if op.get("shoulder") else []
            passes.append((cat.parse(op["tile"]), width))
            # All shoulders first, then all surfaces: intersections retain their paving.
            for tile, size in passes:
                lo, hi = -(size // 2), size - size // 2
                for points in lines:
                    for x, y in points:
                        for dy in range(lo, hi):
                            for dx in range(lo, hi):
                                if op.get("clip", False) and not m.contains(x + dx, y + dy): continue
                                self.put(m, x + dx, y + dy, tile)
        elif name == "stamp":
            kind = op.get("prefab", "custom")
            fields(op, common + ("prefab", "rect", "at", "roof", "wall", "floor", "roof_height", "wall_height", "tiles", "cells", "base", "top"))
            if kind in ("building", "room"):
                x, y, w, h = self.rectangle(op["rect"], m, origin)
                if w < 3 or h < 3: raise WorldError("Building/room needs at least 3x3 cells")
                if kind == "building":
                    rh = integer(op.get("roof_height", h - 2), "roof height", 1, h - 1)
                    self.fill(m, (x, y, w, rh), cat.parse(op["roof"]))
                    self.fill(m, (x, y + rh, w, h - rh), cat.parse(op["wall"]))
                else:
                    wh = integer(op.get("wall_height", 2), "wall height", 1, h - 2)
                    self.fill(m, (x, y, w, h), cat.parse(op["floor"]))
                    wall = cat.parse(op["wall"])
                    for r in ((x, y, w, wh), (x, y + h - 1, w, 1), (x, y, 1, h), (x + w - 1, y, 1, h)):
                        self.fill(m, r, wall)
            else:
                x, y = self.point(op["at"], m, origin)
                if kind == "vertical":
                    cells = [[0, i, t] for i, t in enumerate(op["tiles"])]
                elif kind == "table_setting":
                    cells = [[0, 0, cat.parse(op["base"], layer="decoration").json()], [0, 0, cat.parse(op["top"], layer="foreground").json()]]
                elif kind == "custom":
                    cells = op["cells"]
                else:
                    raise WorldError(f"Unknown prefab: {kind}")
                for dx, dy, tile in cells:
                    self.put(m, x + integer(dx, "stamp dx"), y + integer(dy, "stamp dy"), cat.parse(tile))
        elif name in ("copy", "move"):
            fields(op, common + ("source", "at", "layers"), ("source", "at"))
            source = op["source"]
            fields(source, ("map", "rect"), ("map", "rect"))
            src = world.map(source["map"])
            sx, sy, w, h = src.bounds(source["rect"])
            x, y = self.point(op["at"], m, origin)
            layers = op.get("layers", list(LAYERS))
            if not isinstance(layers, list) or any(l not in LAYERS for l in layers):
                raise WorldError("Invalid copy layers")
            source_view = src.view()
            saved = [(l, dx, dy, source_view.get(l, sx + dx, sy + dy)) for l in layers for dy in range(h) for dx in range(w)]
            if name == "move":
                for l in layers: self.fill(src, (sx, sy, w, h), None, l)
            for l, dx, dy, t in saved:
                self.put(m, x + dx, y + dy, cat.canonical(t) if t else None, cat.canonical(t).layer if t else l)
        elif name == "tag_set":
            fields(op, common + ("at", "value"), ("at", "value"))
            x, y = self.point(op["at"], m, origin); self.allow(m, x, y)
            if op["value"] is None: m.tags.pop((x, y), None)
            elif isinstance(op["value"], str) and op["value"]: m.tags[x, y] = op["value"]
            else: raise WorldError("A tag is a non-empty string, or null to remove")
        elif name == "region_set":
            fields(op, common + ("id", "name", "rect", "kind", "weather"), ("id", "name", "rect", "kind"))
            if m.doc["indoor"]: raise WorldError("Regions require an exterior map")
            bounds = self.rectangle(op["rect"], m, origin)
            old = next((r for r in world.doc["regions"] if r["id"] == op["id"]), None)
            for target, r in ((m, bounds), *(([(world.map(old["map"]), [old[k] for k in ("x", "y", "width", "height")])] if old else []))):
                x, y, w, h = r
                for Y in range(y, y + h):
                    for X in range(x, x + w): self.allow(target, X, Y)
            new = dict(old or {}, id=op["id"], name=op["name"], map=m.id, kind=op["kind"], **dict(zip(("x", "y", "width", "height"), bounds)))
            if "weather" in op: new["weather"] = op["weather"]
            if old: old.update(new)
            else: world.doc["regions"].append(new)
        else:
            raise WorldError(f"Unknown operation: {name}")
