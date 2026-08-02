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
- `Source/App/DuelWindow.cpp`: duel lifecycle, actions, input, drag/drop,
  choices, AI turns, and board composition.
- `Source/App/DeckBuilder.cpp`: player collection/deck persistence and the
  deck-builder screen.
- `Source/App/Menus.cpp`: overworld pause menu and auxiliary screens.
- `Source/App/CardRenderer.cpp`: card textures, zones, hands, animation,
  tapping, dragging, and hover enlargement.
- `Lua/Npcs.lua`: authoritative overworld NPC placement, decks, rewards, AI
  personalities, and dialogue metadata.
- `Source/App/AppSupport.h`: shared logical dimensions and small UI helpers.
- `Source/AI/HeuristicBot.cpp`: phase-aware rival move scoring. Keep it from
  inspecting identities in opposing hidden zones.

Keep `Application` as the owner of SDL resources and screen state unless a
change has a clear lifetime model and test coverage.

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
- Card and board assets are loaded with paths relative to the repository root.
- Opponent hands and shields stay face-down. Do not reveal them through hover
  hitboxes or previews.
- Hand cards hover immediately; other face-up cards require a continuous
  one-second dwell. Hover remains active only inside the original card bounds,
  and enlarged cards must not create a hitbox spanning unrelated zones.
- Use C++14 and match the existing tab-indented C++ style.
