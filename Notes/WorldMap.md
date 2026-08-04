# World Map

This is the planned campaign topology rather than a literal scale map. `Lua/World.lua` remains authoritative for maps that are actually implemented.

The campaign uses one connected overworld. “First tier” and “second tier” describe progression availability, not separate world maps. Every ordinary road is bidirectional, unlocked roads stay open permanently, and the player may return from the outer regions to Emberglen at any time.

Emberglen sits near the center because it remains the campaign's practical home base. Mercer has the broadest general shop there, his stock expands as regional routes reopen, and several story characters return there between major events.

An asterisk marks an optional town. Optional towns are short offshoots: the player never has to pass through one to reach a Crest, Resonance Seal, Hollow objective, or later region.

## Coherent world layout

```text
                                            Crown Gate
                                                |
                    Ribbonfair* --------- Mirror Arena -------- Sunspire Cloister
                         |                    /     \                    |
                         +-------------------/       \-------------------+
                                             \       /
                 Reedwake*                 Confluence Arena -------- Hollow Archive
                     |                      /     |    \
              Glasswater Port -------------/      |     \------------- Stormbreak Plateau -- Cloudrest*
                    /  \                          |
                   /    \                         |
        Honeyreach* -- Rootmaze Commons ----- Emberglen ----- Cinderrail Foundry -- Clayhearth*
                            \                    |                    /
                             \                   |                   /
                              +------------- Gloam Quarry ----------+
                                                |
                                           Lanternfen*
```

The diagram is schematic. Exact terrain may bend these roads, but the connections and ability to backtrack should remain. A triangle in this destination-only view can represent one shared three-exit region rather than three separate roads.

## Shared three-way connectors

| Connector region | Exits |
| --- | --- |
| Watershed Crossroads | Emberglen, Glasswater Port, Rootmaze Commons |
| Ribbonway Crossroads | Confluence Arena, Mirror Arena, Ribbonfair |
| Sunmirror Causeway | Confluence Arena, Mirror Arena, Sunspire Cloister |

Each row is one playable connecting area with three seamless exits. It is not shorthand for three pairwise connector regions. Ribbonway Crossroads and Sunmirror Causeway deliberately provide two different routes between Confluence and Mirror: the former is the lively Ribbonfair-side route, while the latter is the warded Sunspire-side route.

## Road network

The adjacency list is authoritative for the planned topology when the ASCII spacing is ambiguous. “Direct” means reachable through one connecting area without crossing another destination; several direct links may therefore share the same three-exit connector. Each listed connection works in both directions after its gate is unlocked.

| Destination | Direct road connections |
| --- | --- |
| Emberglen | Glasswater Port, Rootmaze Commons, Cinderrail Foundry, Gloam Quarry, Confluence Arena |
| Glasswater Port | Emberglen, Rootmaze Commons, Reedwake |
| Rootmaze Commons | Emberglen, Glasswater Port, Cinderrail Foundry, Honeyreach |
| Cinderrail Foundry | Emberglen, Rootmaze Commons, Gloam Quarry, Clayhearth |
| Confluence Arena | Emberglen, Mirror Arena, Stormbreak Plateau, Sunspire Cloister, Ribbonfair, Hollow Archive |
| Mirror Arena | Confluence Arena, Stormbreak Plateau, Sunspire Cloister, Ribbonfair, Crown Gate |
| Stormbreak Plateau | Confluence Arena, Mirror Arena, Cloudrest |
| Gloam Quarry | Emberglen, Cinderrail Foundry, Lanternfen |
| Sunspire Cloister | Confluence Arena, Mirror Arena |
| Crown Gate | Mirror Arena |
| Hollow Archive | Confluence Arena |
| Reedwake | Glasswater Port |
| Honeyreach | Rootmaze Commons |
| Clayhearth | Cinderrail Foundry |
| Cloudrest | Stormbreak Plateau |
| Lanternfen | Gloam Quarry |
| Ribbonfair | Confluence Arena, Mirror Arena |

