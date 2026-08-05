# Kaijudo Duel contributor guide

## Project overview

Kaijudo Duel is a Linux-native SDL2 prototype built around the original
Duel Masters/Kaijudo C++ rules engine and Lua card scripts. The current UI
contains a top-down overworld and a Hearthstone-style duel screen.

## Build and run

Run all commands from the repository root so relative `Lua`, `Decks`,
`Resources`, and `World` paths resolve correctly.

```bash
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++
cmake --build Build --parallel
./Bin/KaijudoDuel
```

Additional launch modes:

```bash
./Bin/KaijudoDuel --duel <player-deck> <ai-deck>
./Bin/KaijudoDuel --world-builder
./Bin/KaijudoDuel --lua-trace --duel <player-deck> <ai-deck>
```

The World Builder is available only through `--world-builder`. It edits map
tiles and NPC/object/shard locations. Single-click an entity side-list row to
select it; double-click it to center its map location. Select entities from the
Lua-populated side lists or directly on the map, then click or drag them to a
free walkable tile. The tile palette exposes the complete Dungeon, Inside,
Outside, and World tilesets. Use the sheet arrows to browse A1-A5/B/C and hover
a source-sheet region to see its metadata name. The render layer is inferred
from the selected tile. The palette preserves each PNG's native rows,
columns, and aspect ratio. Left-click/drag paints the selected visual layer;
right-click/drag erases it. Collision is inferred from the uppermost catalog
tile with collision behavior; cells without a collision-bearing catalog tile
are blocked. Switch maps with the on-screen arrows or `PageUp`/`PageDown`; hold
the arrow or WASD keys to pan large maps. Use the mouse wheel over the map or
`+`/`-` to zoom; the wheel continues to scroll when the pointer is over a side
list. Save with the on-screen button or `Ctrl+S`.

When adding a C++ source file, add it to `GAME_SOURCES` in `CMakeLists.txt`.
Do not restore the legacy Windows/OpenGL interface to the Linux target.

## Verification

After changes to the rules engine, Lua bridge, application state, rendering,
or input handling, run:

```bash
ctest --test-dir Build --output-on-failure
```

The test suite is headless. It covers card loading, all NPC decks, binary and
card-targeted suspended Lua choices, UI action clicking, hovering, an AI turn,
rendering, native world loading and map round-tripping, the deprecated-world
converter, and repeated duel teardown.

## Application structure

- `Source/App/Application.cpp`: process lifecycle, SDL ownership, main loop,
  shared drawing primitives, fonts, and coordinate handling.
- `Source/App/AssetManager.h/.cpp`: shared SDL texture cache and texture lifetime.
- `Source/App/SpriteSheetRenderer.h/.cpp`: directional character-sheet frame
  selection and nearest-neighbor sprite rendering.
- `Source/App/Overworld.cpp`: map movement, NPC interaction, and overworld
  rendering.
- `Source/App/WorldBuilder.cpp`: native world loading, map painting,
  NPC/object/shard placement, validation, and native world saving.
- `Source/App/WorldData.h/.cpp`: renderer-independent ownership and lookup for
  maps, regions, portals, the player start, and entity positions. World
  serializers populate this model rather than exposing their file format to
  gameplay code.
- `Source/App/RtpTile.h`: renderer-independent RTP tile references, layers,
  families, sheets, and collision values shared by world data and rendering.
- `Source/App/CatalogMapStorage.h/.cpp`: validated native JSON loading and
  atomic saving for catalog-only map files.
- `Source/App/WorldStorage.h/.cpp`: validated loading and atomic saving for the
  native world manifest, including regions, portals, start, and entity positions.
- `Source/App/DeprecatedWorldTileRenderer.h/.cpp`: frozen compatibility
  renderer for the legacy one-byte semantic maps. Do not add new behavior or
  remove it without an explicit legacy-cleanup request.
- `Tools/convert_legacy_world.py`: one-way migration tool from `Lua/World.lua`
  to `World/World.json` and catalog-only `World/Maps/*.json`. It intentionally
  ignores the old visual `tile_layers` data.
- `Source/App/WorldObject.h/.cpp`: overworld-object metadata loading and stable
  object kinds.
- `Source/App/DuelWindow.cpp`: duel lifecycle, actions, input, drag/drop,
  choices, AI turns, and board composition.
- `Source/App/DeckBuilder.cpp`: player collection/deck persistence and the
  deck-builder screen.
