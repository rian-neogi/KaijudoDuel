# Python world builder helpers

Run `python3 Tools/world_builder.py --help`. This is a headless CLI for the
existing native World files. Query results, edit reports and errors are JSON.
It uses Python 3.10+ and the game's Lua 5.4 shared library. PNG commands also
need Pillow (`python3 -m pip install -r Tools/requirements-world-builder.txt`).
The game and interactive builder require no changes to consume its output.

Paths default to this repository regardless of the working directory. Global
options go **before** the command:

```bash
python3 Tools/world_builder.py --world-dir /tmp/world-copy --compact summary
```

`--graphics-dir` and `--lua-dir` override the asset and metadata directories.
Metadata commands execute the trusted game Lua files through Lua 5.4, including
the generated `WorldObjectTemplates`; they do not try to extract tables with
regular expressions. No command edits Lua, decks, rewards, or game code.

## Discover and inspect

```bash
python3 Tools/world_builder.py summary
python3 Tools/world_builder.py summary --region gloam
python3 Tools/world_builder.py inspect --rect 543 913 9 9
python3 Tools/world_builder.py cell --map overworld --at 547 917
python3 Tools/world_builder.py space --map overworld --size 32 24 --limit 5
python3 Tools/world_builder.py catalog 'wall stone' --family Outside --limit 20 --png /tmp/walls.png
python3 Tools/world_builder.py catalog 'bed' --family Inside --limit 20 --png /tmp/beds.png
python3 Tools/world_builder.py metadata npcs Vey
python3 Tools/world_builder.py metadata templates flame --limit 8
python3 Tools/world_builder.py prefabs
```

| Command | Result |
| --- | --- |
| `summary` | World revision, ordered maps, region IDs and entity counts. With a selection: painted/empty counts, common tiles and occupant coordinates. |
| `inspect` | ASCII walkability with NPC, object, shard, player start, door and arrival markers. Maximum 4,096 cells. |
| `cell` | All normalized tile layers, tint, collision source, blocking fixtures, tag and occupants. |
| `space` | Non-overlapping empty rectangles. Checks every tile layer, tags, entity/portal positions and region claims. `--allow-regions` permits space inside an existing region. |
| `catalog` | Search actual names and stable `Family/Sheet/index` references, inferred layer, collision, source rectangle, canonical composite anchors and footprint. |
| `metadata` | Actual placeable IDs and current native positions, with useful Lua metadata. Groups: `npcs`, `objects`, `shards`, `templates`. An empty result means the ID/name is absent. |
| `prefabs` | Available building/furniture assembly types and parameters. |

Selections use `--map ID --rect X Y W H` or `--region ID`. Rectangles are
half-open: `[x,x+width)` and `[y,y+height)`. All query coordinates are global
within their map, including when selecting a region. `summary` lists the exact
region IDs; Gloam Quarry's ID is `gloam`. Without a selection, most spatial
queries select the whole overworld. Use bounded rectangles for detail queries.

Catalog searches require every search word to match the name or catalog key.
Indices are zero-based logical indices, **not** sheet image row order. B/C
sheets use two eight-column pages. `--aliases` includes raw composite pieces;
painting those keys stores their canonical tree/lamp anchor. `--limit` (1–200)
and `--offset` paginate catalog and metadata results. Contact sheets show the
returned page, with isolated and filled autotile samples.

## Render and check access

```bash
python3 Tools/world_builder.py render --region gloam --output /tmp/gloam.png
python3 Tools/world_builder.py render --map gloam_ashvault --output /tmp/ashvault.png --collision --labels
python3 Tools/world_builder.py path --region gloam --from 545 889 --to 547 917 --interact
python3 Tools/world_builder.py validate
python3 Tools/world_builder.py validate --region gloam --from 545 889
```

