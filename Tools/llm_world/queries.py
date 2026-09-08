"""Spatial summaries, navigation and actionable native-world validation."""
from array import array
from collections import Counter, deque
import heapq
import re

from .catalog import LAYERS, WorldError, integer
from .storage import inside, overlaps, rect

OFFSETS = ((0, -1), (1, 0), (0, 1), (-1, 0))


def positions(world, map_id):
    rows = []
    for group in ("npcs", "objects", "shards"):
        rows.extend(dict(p, category=group) for p in world.doc["entities"].get(group, []) if p.get("map") == map_id)
    start = world.doc["start"]
    if start.get("map") == map_id:
        rows.append(dict(start, category="start", id="player_start"))
    for i, portal in enumerate(world.doc["portals"]):
        for end in ("from", "to"):
            p = portal[end]
            if p.get("map") == map_id:
                rows.append(dict(p, category="portal_" + end, id=f"portal:{i}:{end}", appearance=portal.get("appearance")))
    return rows


def cell(world, m, x, y, cleared=()):
    m.point([x, y])
    behavior, source = m.collision(x, y)
    occupants = [p for p in positions(world, m.id) if (p["x"], p["y"]) == (x, y)]
    blockers = [p["id"] for p in occupants if (p["category"] in ("npcs", "objects") and p["id"] not in cleared) or (p["category"] == "portal_from" and p["appearance"])]
    tag = m.tags.get((x, y))
    return dict(map=m.id, at=[x, y], layers={l: t.json() if (t := m.view().get(l, x, y)) else None for l in LAYERS},
                tile_collision=behavior, collision_source=source.key if source else "empty/inherited layers",
                walkable=behavior == "walkable" and not blockers,
                blockers=blockers, tag=tag, occupants=occupants)