These cross-links create loops around Emberglen and Confluence. A player leaving Stormbreak can travel directly to Mirror, return through Confluence, or continue all the way back to Emberglen without using a menu or one-way transition. Gloam is a separate southern branch reached through Emberglen or Cinderrail.

## Progression gates

### Act I and Crest 1

Emberglen contains the story setup and Dawn Crest. The nearby roads are initially unsafe during the Fading. The post-Crest Hollow scene restores access to Glasswater, Rootmaze, and Cinderrail.

### Crests 2–4

Glasswater, Rootmaze, and Cinderrail may be completed in any order. Their local roads form a loop around Emberglen, and the player may return to Mercer between any two objectives.

Reedwake, Honeyreach, and Clayhearth open with their neighboring routes but remain optional.

### Crest 5 and the outer-road gate

Confluence is physically connected to Emberglen from the beginning, but its relay checkpoint blocks travel into the outer regions. After the player holds the Tidal, Verdant, and Forge Crests and completes the midpoint Hollow scene, Kestrel offers the Confluence match.

Winning restores the Confluence relay and permanently opens the outer network toward Stormbreak, Gloam Quarry, Mirror Arena, and Sunspire. Gloam is reached through Emberglen or Cinderrail rather than by a direct road from Confluence or Stormbreak. Opening the network does not close or replace any earlier road.

### Crests 6–8

Stormbreak, Gloam Quarry, and Mirror Arena may be completed in any order. Stormbreak and Mirror have direct cross-links to Confluence and each other, while Gloam branches from Emberglen and Cinderrail. The persistent central roads keep all three objectives accessible without imposing an order.

Cloudrest, Lanternfen, and Ribbonfair open with their neighboring routes but remain optional. Sunspire is a required Hollow-story region with no Crest.

### Hollow finale, Crest 9, and Championship

The Hollow Archive entrance branches from Confluence. After the Hollow finale, the player can freely leave the restored Archive, return anywhere in the world, prepare at Mercer's shop, and then travel to Crown Gate through Mirror Arena.

Caelum's Unity match is the final Crest objective. Crown Gate remains connected afterward, and the Grand Championship does not remove access to the rest of the overworld.

## Destination key

| Destination | Role | Required for main progression |
| --- | --- | --- |
| Emberglen | Central hub, Mercer, Act I, and Dawn Crest | Yes |
| Glasswater Port | Tidal Crest and Water Seal | Yes, order-independent |
| Cinderrail Foundry | Forge Crest and Fire Seal | Yes, order-independent |
| Rootmaze Commons | Verdant Crest and Nature Seal | Yes, order-independent |
| Reedwake | Fishing and ferry side town | No |
| Honeyreach | Orchard festival side town | No |
| Clayhearth | Pottery and cosmetics side town | No |
| Confluence Arena | Fifth Crest and permanent outer-road gate | Yes |
| Stormbreak Plateau | Tempest Crest | Yes, order-independent |
| Gloam Quarry | Ashen Crest and Darkness Seal | Yes, order-independent |
| Mirror Arena | Mirror Crest | Yes, order-independent |
| Sunspire Cloister | Light Seal, no Crest | Yes for Hollow story |
| Cloudrest | Hot-spring and courier side town | No |
| Lanternfen | Wetland and graveyard-strategy side town | No |
| Ribbonfair | Carnival and rules-challenge side town | No |
| Hollow Archive | Hollow-story finale | Yes |
| Crown Gate | Unity Crest after Hollow finale | Yes |
| Grand Championship | Sporting finale | Yes |

Required-town histories are in `Notes/TownBackstories.md`. Detailed optional-town concepts are in `Notes/OptionalTowns.md`. Building plans for Cinderrail, Glasswater, Rootmaze, and Gloam Quarry are in their matching files under `Notes/`. The playable routes between destinations are catalogued in `Notes/ConnectingAreas.md`.
