# Overworld JSON format

The seamless exterior is stored in `World/Maps/overworld.json`. This file owns
the map dimensions, visual tiles, inferred collision inputs, and map-local
gameplay tags. It does not own regions, portals, the player start, or entity
positions; those belong to `World/World.json`.

## Top-level fields

| Field | Meaning |
| --- | --- |
| `format` | Format identifier. It must be `kaijudo-catalog-map`. |
| `version` | Schema version. The current version is `1`. |
| `id` | Stable map identifier referenced by `World/World.json`. |
| `name` | Display name used by the application. |
| `indoor` | Whether the map is an interior. It is `false` for the overworld. |
| `width`, `height` | Map dimensions in tiles. The overworld is 1024 by 1024. |
| `palette` | Map-local table of tile references used by the layer data. |
| `layers` | Ground, decoration, and foreground tile grids encoded with RLE. |
| `source` | Informational migration provenance; it is not used for rendering. |
| `tags` | Optional coordinate-based gameplay semantics. |

Coordinates are zero-based. `(0,0)` is the northwest corner, `x` increases to
the east, and `y` increases to the south. A coordinate maps to a flattened
row-major layer index with:

```text
cellIndex = y * width + x
```

The current 408-by-185 migrated world begins at `(288,665)` within the
1024-by-1024 canvas. Emberglen's region origin is `(512,700)`. Cells outside
the authored terrain are empty and blocked.

## Palette

`palette[0]` is always `null`. A layer value of `0` therefore means that the
cell has no tile on that layer. Every other palette element has this form:

```json
{
  "tileset": "Outside",
  "sheet": "A2",
  "index": 0,
  "layer": "ground",
  "tint": [255, 255, 255]
}
```

- `tileset` selects the `Dungeon`, `Inside`, `Outside`, or `World` tileset
  family under `Resources/Graphics/Tilesets`.
- `sheet` selects `A1`, `A2`, `A3`, `A4`, `A5`, `B`, or `C`, when that family
  provides the sheet.
- `index` is the zero-based logical tile index within the sheet. Autotile
  sheets use logical autotile blocks rather than raw 32-pixel image cells.
- `layer` must match the layer that references this palette entry. Layer choice
  is inferred by the World Builder: A-series tiles use ground, ordinary B/C
  tiles use decoration, and metadata-marked front pieces use foreground.
- `tint` contains red, green, and blue texture multipliers from 0 through 255.
  `[255,255,255]` leaves the source art unchanged. Tint support remains part of
  the native format even though the migrated overworld palette is neutral.

The palette is local to this map. Its numeric indices are serialization
details, not stable gameplay IDs; saving in the World Builder may reorder them.

## Layers and run-length encoding

The `layers` object contains exactly `ground`, `decoration`, and `foreground`.
They render in that order, with foreground drawn above characters. Each layer
uses row-major run-length encoding:

```json
"ground": {
  "encoding": "rle-row-major",
  "data": [[0, 681248], [1, 3], [0, 10]]
}
```

Each pair is `[paletteIndex, cellCount]`. Decode it by repeating the palette
index `cellCount` times, then continuing with the next pair. The sum of all run
lengths in each layer must equal `width * height`. For the overworld, that is
`1,048,576` cells per layer.

The long runs of palette index `0` at the beginning and end of the overworld
represent the currently unassigned parts of the 1024-by-1024 canvas.

## Collision

There is no separate collision grid. At a cell, the engine checks foreground,
decoration, then ground and uses the first tile with defined collision
behavior. If no layer supplies collision, the cell is blocked. Consequently,
an entirely empty cell is not walkable.

Collision rules come from tileset family, sheet, tile index, and tile metadata.
Changing a visual tile can therefore change pathability.

## Tags

Tags attach stable gameplay meaning to coordinates when it cannot be inferred
from artwork alone:

```json
{
  "x": 536,
  "y": 754,
  "value": "blackstone_gate"
}
```

The `blackstone_gate` tag lets story logic keep the relay gate closed until the
Confluence Crest has been earned. Tags should remain stable once code or saved
progress refers to them.

## Related world data

`World/World.json` lists this map as `Maps/overworld.json` and separately owns:

- named town and connector regions;
- the player starting position;
- directed portal endpoints;
- NPC, object, and shard positions.

All of those overworld coordinates use the same 1024-by-1024 coordinate system.
Use the World Builder to edit normal map content. Direct JSON edits must retain
valid palette indices, exact layer cell totals, in-bounds tags, and walkable,
non-overlapping world positions.