Previews use the existing PNG assets, native autotile connections and material
transitions, tints, and composite tree/lamp layers. Crops retain neighbors outside
the crop for autotile decisions and canopies. Optional overlays are `--grid`,
`--collision`, `--labels`, and `--regions`; `--tile-size` accepts 8, 16, 32 or 48.
Rendering is limited to 20 million pixels at both the native and output sizes;
split large regions into crops. This is a static authoring preview without
weather, time-of-day lighting, save state or animation. Unsupported legacy NPC
appearances use gold markers and appear in `missing_sprites`. New-style sprites
and template object appearances use the real character sheets.

Navigation is four-directional and local to one map. It respects tile collision,
manifest NPCs, uncleared objects and visible portal origins. Movement has no
progression prerequisites; the former Blackstone gate tag has no effect.
Visible doors are solid: use `--interact` to find a reachable adjacent cell.
Invisible portal origins end a local path because stepping there causes travel.
The returned path lists orthogonal corners; `--all-steps` returns every cell.
`--rect` constrains the entire search, so a route outside the selected rectangle
does not count. Route-duelist encounter radii and save-dependent NPC visibility
are not simulated.

`path --cleared OBJECT_ID` excludes a cleared object from obstacles; repeat it for
several IDs. This changes the query's assumptions, never the world or a save file.
Bushes and rocks placed with the builder can be cleared without a particular card.

`validate` checks structure, region overlaps, supporting tile walkability,
distinct positions, appearances, and correspondence with every current Lua
entity/template. With `--from`, it also flood-fills the selection and checks
that every selected NPC, object and visible door has a reachable interaction
cell, and every shard, arrival and invisible exit is reachable. Ordinary
structural validation alone cannot establish that a door apron is reachable.

## Edit, review and undo

Edits use versioned JSON patches. No manual palette or RLE manipulation is needed.
Operations run in order in memory. The tool validates the final state before
writing anything to World. A rejected patch leaves the on-disk world unchanged.

The supplied example creates a furnished 12×10 workshop map. It deliberately has
no connection to the exterior; the paired-entrance operation below adds one once
an exterior site is selected. Test it on a copy:

```bash
cp -a World /tmp/world-helper-demo
python3 Tools/world_builder.py --world-dir /tmp/world-helper-demo apply Tools/examples/workshop.patch.json --dry-run --preview /tmp/workshop.png --map llm_workshop_demo
python3 Tools/world_builder.py --world-dir /tmp/world-helper-demo apply Tools/examples/workshop.patch.json --receipt /tmp/workshop-edit.json
python3 Tools/world_builder.py --world-dir /tmp/world-helper-demo diff --against World
python3 Tools/world_builder.py --world-dir /tmp/world-helper-demo undo /tmp/workshop-edit.json
```

`apply -` reads a patch from standard input. `--dry-run` reports `would_write`,
validation results and a spatial diff. `--preview PATH` also renders the candidate
world, including during dry runs. Preview selection and overlay flags work as
with `render`. Reports, PNGs and receipts must be outside the native World folder.

An applied edit returns its new revision and undo receipt path. Supply a new
`--receipt PATH` for lasting history; the default is a unique receipt under the
system temporary directory. Receipts contain the exact previous bytes. Undo
refuses to proceed if any loaded world file changed after that edit. Undo the
most recent edit first; keep receipts outside World and version control as desired.

Every query includes a SHA-256 `revision` over the manifest and registered map
files. Set `expected_revision` in a patch to reject an edit based on stale input.
The writer also rechecks the loaded files just before writing and serializes
helper processes with a lock. Individual files use atomic replacement; ordinary
write failures roll back completed replacements. Multi-file edits are not a
single crash-atomic filesystem transaction. The interactive C++ builder does not
take the helper lock; avoid saving from both tools at the same time.

The helper preserves existing palette indices, untouched map bytes, unknown
top-level map metadata, and entity metadata. Spatial diffs compare tile references
instead of palette numbers and report changed rectangles, cell/layer counts,
bounded samples and manifest changes. Diff results default to at most 20 samples
per map/manifest array; use `diff --limit N` to change that. An RLE line changing
does not imply its entire map row changed.

