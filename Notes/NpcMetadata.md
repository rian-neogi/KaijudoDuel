# NPC Metadata

Overworld NPCs are defined in `Lua/Npcs.lua`. The file returns an array with one table per NPC and is loaded before SDL creates the game window.

The loader rejects invalid metadata with a specific startup error. It validates duplicate IDs and names, kinds, sprite appearances, positions, deck files, reward cards, reward limits, and empty rosters.

NPC positions belong exclusively to `Lua/World.lua`. Each exterior region also declares `kind = "town"` or `kind = "connector"` for geographic organization and editor labels. Region kinds do not restrict which NPC kinds may be placed there.

## Town NPC template

```lua
{
    id = "unique_id",
    name = "Display Name",
    kind = "town_npc",
    options = {
        duel = true,   -- optional, defaults to false
        trade = false, -- optional, defaults to false
        wander = true  -- optional, defaults to true
    },
    appearance = "mira",

    max_battles = 4,

    decks = { "Example.txt", "ExampleAdvanced.txt" },
    rewards = {
        { card = "First Reward Card", gold = 100 },
        { card = "Later Reward Card", gold = 125 },
    },

    ai = {
        personality = "balanced"
    },

    dialogue = {
        greeting = "Hello. What brings you here?",
        talk = "Optional everyday conversation after choosing Talk.",
        defeat = "The player defeated me.",
        victory = "I defeated the player.",
        complete = "You have earned every reward I can offer.",
        clue = "Optional Act I clue.",
        investigation = "Act I investigation dialogue.",
        stabilize_before = "Dialogue before the first stabilizing victory.",
        stabilize_after = "Dialogue after a stabilizing victory.",
        boss_reveal = "Dialogue while the Act I boss is available.",
        act_complete = "Dialogue after Act I."
    }
}
```

Interacting with a town NPC first displays `greeting`, then opens a menu containing Talk, any Lua-enabled Duel or Trade options, and Leave. Talk uses story-specific dialogue when relevant and otherwise uses `talk`, falling back to `greeting`. A disabled capability is absent from the menu rather than shown as an unusable choice.

### Battle sequence

`max_battles` can be between one and four. Duel-enabled NPCs require non-empty
`decks` and `rewards` arrays. Entries are used in order for each battle. If
`max_battles` is greater than either array's size, that array's final entry is
reused independently for every remaining battle. In the example above, battles
three and four use `ExampleAdvanced.txt` and grant `Later Reward Card` with 125
gold.

Deck names are searched beneath `Decks/` automatically. An explicit path can
still be used for a deck elsewhere; if that path exists, it takes priority.

## Route duelist template

Route duelists wander within the 3-by-3 area centered on their authored `World.lua` position. Before their first defeat, they detect the player anywhere inside their configured taxicab-distance radius. Detection is independent of facing. The trainer stops wandering as soon as the player is detected, displays an exclamation mark, stops player movement, and follows a cardinal path around walkable obstacles until adjacent before delivering `greeting` and starting a forced duel. Losing does not permanently lock the player in another challenge; leaving and re-entering the radius re-arms it. Later battles are voluntary rematches. Route duelists may be positioned in towns or connecting regions.

```lua
{
    id = "road_trainer",
    name = "Road Trainer",
    kind = "route_duelist",
    sight = { range = 7 },
    appearance = "generic-male-1",
    max_battles = 4,
    decks = { "Example.txt" },
    rewards = {
        { card = "Reward Card", gold = 100 },
    },
    ai = { personality = "balanced" },
    dialogue = {
        greeting = "I saw you. Prepare to duel!",
        defeat = "You won this time.",
        victory = "Watch the road more carefully.",
        complete = "You have passed every test I can offer."
    }
}
```

Sight ranges must be from 1 through 12 and are measured as Manhattan/taxicab distance: `abs(playerX - trainerX) + abs(playerY - trainerY)`. The encounter radius follows the trainer's current position while they wander.

Generic sprites are divided into `generic-male-1` through `generic-male-10`
and `generic-female-1` through `generic-female-10`. Matching numbers retain
the same outfit palette and accessory theme while using the selected gendered
silhouette. The former unqualified `generic1` through `generic10` names are no
longer accepted.

## Trade-only town NPC template

Town NPCs without Duel do not need battle, deck, reward, or AI fields.

```lua
{
    id = "merchant_id",
    name = "Merchant Name",
    kind = "town_npc",
    options = { trade = true, wander = false },
    appearance = "mercer",
    ai = { personality = "none" },
    dialogue = {
        greeting = "Welcome.",
        shop_early = "Early-story shop dialogue.",
        shop_late = "Later-story shop dialogue.",
        act_complete = "Post-act shop dialogue."
    }
}
```

## Boss template

Bosses use the same deck and reward arrays as battle-enabled town NPCs.
`max_battles` is normally one.

```lua
{
    id = "boss_id",
    name = "Boss Name",
    kind = "boss",
    appearance = "veiled_one",
    max_battles = 1,
    decks = { "Boss.txt" },
    rewards = {
        { card = "Exact Card Name", gold = 250 },
    },
    ai = { personality = "adaptive" },
    dialogue = {
        greeting = "Boss challenge.",
        defeat = "Boss defeat line.",
        victory = "Boss victory line.",
        complete = "Dialogue after the boss has been defeated."
    }
}
```

## Supported values

Kinds:

- `town_npc`
- `route_duelist`
- `boss`

Current appearances:

- `mira`
- `marin`
- `rook`
- `aurelia`
- `flint`
- `nyx`
- `tidal`
- `briar`
- `mercer`
- `veiled_one`

Current AI personalities:

- `balanced` — standard heuristic priorities
- `aggressive` — favors summoning and safe attacks
- `defensive` — favors blockers and shield triggers
- `control` — favors choices, tap abilities, and trigger value
- `tempo` — favors board development and attacks
- `ramp` — places additional value on mana development
- `sacrifice` — slightly favors card plays and mandatory target choices
- `adaptive` — currently uses balanced scoring and is reserved for richer matchup logic
- `none` — for non-duelists

## Adding new appearances

The metadata loader intentionally validates appearance names. Adding a completely new sprite therefore requires:

1. Add the value to `CharacterAppearance` in `Source/App/Npc.h`.
2. Add its string mapping in `Source/App/Npc.cpp`.
3. Draw it in `Application::drawCharacter` in `Source/App/Overworld.cpp`.
4. Use the new string in `Lua/Npcs.lua`.

Dialogue keys are extensible and do not require loader changes. Application code must explicitly request a new key before it affects the UI.
