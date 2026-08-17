# Catalog map format

`World/World.json` is the authoritative native world manifest. It lists the
maps and owns regions, the player start, portals, and ID-keyed NPC, object, and
shard positions. `World/Maps/*.json` owns visual layers, dimensions, collision,
and map-local gameplay tags. Gameplay and the World Builder load these files
directly and never read `Lua/World.lua`.

Objects created from the World Builder's Lua-populated Add palette are recorded
in the manifest's optional `entities.object_definitions` array. Each definition
maps its generated object ID to a template in `Lua/Objects.lua`; its location is
stored in the normal `entities.objects` array.

See `World/OverworldFormat.md` for a field-by-field explanation of the catalog
map schema, palette references, RLE layers, collision, coordinates, and tags.

For a one-time migration from the deprecated Lua format, run from the
repository root:

```bash
python3 Tools/convert_legacy_world.py
```

This is a migration command: rerunning it overwrites the native manifest and
catalog maps from the deprecated Lua file, and therefore discards later World
Builder edits.

For visuals, the converter reads only each Lua map's `tiles` array and
deliberately ignores `tile_layers`, so existing hand-painted catalog layers
cannot silently affect base-map migration. It also copies regions, the player
start, portals, and entity positions into `World/World.json`.

Each JSON file contains a map-local palette and three row-major, run-length
encoded visual layers. Palette index `0` is empty; every other entry identifies
the tileset family, source sheet, source index, render layer, and RGB tint.
Each run is `[paletteIndex, cellCount]`. The sum of cell counts in every layer
must equal `width * height`.

The exterior `overworld` canvas is 1024-by-1024. The legacy 408-by-185 world is
placed at offset `(288,665)`, so Emberglen begins at `(512,700)`. Space outside
that migrated rectangle is encoded as palette index `0` on all layers and is
blocked until terrain is painted there. The converter deduplicates neutral
palette references by tileset, sheet, tile index, and layer; native files still
retain RGB tint fields for intentionally authored color variations.

The optional `tags` array preserves gameplay semantics that cannot be inferred
from a visual tile alone. The initial converter emits `blackstone_gate` tags for
legacy `z` cells. The files contain no legacy glyph grid.

Legacy trees were drawn from hand-cut, multi-cell atlas rectangles. Because a
catalog cell contains exactly one tile, the converter substitutes the named
single-cell Tree/foliage catalog entries rather than reproducing those custom
rectangles.

At startup, C++ loads every map referenced by `World/World.json`. Missing,
partial, malformed, or structurally inconsistent native data is rejected
instead of silently mixing formats. World Builder edits are saved atomically
to the manifest and map JSON files. The legacy renderer remains isolated for
old code compatibility, but is not part of the normal loading path.