## Patch format

```json
{
  "version": 1,
  "scope": [{ "map": "overworld", "rect": [488, 850, 128, 96] }],
  "protect": [{ "map": "overworld", "rect": [535, 850, 3, 4] }],
  "frames": { "town": { "map": "overworld", "origin": [488, 850] } },
  "anchors": { "plaza": { "frame": "town", "at": [57, 39] } },
  "operations": [
    { "op": "fill", "frame": "town", "rect": [56, 38, 3, 3], "tile": "Outside/A2/3" }
  ],
  "require_routes": [{ "frame": "town", "from": "plaza", "to": [57, 93], "interact": true }]
}
```

`scope` is required. Each entry declares a map and absolute rectangle; optional
`layers` limits tile writes to specific layers. `protect` uses the same format
and vetoes writes. Both the old and new positions of moved entities/portals must
be in scope. Manifest placement and region changes require all three layers to
be in scope. New-map creation requires its entire rectangle to be in scope.
Scope describes stored cells; autotile edges and overhanging artwork may change
the appearance of neighboring cells. Include a margin in visual reviews.

Operations may name `map` directly or a `frame`; frame coordinates are offsets
from its origin. Frames and anchors exist only in the patch and are not saved as
unsupported game fields. A point may be `[x,y]` or an anchor name. Anchor points
are resolved in their own frame and may not refer to other anchors. A rectangle
is always `[x,y,width,height]`. Source rectangles for `copy`/`move`, scopes, and
protected rectangles are absolute. Neither painting nor stamps silently clip.

A tile is a key such as `Outside/A2/3`, or an object:

```json
{ "tileset": "Outside", "sheet": "A2", "index": 3, "tint": [180, 190, 210], "layer": "ground" }
```

Tint defaults to white and layer defaults to the native catalog's inferred layer.
Trees and streetlights always canonicalize to decoration anchors. Large trees on
the same row require two-column spacing; the helper rejects placements the native
loader would silently remove. Footprints must fit inside the map.

| Operation | Additional fields and behavior |
| --- | --- |
| `fill` | `rect`, `tile`; optional `layer` override. Changes only that tile's layer. |
| `erase` | `rect`, `layer`. Removes that layer, exposing layers below. |
| `normalize` | `map`. Materializes native composite alias/layer normalization and tree-spacing cleanup. Every changed cell must be in scope. Use before editing legacy component layouts; queries already display their native normalized appearance. |
| `path` | `points`, `tile`, optional `width` (default 1), `shoulder` tile and `shoulder_width` (default 1). Consecutive points connect with orthogonal grid steps. |
| `road_network` | `paths` is an array of point arrays; otherwise same as `path`. Paints every shoulder before every road surface to preserve intersections. |
| `stamp` | `prefab` and fields described below; each component retains its render layer. |
| `copy`, `move` | `source: {"map":"ID","rect":[x,y,w,h]}`, destination `at`, optional `layers`. Copies tiles, including empty cells, from a snapshot; overlapping moves work. Does not move tags, entities or portals. |
| `new_map` | `id`, `name`, `width`, `height`, boolean `indoor`. Registers a blank native map; safe unique ID and dimensions 1–1024 required. Paint supporting floors separately. |
| `region_set` | `id`, `name`, `rect`, `kind` (`town`/`connector`), optional `weather` (`rain`/`snow`). Creates or updates an exterior rectangle, preserving unspecified existing weather. Overlaps are rejected. |
| `entity_move` | `group` (`npcs`/`objects`/`shards`), existing metadata `id`, `position`. Can place an unplaced metadata ID. Optional object `appearance` selects a chest variant; null removes its override. |
| `object_add` | Unique `id`, real Lua `template`, `position`; optional chest `appearance`. Adds both native object definition and position. |
| `start_move` | `position`. Moves the player start. |
| `tag_set` | `at`, `value` (string, or null to remove). Tags do not invent new game behavior; preserve story/save-referenced IDs. |
| `portal_add` | `from`, `to` positions and optional `appearance`. One directed portal. Omitted appearance means an intentional invisible step trigger. |
| `portal_remove` | `index` from query occupant IDs (`portal:N:from/to`). Indices refer to the current operation state and shift after removal. Use `expected_revision` when editing existing portals. |
| `entrance` | Four named positions and appearances as shown below. Creates both directed portals and checks access in each direction. |