- `Source/App/Menus.cpp`: overworld pause menu and settings screen.
- `Source/App/SaveMenu.cpp`: title screen, save-slot selection, legacy-data
  migration, and selected-save lifecycle.
- `Source/App/CardRenderer.cpp`: card textures, zones, hands, animation,
  tapping, dragging, and hover enlargement.
- `World/Maps/*.json`: authoritative catalog tile layers, map dimensions, and
  map-local gameplay tags. The World Builder saves tile edits here.
- `World/World.json`: authoritative map list, named exterior regions, player
  start, portals, and ID-keyed NPC/object/shard positions.
- `Lua/World.lua`: deprecated migration input. Normal gameplay and the World
  Builder do not read or write it.
- `Source/App/WorldTile.h`: deprecated semantic tile IDs and their compact
  one-byte serialization glyphs. They exist only for compatibility and the
  migration source; new world behavior must use catalog tiles or map tags.
- `Source/App/Landmarks.h`: one-time route discoveries, defined in region-local
  coordinates so seamless-map regions may move without invalidating them.
- `Lua/Npcs.lua`: authoritative NPC identities, kinds, appearances, decks,
  rewards, Crest Holder awards, AI personalities, and dialogue. It does not own
  positions. Appearance values use `<character-sheet>-<one-based index>` (for
  example, `Actor2-3` selects the third character in `Actor2.png`).
- `Lua/Objects.lua`: authoritative interactive-object identities, kinds, names,
  interaction text, and deck-chest rewards. It does not own positions.
- `Lua/MercerStock.lua`: Mercer prices, initial stock, shard identities, and
  shard inventory expansions. It does not own positions.
- `Source/App/AppSupport.h`: shared logical dimensions and small UI helpers.
- `Source/AI/HeuristicBot.cpp`: phase-aware rival move scoring. Keep it from
  inspecting identities in opposing hidden zones.

Keep `Application` as the owner of SDL resources and screen state unless a
change has a clear lifetime model and test coverage.

Player-authored decks are stored exclusively in `PlayerData/<save>/Decks`.
Collection, profile, and progress files live beside that save's `Decks`
directory. Global settings remain at `PlayerData/settings.txt`. The Deck Builder
must not enumerate the bundled gameplay and NPC decks under `Decks`.
New saves copy all five `Decks/Starter` decks into their own `Decks` directory;
their initial collection uses the maximum copy count of each card across those
five decks.

## World data conventions

- `World/World.json` is the sole runtime/editor manifest. It owns the ordered
  map-file list, regions, player start, directed portals, and NPC/object/shard
  positions. Never add positions back to the Lua metadata files.
- `World/Maps/*.json` is the sole runtime/editor source for map dimensions,
  catalog tile layers, and map-local gameplay tags. Each manifest map path must
  be relative and remain beneath `World`; map IDs may contain only letters,
  digits, `_`, and `-` so they are safe as filenames.
- NPC, object, and shard position IDs must match their Lua metadata `id` fields
  and include a valid map ID. Normal gameplay requires every current entity to
  have a native manifest position. Player start, portal endpoints, NPCs,
  objects, and shards must occupy distinct walkable cells.
- Duel-enabled NPCs use ordered, non-empty `decks` and `rewards` arrays. When
  `max_battles` exceeds either array's size, the last entry in that array is
  reused for the remaining battles.
- Exterior regions require an explicit `kind` of `town` or `connector` for
  geographic organization. NPC kinds may be placed in either region kind. The
  World Builder must preserve region kinds when saving.
- The World Builder scans the NPC, object, and shard metadata. New IDs without
  world entries are placed automatically on free walkable tiles when the
  builder is launched, mark the world dirty, and are persisted on the next
  save. Stale world IDs disappear on the next save.
- World Builder saves may modify only `World/Maps/*.json` and `World/World.json`,
  leaving NPC dialogue, object text, decks, rewards, Mercer stock, and other
  hand-authored metadata untouched.
- Opened deck chests are persisted per save as `object.opened.<id>=1`. Their
  rewarded deck files are copied into the save's Decks folder and their cards
  are added to that save's collection exactly once.
- Maps are rectangular and may be up to 1024 columns by 1024 rows. Gameplay
  follows the player through maps larger than its 25-by-12-tile viewport; the
  World Builder keeps a 960-by-576-pixel viewport beside its editor controls
  (20-by-12 tiles at 100% zoom).
