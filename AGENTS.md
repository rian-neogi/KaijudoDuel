# Kaijudo Duel contributor guide

## Project overview

Kaijudo Duel is a Linux-native SDL2 prototype built around the original
Duel Masters/Kaijudo C++ rules engine and Lua card scripts. The current UI
contains a top-down overworld and a Hearthstone-style duel screen.

## Build and run

Run all commands from the repository root so relative `Lua`, `Decks`, and
`Resources` paths resolve correctly.

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
tiles and NPC/shard locations. Select entities from the Lua-populated side
lists or directly on the map, then click or drag them to a free grass/path
or indoor floor tile. Switch maps with the on-screen arrows or
`PageUp`/`PageDown`; hold the arrow or WASD keys to pan large maps. Save with
the on-screen button or `Ctrl+S`.

When adding a C++ source file, add it to `GAME_SOURCES` in `CMakeLists.txt`.
Do not restore the legacy Windows/OpenGL interface to the Linux target.

## Verification

After changes to the rules engine, Lua bridge, application state, rendering,
or input handling, run:

```bash
ctest --test-dir Build --output-on-failure
```

The smoke test is headless and covers card loading, all NPC decks, binary and
card-targeted suspended Lua choices, UI action clicking, hovering, an AI turn,
rendering, and repeated duel teardown.

## Application structure

- `Source/App/Application.cpp`: process lifecycle, SDL ownership, main loop,
  shared drawing primitives, fonts, and coordinate handling.
- `Source/App/Overworld.cpp`: map movement, NPC interaction, and overworld
  rendering.
- `Source/App/WorldBuilder.cpp`: Lua world loading, map painting, NPC/shard
  placement, validation, and atomic `World.lua` saving.
- `Source/App/DuelWindow.cpp`: duel lifecycle, actions, input, drag/drop,
  choices, AI turns, and board composition.
- `Source/App/DeckBuilder.cpp`: player collection/deck persistence and the
  deck-builder screen.
- `Source/App/Menus.cpp`: overworld pause menu and auxiliary screens.
- `Source/App/CardRenderer.cpp`: card textures, zones, hands, animation,
  tapping, dragging, and hover enlargement.
- `Lua/World.lua`: authoritative seamless exterior, interior maps, named exterior
  regions, player start, interior portals, and ID-keyed NPC/shard positions. This
  file is entirely maintained by the World Builder.
- `Source/App/WorldTile.h`: stable semantic tile IDs and their compact one-byte
  serialization glyphs. Rendering and collision must use these IDs rather than
  assigning different meanings based on the current map.
- `Lua/Npcs.lua`: authoritative NPC identities, kinds, appearances, decks,
  rewards, Crest Holder awards, AI personalities, and dialogue. It does not own
  positions.
- `Lua/MercerStock.lua`: Mercer prices, initial stock, shard identities, and
  shard inventory expansions. It does not own positions.
- `Source/App/AppSupport.h`: shared logical dimensions and small UI helpers.
- `Source/AI/HeuristicBot.cpp`: phase-aware rival move scoring. Keep it from
  inspecting identities in opposing hidden zones.

Keep `Application` as the owner of SDL resources and screen state unless a
change has a clear lifetime model and test coverage.

Player-authored decks are stored exclusively in `PlayerData/Decks`. The Deck
Builder must not enumerate the bundled gameplay and NPC decks under `Decks`.

## World data conventions

- Keep all maps, portals, and entity coordinates in `Lua/World.lua`; never add
  `position` fields back to `Lua/Npcs.lua` or `Lua/MercerStock.lua`.
- NPC and shard position keys must match their metadata `id` fields and include
  a valid `map` ID. Normal gameplay requires every current entity ID to have a
  valid `World.lua` map position.
- The World Builder scans the NPC and shard metadata. New IDs without world
  entries are placed automatically on free walkable tiles when the builder is
  launched, mark the world dirty, and are persisted on the next save. Stale
  world IDs disappear on the next save.
- World Builder saves must modify only `Lua/World.lua`, leaving NPC dialogue,
  decks, rewards, Mercer stock, and other hand-authored metadata untouched.
- Maps are rectangular and may be up to 1024 columns by 1024 rows. Gameplay
  follows the player through maps larger than its 25-by-12-tile viewport; the
  World Builder keeps a 20-by-12 viewport beside its editor controls.
  Outdoor tile characters are `.` (grass), `=` (path), `~` (water), `H`
  (house), `T` (tree), `#` (dense forest), `B` (bonfire), `A` (feast table),
  `S` (walkable dueling sand), `M` (marble), `Q` (marble roof), `R` (rail),
  `X` (walkable rail
  crossing), `G` (walkable metal grate), `I` (industrial brick), `P`
  (machinery), `V` (furnace), `K` (timber roof), `J` (industrial roof), `U`
  (walkable timber bridge), and `O` (rocky cliff). Stable regional tile IDs are
  `1` (Old Road path), `2` (waystone), `3` (Cinderrail ground), `4`
  (Cinderrail path), `5` (Cinderrail rubble), `6` (Cinderrail dueling sand),
  `7` (Cinderrail door), `8` (Watershed ground), `9` (Watershed path), and
  `0` (Watershed route marker). Glasswater uses `a` (canal-stone ground), `b`
  (tideglass paving), `c` (wave roof), `d` (walkable dock), `e` (glass wall),
  `f` (walkable blue door), `g` (walkable arena floor), and `h` (harbor marker).
  Rootmaze uses `i` (clearing ground), `j` (stable path), `k` (living root),
  `l` (walkable root bridge), `m` (living roof), `n` (root wall), `o`
  (walkable door), `p` (walkable meadow arena), and `q` (leaf marker).
  Indoor/wooden-building tiles are `W` (wood wall), `D` (door), `F` (wood
  floor), `C` (counter), and `E` (workshop tools). Outdoor buildings must use
  explicit `K`, `J`, or `Q` roof tiles rather than relying on wall tiles to draw a
  roof automatically. NPCs and shards require distinct walkable tiles.
  Player starts, portal entrances, and portal destinations must remain
  walkable and unoccupied.
- Portals are directed transitions. Define both directions explicitly when a
  doorway must support entering and leaving an interior. Exterior regions share
  the `overworld` map and must connect through adjacent walkable tiles, not
  portals.

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