`path` and `road_network` use square brushes. Odd widths are symmetric; even
widths extend one cell farther toward negative coordinates. `clip: true`
explicitly clips their footprint at map edges; it never bypasses scope/protect.
Operations are ordered: later writes to the same layer replace earlier ones.

Stamp types:

- `building`: `rect`, `roof`, `wall`; `roof_height` defaults to height minus 2.
- `room`: `rect`, `floor`, `wall`; `wall_height` defaults to 2. Paints a floor,
  upper wall and one-cell perimeter. Cut door cells with `fill`/`erase` afterward.
- `vertical`: `at`, top-to-bottom `tiles`, e.g. `["Inside/B/80","Inside/B/88"]`.
- `table_setting`: `at`, `base`, `top`. Uses decoration for the table and
  foreground for the surface prop, so the prop does not erase its support.
- `custom` (default): `at`, `cells: [[dx,dy,tile], ...]`. Reuse an explicit
  multi-layer assembly such as a tent, shelf or furnished corner.

All nested positions use `{ "map": "ID", "at": [x,y] }` or
`{ "frame": "NAME", "at": "ANCHOR" }`. An entrance example:

```json
{
  "op": "entrance",
  "outside_door": { "map": "overworld", "at": [100, 104] },
  "outside_return": { "map": "overworld", "at": [100, 105] },
  "inside_arrival": { "map": "llm_workshop_demo", "at": [5, 7] },
  "inside_exit": { "map": "llm_workshop_demo", "at": [5, 9] },
  "appearance": "!Door3-5"
}
```

Choose and paint the exterior site first: these example coordinates are not a
preselected existing doorway. All four positions must be distinct, walkable and
unoccupied. `exit_appearance` optionally makes the interior exit visible. Door
appearances accept `!Door1-1` through `!Door3-8`, `!$Gate1-1`, and `!$Gate2-1`.
Exterior regions remain joined by ordinary adjacent walkable terrain.

An entrance automatically requires outside-return → outside-door interaction
access and inside-arrival → inside-exit access. Add `require_routes` to prove
connection to a town plaza or main road as well. Each requirement accepts `map`
or `frame`, `from`, `to`, optional `rect`, and boolean `interact`.
Successful syntax and placement checks alone do not prove gameplay connectivity.

Exit codes: **0** success; **1** invalid world/required route or no path;
**2** malformed input, unavailable dependency, stale revision, unsafe operation
or I/O error. Validation errors include codes and relevant coordinates.

## Verification and remaining limits

```bash
python3 Tools/test_world_builder.py
python3 Tools/test_world_builder_native.py
ctest --test-dir Build --output-on-failure
```

The first suite checks transactions, scope, metadata, routes, composites,
assemblies, previews and preservation on temporary worlds. The native suite
compiles a temporary oracle linked to the actual C++ sources and compares every
available catalog index, collision and inferred layer, source rectangles, all
autotile quarter masks, ground transitions, composite normalization, generated
map round-tripping and rendered sample pixels. It requires the game development
dependencies. Pillow-specific tests skip if that optional dependency is absent.

These tools address the spatial inventory, catalog discovery, RLE editing,
coordinate conversion, assembly, doorway, preview, road-overlap and review
friction recorded in `Notes/GloamQuarryImplementationLog.md`. They cannot resolve
conflicting design documents or create absent NPC dialogue, duels, shops, quests,
rewards or progression rules. Those remain authored game metadata and logic.
