"""Validated native JSON storage and reversible, optimistic batch writes."""
from array import array
from copy import deepcopy
from hashlib import sha256
from itertools import groupby
from pathlib import Path
import base64
import contextlib
import fcntl
import json
import os
import re
import tempfile
import uuid

from .catalog import Catalog, Tile, LAYERS, TREES, LAMPS, WorldError, integer


def read_json(data):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise WorldError(f"Duplicate JSON key: {key}")
            result[key] = value
        return result
    def constant(value):
        raise WorldError(f"Invalid JSON number: {value}")
    try:
        return json.loads(data, object_pairs_hook=pairs, parse_constant=constant)
    except (ValueError, UnicodeError) as exc:
        raise WorldError(f"Invalid JSON: {exc}") from exc


def native(value):
    if isinstance(value, dict):
        return "{ " + ", ".join(json.dumps(k) + ": " + native(v) for k, v in value.items()) + " }"
    if isinstance(value, (tuple, list)):
        return "[" + ", ".join(native(v) for v in value) + "]"
    return json.dumps(value, ensure_ascii=False, allow_nan=False)


def encode_document(doc, map_document=False):
    """Match the native writer: compact records, no palette renumbering."""
    lines = ["{"]
    for number, (key, value) in enumerate(doc.items()):
        prefix = "  " + json.dumps(key) + ": "
        if key == "layers" and map_document:
            lines.append(prefix + "{")
            for i, layer in enumerate(LAYERS):
                lines.append("    " + json.dumps(layer) + ": " + native(value[layer]) + ("," if i < 2 else ""))
            lines.append("  }")
        elif key == "entities":
            lines.append(prefix + "{")
            for i, (group, entries) in enumerate(value.items()):
                lines.append("    " + json.dumps(group) + ": [")
                lines.extend("      " + native(e) + ("," if j + 1 < len(entries) else "") for j, e in enumerate(entries))
                lines.append("    ]" + ("," if i + 1 < len(value) else ""))
            lines.append("  }")
        elif isinstance(value, list):
            lines.append(prefix + "[")
            lines.extend("    " + native(e) + ("," if i + 1 < len(value) else "") for i, e in enumerate(value))
            lines.append("  ]")
        else:
            lines.append(prefix + native(value))
        if number + 1 < len(doc):
            lines[-1] += ","
    return ("\n".join(lines + ["}", ""])).encode("utf-8")


def map_id(value):
    if not isinstance(value, str) or not re.fullmatch(r"[A-Za-z0-9_-]+", value):
        raise WorldError(f"Unsafe map ID: {value!r}")
    return value


def rect(value):
    if not isinstance(value, (list, tuple)) or len(value) != 4:
        raise WorldError("A rectangle needs [x, y, width, height]")
    x, y, w, h = [integer(v, "rectangle coordinate") for v in value]
    if w < 1 or h < 1:
        raise WorldError("Rectangle dimensions must be positive")
    return x, y, w, h


def inside(bounds, x, y):
    a, b, w, h = bounds
    return a <= x < a + w and b <= y < b + h


def overlaps(a, b):
    x, y, w, h = a
    X, Y, W, H = b
    return x < X + W and x + w > X and y < Y + H and y + h > Y


