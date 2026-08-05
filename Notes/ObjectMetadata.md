# Overworld Object Metadata

Interactive overworld objects are defined in `Lua/Objects.lua`. Their positions
belong exclusively to the `objects` array in `World/World.json`, so the World
Builder can move them without rewriting hand-authored interaction text.

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

## Abandoned deck chest template

```lua
{
    id = "unique_chest_id",
    name = "Weathered Chest",
    kind = "deck_chest",
    deck = "Treasure/Example.txt",
    deck_name = "Display Name in Deck Builder",
    text = "Shown when the deck is discovered.",
    opened_text = "Shown on later interactions."
}
```

Deck paths are searched beneath `Decks/`. Opening a chest adds every card in
the file to the current save's collection and creates a usable copy of the deck
under that save's `PlayerData/<save>/Decks` folder. Each chest can be claimed
once per save; its opened state is stored in `progress.txt`.

The World Builder's Objects tab combines interactive objects from
`Lua/Objects.lua` with collectible shards from `Lua/MercerStock.lua`. Selecting
an entry allows it to be placed on an empty walkable tile; double-clicking an
entry pans the camera to its current position. Saving modifies only positions
in `World/World.json`.