- The `overworld` map is allocated at 1024-by-1024. The migrated 408-by-185
  world occupies offset `(288,665)`, placing the Emberglen region origin at
  `(512,700)`. Unassigned cells use empty palette index `0` on every layer and
  are therefore blocked until painted.
- Every native map contains a map-local palette and exactly three row-major,
  run-length encoded layers: `ground`, `decoration`, and `foreground`. Palette
  index `0` is empty. Every non-empty palette entry identifies its tileset
  family, source sheet, tile index, render layer, and RGB tint. Each layer's
  runs must total exactly `width * height` cells.
- Collision is inferred from the uppermost collision-bearing catalog tile in
  foreground, decoration, ground order. If no layer supplies collision, the
  cell is blocked. Do not add a separate collision grid or rely on deprecated
  glyph collision.
- Use map `tags` for gameplay semantics that cannot be inferred from artwork.
  Keep tag IDs stable once saves or story logic reference them. The current
  `blackstone_gate` tag marks the relay gate, which stays blocked until the
  Confluence Crest has been earned.
- Portals are directed transitions. Define both directions explicitly when a
  doorway must support entering and leaving an interior. Exterior regions share
  the `overworld` map and must connect through adjacent walkable tiles, not
  portals.
- Treat connecting regions as explorable adventures rather than transit
  corridors. The implemented Watershed Crossroads is 128-by-72 tiles and the
  Old Road is 96-by-48; preserve their readable main routes, optional loops,
  camps, route duelists, and off-road shard discoveries when editing them.
- Landmark IDs are persistent progress keys. Keep them stable, keep landmark
  coordinates relative to their named region, and ensure that a walkable tile
  lies within every discovery radius.
- Town NPC interaction capabilities come from `Lua/Npcs.lua` `options` fields.
  Talk is always present; Duel and Trade must not appear unless enabled. Route
  duelist taxicab-distance encounter radii also come from Lua. Their first
  undefeated radius encounter is forced, while rematches are voluntary.
- New NPC appearances should use `<character-sheet>-<one-based index>`. Legacy
  named and `generic-male-*`/`generic-female-*` appearances remain accepted for
  compatibility but should not be used for new metadata.

## Legacy world migration

- `Lua/World.lua` is migration input only. Normal gameplay and the World
  Builder must never fall back to it or write it.
- Run `python3 Tools/convert_legacy_world.py` only when deliberately rebuilding
  the native world from the legacy byte maps. It overwrites `World/World.json`
  and `World/Maps/*.json`, discarding later World Builder edits.
- The converter translates only each legacy map's base `tiles` byte grid for
  visuals and intentionally ignores legacy `tile_layers`. It copies regions,
  start, portals, and entity positions into the native manifest. For the
  exterior it applies the common `(288,665)` offset, expands the canvas to
  1024-by-1024, and emits one neutral palette entry per unique
  tileset/sheet/tile/layer reference. Native tint support remains available for
  deliberate post-migration authoring.
- Keep the deprecated loader and renderer isolated from new world features.
  New maps and editor saves must remain fully native.

## Rules-engine safety

- The Lua scripts are the authoritative card-rule layer. Preserve their card
  names and callback behavior when changing the UI.
- Hold `gMutex` whenever UI code reads or mutates live `Duel` state.
- A Lua callback may suspend while waiting for a choice. Never re-enter the
  same Lua state while `mLuaCallbackSuspended` is true; use cached choice data
  and base card values during that interval.
- Keep Lua stack restoration on every success and error path in `Card`,
  `Choice`, and `Modifier` callbacks.
- Card unique IDs are indices into `mCardList`; validate them before access.

## SDL/UI conventions

- Rendering uses a fixed 1280x800 logical canvas. Because
  `SDL_RenderSetLogicalSize` filters mouse events into logical coordinates,
  do not scale SDL mouse-event coordinates a second time.
- Overworld and World Builder rendering must iterate only the bounds returned by
  `visibleTileBounds`, including its one-tile margin. Do not restore full-map
  tile loops as the exterior grows toward 1024 by 1024 tiles.
- Card and board assets are loaded with paths relative to the repository root.
- Opponent hands and shields stay face-down. Do not reveal them through hover
  hitboxes or previews.
- Hand cards hover immediately; other face-up cards require a continuous
  one-second dwell. Hover remains active only inside the original card bounds,
  and enlarged cards must not create a hitbox spanning unrelated zones.
- Use C++14 and match the existing tab-indented C++ style.