class MapData:
    def __init__(self, doc, catalog):
        if not isinstance(doc, dict) or doc.get("format") != "kaijudo-catalog-map" or type(doc.get("version")) is not int or doc["version"] != 1:
            raise WorldError("Unsupported catalog map format/version")
        self.doc, self.catalog = deepcopy(doc), catalog
        self.id = map_id(doc.get("id"))
        if not isinstance(doc.get("name"), str) or not doc["name"].strip() or type(doc.get("indoor")) is not bool:
            raise WorldError(f"{self.id}: map needs a name and boolean indoor field")
        self.width = integer(doc.get("width"), "map width", 1, 1024)
        self.height = integer(doc.get("height"), "map height", 1, 1024)
        self.size = self.width * self.height
        values = doc.get("palette")
        if not isinstance(values, list) or not values or values[0] is not None:
            raise WorldError(f"{self.id}: palette[0] must be null")
        if any(not isinstance(t, dict) or not {"tileset", "sheet", "index", "layer", "tint"} <= set(t) for t in values[1:]):
            raise WorldError(f"{self.id}: native palette entries require tileset, sheet, index, layer and tint")
        self.palette = [None] + [catalog.parse(t, canonical=False) for t in values[1:]]
        self.lookup = {t: i for i, t in enumerate(self.palette)}
        if not isinstance(doc.get("layers"), dict) or set(doc["layers"]) != set(LAYERS):
            raise WorldError(f"{self.id}: exactly ground, decoration and foreground are required")
        self.grids = {}
        for layer in LAYERS:
            data = doc["layers"][layer]
            if not isinstance(data, dict) or data.get("encoding") != "rle-row-major" or not isinstance(data.get("data"), list):
                raise WorldError(f"{self.id}/{layer}: invalid RLE encoding")
            grid = array("I")
            for run in data["data"]:
                if not isinstance(run, list) or len(run) != 2:
                    raise WorldError(f"{self.id}/{layer}: runs need [palette_index, count]")
                index = integer(run[0], "palette index", 0, len(self.palette) - 1)
                count = integer(run[1], "run length", 1, self.size)
                if len(grid) + count > self.size:
                    raise WorldError(f"{self.id}/{layer}: RLE exceeds width * height")
                if index and self.palette[index].layer != layer:
                    raise WorldError(f"{self.id}/{layer}: palette {index} belongs to {self.palette[index].layer}")
                grid.extend(array("I", [index]) * count)
            if len(grid) != self.size:
                raise WorldError(f"{self.id}/{layer}: RLE totals {len(grid)}, expected {self.size}")
            self.grids[layer] = grid
        self.tags = {}
        if not isinstance(doc.get("tags", []), list):
            raise WorldError(f"{self.id}: tags must be an array")
        for tag in doc.get("tags", []):
            x, y = self.point([tag.get("x"), tag.get("y")])
            if not isinstance(tag.get("value"), str) or not tag["value"] or (x, y) in self.tags:
                raise WorldError(f"{self.id}: invalid or duplicate tag at {x},{y}")
            self.tags[x, y] = tag["value"]
        self._view = None
        self._dirty = False

    @classmethod
    def create(cls, id, name, width, height, indoor, catalog):
        size = integer(width, "width", 1, 1024) * integer(height, "height", 1, 1024)
        return cls(dict(format="kaijudo-catalog-map", version=1, id=id, name=name, indoor=indoor,
                        width=width, height=height, palette=[None],
                        layers={l: dict(encoding="rle-row-major", data=[[0, size]]) for l in LAYERS}), catalog)

    def clone(self):
        other = object.__new__(MapData)
        other.__dict__ = dict(self.__dict__)
        other.doc = deepcopy(self.doc)
        other.palette, other.lookup, other.tags = list(self.palette), dict(self.lookup), dict(self.tags)
        other.grids = {l: array("I", a) for l, a in self.grids.items()}
        other._view = None
        return other

    def contains(self, x, y):
        return 0 <= x < self.width and 0 <= y < self.height

    def point(self, value):
        if not isinstance(value, (tuple, list)) or len(value) != 2:
            raise WorldError("A point needs [x, y]")
        x, y = [integer(v, "point coordinate") for v in value]
        if not self.contains(x, y):
            raise WorldError(f"{self.id}: ({x},{y}) outside {self.width}x{self.height}")
        return x, y

    def bounds(self, value=None):
        if value is None:
            return 0, 0, self.width, self.height
        x, y, w, h = rect(value)
        self.point([x, y]); self.point([x + w - 1, y + h - 1])
        return x, y, w, h

    def get(self, layer, x, y):
        if not self.contains(x, y):
            return None
        return self.palette[self.grids[layer][y * self.width + x]]

    def set(self, layer, x, y, tile):
        self.point([x, y])
        if layer not in LAYERS or (tile is not None and tile.layer != layer):
            raise WorldError("Tile and target layer must agree")
        if self.get(layer, x, y) == tile:
            return
        if tile not in self.lookup:
            self.lookup[tile] = len(self.palette)
            self.palette.append(tile)
        self.grids[layer][y * self.width + x] = self.lookup[tile]
        self._view = None
        self._dirty = True

    def encoded(self):
        doc = deepcopy(self.doc)
        # Keep unknown metadata fields and old palette records byte-semantically intact.
        doc["palette"] = list(doc["palette"]) + [t.json() for t in self.palette[len(doc["palette"]):]]
        doc["layers"] = {l: dict(encoding="rle-row-major", data=[[v, sum(1 for _ in group)] for v, group in groupby(self.grids[l])]) for l in LAYERS}
        original_tags = {(t["x"], t["y"]): t["value"] for t in doc.get("tags", [])}
        if self.tags != original_tags:
            doc["tags"] = [dict(x=x, y=y, value=v) for (x, y), v in sorted(self.tags.items(), key=lambda p: (p[0][1], p[0][0]))]
        return encode_document(doc, True)

    def view(self):
        """Native load normalization on a query-only copy; never rewrite unrelated cells."""
        if self._view is not None:
            return self._view
        result = self.clone()
        # Collapse old three-cell streetlight stacks before canonicalizing aliases.
        for layer in LAYERS:
            candidates = {i for i, t in enumerate(self.palette) if t and t.tileset == "Outside" and t.sheet == "B" and t.index in (211, 235)}
            if not candidates:
                continue
            for p, index in enumerate(self.grids[layer]):
                if index not in candidates or p + 2 * self.width >= self.size:
                    continue
                a = self.palette[index]
                b = self.palette[self.grids[layer][p + self.width]]
                c = self.palette[self.grids[layer][p + 2 * self.width]]
                if b and c and b.index == a.index + 8 and c.index == a.index + 16 and (a.tileset, a.sheet, a.layer, a.tint) == (b.tileset, b.sheet, b.layer, b.tint) == (c.tileset, c.sheet, c.layer, c.tint):
                    result.grids[layer][p] = 0
                    result.grids[layer][p + self.width] = 0
        composite_cells = set()
        for layer in LAYERS:
            composite_indices = {i for i, t in enumerate(self.palette) if t and self.catalog.composite(t)}
            if not composite_indices:
                continue
            for p, index in enumerate(result.grids[layer]):
                if index in composite_indices:
                    composite_cells.add(p)
        for p in composite_cells:
            # Native iteration is (y,x,layer); later original layers win collisions.
            tiles = [result.palette[result.grids[l][p]] for l in LAYERS]
            for l in LAYERS: result.grids[l][p] = 0
            for tile in tiles:
                if tile:
                    tile = self.catalog.canonical(tile)
                    result.set(tile.layer, p % self.width, p // self.width, tile)
        accepted = set()
        big = {i for i, t in enumerate(result.palette) if t and self.catalog.composite(t) == (2, 2)}
        for p, i in enumerate(result.grids["decoration"]):
            if i in big:
                x, y = p % self.width, p // self.width
                if x < 1 or y < 1 or p - 1 in accepted or p + 1 in accepted:
                    result.grids["decoration"][p] = 0
                else:
                    accepted.add(p)
        result._view = result
        self._view = result
        return result

    def collision(self, x, y):
        self.point([x, y])
        view = self.view()
        for layer in reversed(LAYERS):
            tile = view.get(layer, x, y)
            if tile:
                behavior = self.catalog.collision(tile)
                if behavior != "inherits":
                    return behavior, tile
        return "blocked", None


class World:
    def __init__(self, directory, catalog):
        self.root, self.catalog = Path(directory).resolve(), catalog
        self.before = {}
        self.doc = read_json(self.read("World.json"))
        if not isinstance(self.doc, dict) or self.doc.get("format") != "kaijudo-world" or type(self.doc.get("version")) is not int or self.doc["version"] != 1:
            raise WorldError("Unsupported world manifest format/version")
        for key in ("maps", "regions", "portals"):
            if not isinstance(self.doc.get(key), list):
                raise WorldError(f"Manifest needs an array: {key}")
        if not isinstance(self.doc.get("entities"), dict) or not isinstance(self.doc.get("start"), dict):
            raise WorldError("Manifest needs entities and player start")
        if not self.doc["maps"]:
            raise WorldError("Manifest needs at least one map")
        for group in ("npcs", "objects", "shards", "object_definitions", "object_appearances"):
            rows = self.doc["entities"].get(group, [])
            if not isinstance(rows, list) or any(not isinstance(row, dict) or not isinstance(row.get("id"), str) or not row["id"] for row in rows):
                raise WorldError(f"entities.{group} must contain objects with non-empty string IDs")
        for key in ("regions", "portals"):
            if any(not isinstance(row, dict) for row in self.doc[key]):
                raise WorldError(f"{key} must contain objects")
        self.maps, self.paths = {}, {}
        for path in self.doc["maps"]:
            m = MapData(read_json(self.read(path)), catalog)
            if m.id in self.maps:
                raise WorldError(f"Duplicate map ID: {m.id}")
            self.maps[m.id], self.paths[m.id] = m, path
        self.initial_doc = deepcopy(self.doc)
        self.revision = self.snapshot_revision(self.before)

    def path(self, name):
        if not isinstance(name, str) or "\\" in name or Path(name).is_absolute() or ".." in Path(name).parts:
            raise WorldError(f"Unsafe world path: {name!r}")
        path = (self.root / name).resolve()
        if not path.is_relative_to(self.root):
            raise WorldError(f"World path escapes its directory: {name}")
        return path

    def read(self, name):
        data = self.path(name).read_bytes()
        self.before[name] = data
        return data

    def map(self, id):
        if id not in self.maps:
            raise WorldError(f"Unknown map: {id}; available IDs: {', '.join(self.maps)}")
        return self.maps[id]

    def region(self, id):
        for r in self.doc["regions"]:
            if r.get("id") == id:
                return r
        raise WorldError(f"Unknown region: {id}; available IDs: {', '.join(r['id'] for r in self.doc['regions'])}")

    def selection(self, id=None, bounds=None, region=None):
        if region:
            if bounds is not None:
                raise WorldError("Use either --region or --rect")
            r = self.region(region)
            if id and id != r["map"]:
                raise WorldError("Region and map disagree")
            id = r["map"]
            bounds = [r[k] for k in ("x", "y", "width", "height")]
        m = self.map(id or "overworld")
        return m, m.bounds(bounds)

    @staticmethod
    def snapshot_revision(files):
        digest = sha256()
        for path, data in sorted(files.items()):
            digest.update(path.encode() + b"\0" + sha256(data).digest())
        return digest.hexdigest()

    def changed_files(self):
        result = {}
        if self.doc != self.initial_doc:
            result["World.json"] = encode_document(self.doc)
        for id, m in self.maps.items():
            path = self.paths[id]
            original = read_json(self.before[path]) if path in self.before else None
            if original is not None and not m._dirty and m.doc == original and m.tags == {(t["x"],t["y"]):t["value"] for t in original.get("tags",[])}:
                continue
            # Compare decoded semantics so query normalization and formatting do not cause edits.
            candidate = m.encoded()
            if original is None or read_json(candidate) != original:
                if not Path(path).is_relative_to("Maps") or self.path(path).parent != self.root / "Maps":
                    raise WorldError(f"Edits may write only World/Maps/<id>.json, not {path}")
                result[path] = candidate
        return result


def atomic_write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix="." + path.name + ".", dir=path.parent)
    try:
        if path.exists():
            os.fchmod(fd, path.stat().st_mode & 0o777)
        with os.fdopen(fd, "wb") as out:
            out.write(data); out.flush(); os.fsync(out.fileno())
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def external_output(world, path):
    path = Path(path).resolve()
    if path == world.root or path.is_relative_to(world.root):
        raise WorldError("Reports, previews and receipts must be outside the native World directory")
    return path


