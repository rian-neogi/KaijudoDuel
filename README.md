# Kaijudo Duel

Kaijudo Duel is a Linux-native prototype that keeps the original Lua-driven
card rules and presents them through a new SDL2 interface. It includes a
top-down overworld, NPC encounters, and a tavern-table duel screen.

## Build on Ubuntu or Debian

Install the compiler and development libraries:

```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev \
  libsdl2-ttf-dev liblua5.4-dev libboost-dev
```

Configure and compile from the repository root:

```bash
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++
cmake --build Build --parallel
```

The explicit compiler setting also repairs build directories left over from
the old Windows/Clang configuration. If CMake reports another stale-cache
problem, configure into a new directory instead:

```bash
cmake -S . -B LinuxBuild -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++
cmake --build LinuxBuild --parallel
```

The executable is written to `Bin/KaijudoDuel`. Run it from the repository
root so it can find the `Lua`, `Decks`, and `Resources` directories:

```bash
./Bin/KaijudoDuel
```

## Command-line options

Open directly into a duel against the heuristic AI by providing the player
deck first and AI deck second:

```bash
./Bin/KaijudoDuel --duel \
  "My Decks/7 - L Tappy Tappy.txt" \
  "VeiledOne.txt"
```

Add `--full-visibility` to reveal and inspect the AI hand during a direct duel:

```bash
./Bin/KaijudoDuel --full-visibility --duel \
  "My Decks/7 - L Tappy Tappy.txt" \
  "VeiledOne.txt"
```

The direct-duel process closes after the match or when Escape is pressed. It
does not award campaign cards, gold, or NPC victories. Relative deck names are
searched beneath `Decks/` automatically; explicit existing paths still work.
Paths containing spaces must be quoted. Deck files use one card entry per line:

```text
4 Aqua Hulcus
2 Aqua Sniper
```

Run `./Bin/KaijudoDuel --help` to display all startup options. Missing files,
malformed lines, unknown cards, and decks with fewer than ten cards are
reported before the duel begins.

## Controls

### Overworld

- Move with WASD or the arrow keys.
- Talk to a nearby duelist with E, Space, or Enter.
- Advance dialogue and accept a duel with E, Space, or Enter.
- Close dialogue or quit with Escape.

### Duel

- Drag a hand card onto your battle zone to cast it; legal mana is tapped automatically.
- Drag a hand card onto your mana row to charge it as mana.
- Drag one of your creatures onto an opposing creature or the rival area to attack.
- Hover over any face-up card to make that card rise, straighten, and enlarge in place.
- Click the relevant card itself when choosing a target, selecting a shield, blocking, or tapping mana.
- Your hand and the rival's face-down hand are fanned across their side of the table.
- Click a card outside a pending choice to focus the fallback actions associated with it.
- Click an action on the right, or press its displayed number key.
- Press Escape to leave a campaign duel and return to the overworld. In direct-duel mode, Escape closes the application.

The three NPCs use different decks and can be challenged again after a duel.

## Automated smoke test

The smoke test initializes the Lua card database, loads the player deck and
all three NPC decks, evolves cards through a Lua target choice, exercises card
hovering and an AI turn, renders the board, and checks repeated duel teardown.
It can run without a display server:

```bash
ctest --test-dir Build --output-on-failure
```

An exit code of zero indicates that startup, rule loading, rendering, and
duel teardown succeeded.

You can also invoke the same test executable directly:

```bash
SDL_VIDEODRIVER=dummy ./Bin/KaijudoDuel --smoke-test
```
