# NPC Metadata

Overworld NPCs are defined in `Lua/Npcs.lua`. The file returns an array with one table per NPC and is loaded before SDL creates the game window.

The loader rejects invalid metadata with a specific startup error. It validates duplicate IDs and names, kinds, sprite appearances, positions, deck files, reward cards, reward limits, and empty rosters.

## Duelist template

```lua
{
    id = "unique_id",
    name = "Display Name",
    kind = "duelist",
    position = { x = 1, y = 1 },
    appearance = "mira",

    max_battles = 4,

    deck1 = "Example.txt",
    deck2 = "ExampleAdvanced.txt", -- optional
    -- deck2 is reused for battles 3 and 4 when deck3/deck4 are omitted.

    reward1 = { card = "First Reward Card", gold = 100 },
    reward2 = { card = "Second Reward Card", gold = 125 },
    reward3 = { card = "Third Reward Card", gold = 150 },
    reward4 = { card = "Fourth Reward Card", gold = 200 },

    ai = {
        personality = "balanced"
    },

    dialogue = {
        greeting = "Ready to duel?",
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

### Battle sequence

`max_battles` can be between one and four. `deck1` is required. Later deck
fields are optional and carry forward: if `deck2` is defined but `deck3` and
`deck4` are omitted, battles three and four also use `deck2`. Each enabled
battle requires its matching `rewardN` table. Rewards do not carry forward,
preventing an omitted reward from silently granting the wrong card.

Deck names are searched beneath `Decks/` automatically. An explicit path can
still be used for a deck elsewhere; if that path exists, it takes priority.

## Shopkeeper template

Shopkeepers do not need battle, deck, or reward fields.

```lua
{
    id = "merchant_id",
    name = "Merchant Name",
    kind = "shopkeeper",
    position = { x = 1, y = 1 },
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

Bosses use the same numbered deck and reward fields as duelists.
`max_battles` is normally one.

```lua
{
    id = "boss_id",
    name = "Boss Name",
    kind = "boss",
    position = { x = 10, y = 4 },
    appearance = "veiled_one",
    max_battles = 1,
    deck1 = "Boss.txt",
    reward1 = { card = "Exact Card Name", gold = 250 },
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

- `duelist`
- `shopkeeper`
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