@contextlib.contextmanager
def world_lock(world):
    directory = Path(tempfile.gettempdir()) / "kaijudo-world-builder" / sha256(str(world.root).encode()).hexdigest()[:20]
    directory.mkdir(parents=True, exist_ok=True)
    with (directory / "lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield directory


def commit(world, files, expected=None, receipt_path=None):
    if expected is not None and expected != world.revision:
        raise WorldError(f"Stale revision: expected {expected}, current {world.revision}")
    if not files:
        return dict(written=[], revision=world.revision, receipt=None)
    with world_lock(world) as history:
        for name, original in world.before.items():
            if not world.path(name).exists() or world.path(name).read_bytes() != original:
                raise WorldError(f"World changed while preparing edits: {name}; inspect and retry")
        for name in files:
            if name not in world.before and world.path(name).exists():
                raise WorldError(f"Refusing to replace an unregistered existing map: {name}")
        receipt_path = external_output(world, receipt_path) if receipt_path else history / (uuid.uuid4().hex + ".json")
        if receipt_path.exists():
            raise WorldError(f"Receipt already exists: {receipt_path}")
        after = dict(world.before); after.update(files)
        receipt = dict(version=1, world=str(world.root), before_revision=world.revision,
                       after_revision=world.snapshot_revision(after), status="prepared", files={
                           name: dict(before=base64.b64encode(world.before[name]).decode() if name in world.before else None,
                                      after_hash=sha256(data).hexdigest()) for name, data in files.items()})
        atomic_write(receipt_path, json.dumps(receipt, indent=2).encode())
        written = []
        try:
            # Add maps before the manifest can reference them.
            for name in sorted(files, key=lambda p: p == "World.json"):
                atomic_write(world.path(name), files[name]); written.append(name)
            receipt["status"] = "applied"
            atomic_write(receipt_path, json.dumps(receipt, indent=2).encode())
        except BaseException:
            for name in reversed(written):
                if name in world.before:
                    atomic_write(world.path(name), world.before[name])
                else:
                    world.path(name).unlink()
            raise
        return dict(written=written, revision=receipt["after_revision"], receipt=str(receipt_path))


def undo(world, receipt_path):
    receipt_path = external_output(world, receipt_path)
    receipt = read_json(receipt_path.read_bytes())
    if receipt.get("version") != 1 or receipt.get("world") != str(world.root) or receipt.get("status") not in ("applied", "prepared"):
        raise WorldError("Receipt is not an applicable edit for this world")
    with world_lock(world):
        if world.revision != receipt.get("after_revision"):
            raise WorldError("Cannot undo: the world has changed since this edit")
        for name, data in world.before.items():
            if world.path(name).read_bytes() != data:
                raise WorldError(f"World changed while preparing undo: {name}")
        restored_bytes = {}
        for name, info in receipt["files"].items():
            if name != "World.json" and not re.fullmatch(r"Maps/[A-Za-z0-9_-]+\.json", name):
                raise WorldError(f"Unsafe receipt path: {name}")
            if sha256(world.path(name).read_bytes()).hexdigest() != info["after_hash"]:
                raise WorldError(f"Cannot undo changed file: {name}")
            restored_bytes[name] = base64.b64decode(info["before"], validate=True) if info["before"] is not None else None
        previous = dict(world.before)
        for name, data in restored_bytes.items():
            if data is None: previous.pop(name, None)
            else: previous[name] = data
        if world.snapshot_revision(previous) != receipt["before_revision"]:
            raise WorldError("Receipt before-data is corrupt")
        # Restore the manifest before removing newly-created maps.
        restored = []
        try:
            for name in sorted(receipt["files"], key=lambda p: p != "World.json"):
                old = restored_bytes[name]
                if old is None:
                    world.path(name).unlink()
                else:
                    atomic_write(world.path(name), old)
                restored.append(name)
            receipt["status"] = "undone"
            atomic_write(receipt_path, json.dumps(receipt, indent=2).encode())
        except BaseException:
            for name in reversed(restored):
                atomic_write(world.path(name), world.before[name])
            raise
        return dict(restored=restored, revision=receipt["before_revision"])
