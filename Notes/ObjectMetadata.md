# Overworld Object Metadata

Interactive overworld objects and editor object templates are defined in
`Lua/Objects.lua`. Their positions belong exclusively to the `objects` array in
`World/World.json`, so the World Builder can place and move them without
rewriting hand-authored interaction text.

## Signpost template

```lua
{
    id = "unique_sign_id",
    name = "Display Name",
    kind = "signpost",
    text = "The directions or message shown when the player reads the sign."
}
```

IDs must be unique, and every field is required. A signpost occupies its map
tile: the player and wandering NPCs cannot walk through it. Face the sign and
press `E`, Space, or Enter to read it.

## Deck chest

```lua
{
    id = "unique_chest_id",
	name = "Weathered Chest",
	kind = "deck_chest",
	appearance = "!Chest-1",
	reward = {
		kind = "deck",
		deck = "Treasure/Example.txt",
		name = "Display Name in Deck Builder"
	},
	text = "Shown when the deck is discovered.",
	opened_text = "Shown on later interactions."
}
```

The appearance uses `<character-sheet>-<one-based index>` and is rendered from
`Resources/Graphics/Characters/!Chest.png`. Deck paths are searched beneath
`Decks/`. Opening a chest adds every card in the reward file to the current
save's collection and creates a usable copy under that save's
`PlayerData/<save>/Decks` folder. Each chest can be claimed once per save; its
opened state is stored in `progress.txt`.

## World Builder templates

The global `WorldObjectTemplates` array supplies reusable prototypes for the
World Builder. Templates support `signpost`, `chest`, `cuttable_bush`,
`smashable_rock`, and `environment` kinds. A template needs a stable `id`, a
display `name`, interaction `text`, and a `kind`. Chest and environment objects
also use an `appearance`; chests use `opened_text` after their first interaction
and remember that state per save. Animated character-sheet fixtures can select a
direction row with `frame_row` from `0` through `3` and enable frame animation
with `animated = true`.

The supplied environment palette exposes all character slots and direction
rows in `!Flame.png`, `!Other1.png`, `!Other2.png`, `!Other3.png`,
`!Crystal.png`, `!Hexagram.png`, `!Switch1.png`, and `!Switch2.png`. Tall frames
are bottom-aligned to their occupied map tile rather than stretched square.

Cuttable bushes require at least one `Xeno Mantis` in the current save's
collection. Smashable rocks require `Smash Warrior Stagrandu`, the exact card
name used by the Lua card database. Once cleared, either obstacle becomes
walkable and its `object.cleared.<id>` state is persisted in `progress.txt`.

The World Builder's Objects tab combines interactive objects from
`Lua/Objects.lua` with collectible shards from `Lua/MercerStock.lua`. Use `Add`
to select a reusable template and click an empty walkable map tile to create an
instance. Use `Placed` to select or move an existing object; double-clicking an
entry pans the camera to it. Delete or Backspace removes only instances created
through the builder, and Ctrl+Z restores additions, removals, and moves.

Saving writes created instance IDs and template references to
`entities.object_definitions` in `World/World.json`, alongside their ordinary
entries in `entities.objects`. It never edits `Lua/Objects.lua`.