class Navigation:
    def __init__(self, world, m, bounds=None, cleared=()):
        self.m, self.bounds = m, m.bounds(bounds)
        self.triggers, self.blockers = set(), {}
        self.walkable = bytearray(m.size)
        view = m.view()
        behaviors = ["inherits" if t is None else m.catalog.collision(t) for t in view.palette]
        x, y, w, h = self.bounds
        for Y in range(y, y + h):
            for X in range(x, x + w):
                p = Y * m.width + X
                for layer in reversed(LAYERS):
                    behavior = behaviors[view.grids[layer][p]]
                    if behavior != "inherits":
                        self.walkable[p] = behavior == "walkable"
                        break
        for item in positions(world, m.id):
            p = item["y"] * m.width + item["x"]
            if item["category"] in ("npcs", "objects") and item["id"] not in cleared or item["category"] == "portal_from" and item.get("appearance"):
                self.blockers.setdefault(p, []).append(item["id"])
                if 0 <= p < m.size:
                    self.walkable[p] = 0
            elif item["category"] == "portal_from":
                self.triggers.add(p)

    def index(self, xy):
        x, y = self.m.point(xy)
        if not inside(self.bounds, x, y):
            raise WorldError(f"Point {x},{y} is outside the navigation bounds")
        return y * self.m.width + x

    def neighbors(self, p):
        x, y = p % self.m.width, p // self.m.width
        for dx, dy in OFFSETS:
            X, Y = x + dx, y + dy
            if self.m.contains(X, Y):
                q = Y * self.m.width + X
                if self.walkable[q]:
                    yield q

    def reachable(self, start):
        p = self.index(start)
        if not self.walkable[p]:
            raise WorldError(f"Navigation start {list(start)} is blocked")
        seen = bytearray(self.m.size); seen[p] = 1
        queue = deque([p])
        while queue:
            p = queue.popleft()
            # Stepping onto an invisible portal leaves this map; don't walk through it.
            if p in self.triggers:
                continue
            for q in self.neighbors(p):
                if not seen[q]:
                    seen[q] = 1; queue.append(q)
        return seen

    def path(self, start, goal, interact=False):
        source, target = self.index(start), self.index(goal)
        if not self.walkable[source]:
            raise WorldError(f"Navigation start {list(start)} is blocked")
        goals = set(self.neighbors(target)) if interact else {target}
        goals = {p for p in goals if self.walkable[p]}
        if not goals:
            return dict(found=False, reason="Target has no walkable interaction cell" if interact else "Target is blocked", path=[])
        gx, gy = goal
        def heuristic(p):
            return max(0, abs(p % self.m.width - gx) + abs(p // self.m.width - gy) - int(interact))
        frontier = [(heuristic(source), 0, source)]
        previous, costs = {source: None}, {source: 0}
        while frontier:
            _, cost, p = heapq.heappop(frontier)
            if cost != costs[p]:
                continue
            if p in goals:
                route = []
                while p is not None:
                    route.append([p % self.m.width, p // self.m.width]); p = previous[p]
                route.reverse()
                return dict(found=True, steps=len(route) - 1, path=route, interaction_target=list(goal) if interact else None)
            if p in self.triggers:
                continue
            for q in self.neighbors(p):
                if cost + 1 < costs.get(q, self.m.size + 1):
                    costs[q], previous[q] = cost + 1, p
                    heapq.heappush(frontier, (cost + 1 + heuristic(q), cost + 1, q))
        return dict(found=False, reason="No route within the selected map and bounds", path=[])


def summary(world, m=None, bounds=None):
    if m is None:
        return dict(revision=world.revision,
                    maps=[dict(id=v.id, name=v.doc["name"], width=v.width, height=v.height, indoor=v.doc["indoor"]) for v in world.maps.values()],
                    regions=world.doc["regions"], portals=len(world.doc["portals"]),
                    entities={k: len(world.doc["entities"].get(k, [])) for k in ("npcs", "objects", "shards")})
    x, y, w, h = m.bounds(bounds)
    counts = {l: Counter() for l in LAYERS}
    painted = 0
    for Y in range(y, y + h):
        for X in range(x, x + w):
            p = Y * m.width + X
            painted += any(m.grids[l][p] for l in LAYERS)
            for l in LAYERS:
                if (t := m.get(l, X, Y)):
                    counts[l][t.key] += 1
    return dict(revision=world.revision, map=m.id, rect=[x, y, w, h], painted_cells=painted,
                empty_cells=w * h - painted,
                tiles={l: [{"key": k, "count": n} for k, n in counts[l].most_common(12)] for l in LAYERS},
                regions=[r for r in world.doc["regions"] if r["map"] == m.id and overlaps([x,y,w,h], [r[k] for k in ("x","y","width","height")])],
                entities=[p for p in positions(world, m.id) if inside([x,y,w,h], p["x"], p["y"])])


def inspect(world, m, bounds):
    x, y, w, h = m.bounds(bounds)
    if w * h > 4096:
        raise WorldError("ASCII inspection is limited to 4096 cells; request a smaller --rect or use summary/render")
    nav = Navigation(world, m, bounds)
    markers = {(p["x"], p["y"]): {"npcs":"N", "objects":"O", "shards":"S", "start":"@", "portal_from":"D", "portal_to":"a"}[p["category"]] for p in positions(world, m.id)}
    rows = ["".join(markers.get((X,Y), "." if nav.walkable[Y*m.width+X] else "#") for X in range(x,x+w)) for Y in range(y,y+h)]
    return dict(map=m.id, rect=[x,y,w,h], legend={".":"walkable", "#":"blocked", "N":"NPC", "O":"object", "S":"shard", "D":"portal origin", "a":"arrival", "@":"player start"}, rows=rows)


def free_space(world, m, bounds, width, height, limit=5, allow_regions=False):
    x, y, w, h = m.bounds(bounds)
    integer(width,"requested width",1,1024); integer(height,"requested height",1,1024)
    if width>w or height>h:
        return []
    occupied = {(p["x"],p["y"]) for p in positions(world,m.id)} | set(m.tags)
    regions = [] if allow_regions else [[r[k] for k in ("x","y","width","height")] for r in world.doc["regions"] if r["map"]==m.id]
    prefix = array("I", [0]) * ((w+1)*(h+1))
    for row in range(h):
        running=0
        for col in range(w):
            p=(y+row)*m.width+x+col
            running += (x+col,y+row) in occupied or any(m.grids[l][p] for l in LAYERS)
            prefix[(row+1)*(w+1)+col+1] = prefix[row*(w+1)+col+1]+running
    found=[]
    for Y in range(h-height+1):
        for X in range(w-width+1):
            area=prefix[(Y+height)*(w+1)+X+width]-prefix[Y*(w+1)+X+width]-prefix[(Y+height)*(w+1)+X]+prefix[Y*(w+1)+X]
            candidate=[x+X,y+Y,width,height]
            if area==0 and not any(overlaps(candidate,r) for r in found+regions):
                found.append(candidate)
                if len(found)>=limit:
                    return found
    return found


def validate(world, metadata, start=None, map_id=None, bounds=None, requirements=()):
    issues=[]
    def issue(code, message, **context):
        issues.append(dict(severity="error", code=code, message=message, **context))
    ids=set()
    for region in world.doc["regions"]:
        try:
            if not isinstance(region.get("id"),str) or not region["id"] or region["id"] in ids or not isinstance(region.get("name"),str) or not region["name"].strip():
                raise WorldError("Regions need unique IDs and non-empty names")
            ids.add(region["id"])
            r=world.map(region["map"]).bounds([region[k] for k in ("x","y","width","height")])
            if region.get("kind") not in ("town","connector") or region.get("weather","rain") not in ("rain","snow"):
                raise WorldError("Region kind/weather is invalid")
            for other in world.doc["regions"]:
                if other is region: break
                if other["map"]==region["map"] and overlaps(r,[other[k] for k in ("x","y","width","height")]):
                    issue("region_overlap",f"{region['id']} overlaps {other['id']}", map=region["map"], rect=list(r))
        except (WorldError,KeyError,TypeError) as exc:issue("invalid_region",str(exc))
    occupied={}
    def position(p,label):
        try:
            m=world.map(p["map"]); x,y=m.point([p["x"],p["y"]]); key=(m.id,x,y)
            if key in occupied:issue("occupied",f"{label} shares a cell with {occupied[key]}",map=m.id,at=[x,y])
            occupied[key]=label
            if m.collision(x,y)[0]!="walkable":
                t=m.collision(x,y)[1]
                issue("blocked_position",f"{label} needs walkable supporting tiles",map=m.id,at=[x,y],tile=t.key if t else None)
        except (WorldError,KeyError,TypeError) as exc:issue("invalid_position",f"{label}: {exc}")
    position(world.doc["start"],"player start")
    for i,p in enumerate(world.doc["portals"]):
        position(p.get("from",{}),f"portal {i} origin");position(p.get("to",{}),f"portal {i} arrival")
        appearance=p.get("appearance","")
        if appearance and not re.fullmatch(r"!(?:Door[1-3]-[1-8]|\$Gate[12]-1)",appearance):
            issue("portal_appearance",f"Portal {i} has unsupported appearance {appearance}")
    groups=metadata.groups
    definitions=world.doc["entities"].get("object_definitions",[])
    object_ids=set(groups["objects"])
    for definition in definitions:
        id=definition.get("id"); template=definition.get("template")
        if id in object_ids or not id or template not in groups["templates"]:
            issue("object_definition",f"Unknown template or duplicate object: {id} / {template}")
        object_ids.add(id)
    for group in ("npcs","objects","shards"):
        rows=world.doc["entities"].get(group,[])
        expected=object_ids if group=="objects" else set(groups[group])
        actual=set()
        for p in rows:
            id=p.get("id")
            if id in actual:issue("duplicate_entity",f"Duplicate {group} ID: {id}")
            actual.add(id);position(p,group+":"+str(id))
        for id in sorted(expected-actual):issue("missing_entity",f"Lua {group} ID has no native position: {id}")
        for id in sorted(actual-expected):issue("unknown_entity",f"Native {group} ID has no Lua metadata: {id}")
    objects=metadata.objects(world); appearance_ids=set()
    for override in world.doc["entities"].get("object_appearances",[]):
        id=override.get("id")
        if id in appearance_ids or id not in objects or objects[id].get("kind") not in ("chest","deck_chest") or not re.fullmatch(r"!Chest-[1-8]",override.get("appearance","")):
            issue("object_appearance",f"Invalid chest appearance override: {id}")
        appearance_ids.add(id)
    navigation=None
    if start is not None:
        try:
            m=world.map(map_id);navigation=Navigation(world,m,bounds);seen=navigation.reachable(start)
            for p in positions(world,m.id):
                if not inside(navigation.bounds,p["x"],p["y"]):continue
                x,y=p["x"],p["y"];index=y*m.width+x
                interact=p["category"] in ("npcs","objects") or (p["category"]=="portal_from" and p.get("appearance"))
                reachable=any(seen[q] for q in navigation.neighbors(index)) if interact else bool(seen[index])
                if not reachable:issue("unreachable",f"Cannot reach {'an interaction cell for ' if interact else ''}{p['id']} from {list(start)}",map=m.id,at=[x,y])
        except WorldError as exc:issue("navigation_start",str(exc))
    for req in requirements:
        try:
            m=world.map(req["map"]);nav=Navigation(world,m,req.get("rect"))
            route=nav.path(req["from"],req["to"],req.get("interact",False))
            if not route["found"]:issue("required_route",route["reason"],map=m.id,at=req["to"])
        except (WorldError,KeyError) as exc:issue("required_route",str(exc))
    return dict(valid=not issues, error_count=len(issues), issues=issues,
                navigation=dict(map=map_id,from_=list(start),reachable_cells=sum(seen)) if navigation and start is not None and 'seen' in locals() else None,
                assumptions="NPCs at manifest positions; uncleared objects; no progression-based movement gates; invisible portals terminate local paths.")


def spatial_diff(before, after, limit=20):
    maps=[]
    for id in sorted(set(before.maps)|set(after.maps)):
        a,b=before.maps.get(id),after.maps.get(id)
        if not a or not b:
            maps.append(dict(map=id,change="added" if b else "removed",size=[(b or a).width,(b or a).height]));continue
        if (a.width,a.height)!=(b.width,b.height):
            maps.append(dict(map=id,change="resized",before=[a.width,a.height],after=[b.width,b.height]));continue
        points=set();counts=Counter();samples=[]
        # Compare references, not palette indices: human saves may reorder palettes.
        for layer in LAYERS:
            translation={i: next((j for j,t in enumerate(b.palette) if t==v),-1) for i,v in enumerate(a.palette)}
            for p,(av,bv) in enumerate(zip(a.grids[layer],b.grids[layer])):
                if translation[av]!=bv and a.palette[av]!=b.palette[bv]:
                    points.add(p);counts[layer]+=1
                    if len(samples)<limit:samples.append(dict(at=[p%a.width,p//a.width],layer=layer,before=a.palette[av].json() if av else None,after=b.palette[bv].json() if bv else None))
        if points or a.tags!=b.tags or a.doc["name"]!=b.doc["name"] or a.doc["indoor"]!=b.doc["indoor"]:
            xs=[p%a.width for p in points];ys=[p//a.width for p in points]
            maps.append(dict(map=id,changed_cells=len(points),layers=dict(counts),rect=[min(xs),min(ys),max(xs)-min(xs)+1,max(ys)-min(ys)+1] if points else None,
                             samples=samples,tags_changed=a.tags!=b.tags,
                             metadata={k:dict(before=a.doc.get(k),after=b.doc.get(k)) for k in ("name","indoor") if a.doc.get(k)!=b.doc.get(k)}))
    manifest={}
    def compare(path,a,b):
        if a==b:return
        if isinstance(a,dict) and isinstance(b,dict):
            for key in sorted(set(a)|set(b)):compare(path+"/"+key,a.get(key),b.get(key))
        elif isinstance(a,list) and isinstance(b,list):
            if all(isinstance(v,dict) and "id" in v for v in a+b):
                A,B={v["id"]:v for v in a},{v["id"]:v for v in b}
                changes=[dict(id=k,before=A.get(k),after=B.get(k)) for k in sorted(set(A)|set(B)) if A.get(k)!=B.get(k)]
            else:
                changes=[dict(index=i,before=a[i] if i<len(a) else None,after=b[i] if i<len(b) else None) for i in range(max(len(a),len(b))) if (a[i] if i<len(a) else None)!=(b[i] if i<len(b) else None)]
            manifest[path]=dict(changed=len(changes),samples=changes[:limit])
        else:manifest[path]=dict(before=a,after=b)
    compare("",before.doc,after.doc)
    return dict(maps=maps,manifest=manifest)
