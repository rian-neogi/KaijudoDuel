# Rootmaze Commons

## Layout purpose

Rootmaze Commons should feel like a settlement that grew with a living forest rather than one imposed on top of it. It is made of several clearings connected by broad roots, flexible bridges, and shallow waterways. The central services remain easy to find, while optional trails and shortcuts can shift during the Rootmaze story.

The town is a required first-tier region containing the Verdant Crest and Nature Resonance Seal. Its Northwater Gate enters the shared crossroads for Glasswater Port and Emberglen; separate routes lead to Cinderrail Foundry and the optional town of Honeyreach.

**Implementation status:** The 96-by-56 surface town is implemented as the
`rootmaze` region of the seamless `overworld` map. Northwater Gate, Waterstep,
Greatroot Common, Heartroot, Verdant Arena, the service and residential
clearings, Southroot Green, and the future Honeyreach and Cinderrail exits are
playable. Major interiors, shifting optional shortcuts, and the Nature Seal
investigation remain future layers of this layout.

## Navigation principles

- The **Greatroot Common** is the central landmark. All major districts loop back to it.
- Main roads and essential services never move. The forest's changing-path theme is expressed through secondary bridges, blocked trails, creature routes, and optional shortcuts.
- Painted leaf symbols identify stable public paths: blue for Glasswater, gold for Emberglen, red for Cinderrail, and amber for Honeyreach.
- Movable wooden signs show current shortcuts. Moss updates them as story events change the forest.
- Streams are shallow visual boundaries crossed by small bridges. They should guide movement without turning the town into a maze of narrow collision corridors.
- Homes use curved root walls, timber frames, canvas awnings, and living roofs. Important buildings have distinct silhouettes and colored banners so players can recognize them without opening a map.

## Schematic layout

```text
                    Watershed Crossroads (Glasswater / Emberglen)
                                       |
                              [Northwater Gate]
                                       |
               [Pump House] -- [Waterstep Market] -- [Reedwright Workshop]
                       \               |
                        \       [Greatroot Common] ------- [Commons Hall]
                         \          /       |      \
                    [Heartroot]  [Card Shop] |    [Wayfarer's Nest]
                         |                   |
                [Nature Sanctuary]     [Homeward Walk]
                         |                  /       \
                [Verdant Arena]     [Canopy Homes]  [Creature House]
                         |                  |               \
                  [Wildgrove Trails] [Southroot Green] -- [Lineage Nursery]
                         |                  |                |
                  Road to Honeyreach   [Forager Huts]  Road to Cinderrail
```

This diagram is schematic rather than tile-accurate. Curved roots and water channels should keep the final map from looking like a rectangular street grid.

## Central district

### Greatroot Common

The town center is a circular clearing enclosed by five enormous roots. A low wooden platform wraps around the oldest tree without cutting into it. Residents use the platform for announcements, casual duels, shared meals, and public votes under the Commons agreement.

The center contains:

- A movable map board maintained by Moss
- A public duel table for non-Crest matches
- Benches grown from trained roots
- A rain canopy that can be unfurled between the trees
- A notice board for trainer challenges, missing creatures, and gathering requests
- Five road markers pointing toward the main regional exits

The player should arrive here on their first visit. Oren greets challengers beside the map board after the introductory creature-path scene.

### Commons Hall

The Commons Hall is the closest Rootmaze has to a town hall. It occupies a hollow beneath a broad living root, with an open front and tiered wooden seating. No mayor works here. Stewardship disputes, route changes, and festival plans are decided through public meetings.

Inside are:

- The stewardship ledger listing which families currently care for each clearing
- A large relief map with replaceable path pieces
- Storage for bridge ropes, emergency food, and creature blankets
- A small records desk used during the Rootmaze story
- A rear meeting room where Oren, Briar, and visiting road wardens discuss displaced creatures

After the Nature Seal is restored, the relief map shows the stable regional loops back to Emberglen and neighboring towns.

### Root and Ripple Card Exchange

The local card shop stands opposite Commons Hall beneath a green-and-blue striped awning. It specializes in Nature and Water cards rather than competing with Mercer's general inventory.

The shop sells:

- Basic mana-acceleration creatures
- Creature-search and race-support cards
- Water draw and bounce support
- Beast Folk and Liquid People evolution pieces
- A small rotating selection imported from Glasswater

Its owner also offers simple deck advice about evolution ratios and civilization access. Mercer remains the better general shop, so the Exchange should provide regional specialization rather than mandatory exclusive staples.

### Wayfarer's Nest

The inn is built across three low roots joined by flexible floors. Travelers sleep in small canvas-and-timber rooms arranged around a communal kitchen. The building rocks slightly when the roots move but is engineered to remain level enough for furniture.

The inn provides:

- A rest point
- A save or future fast-travel marker
- A kitchen where visiting NPCs gather
- A bulletin board for regional side quests
- Temporary rooms used by Rook, Briar, or other Emberglen visitors during Act II

Its sign depicts a bird nest holding five differently colored cards.

## Northern district: Waterstep

### Northwater Gate

The Glasswater road enters through a wide wetland clearing rather than a wall or formal gate. Blue cloth strips and floating markers identify the safe route when the stream rises. A covered checkpoint contains road conditions and a handcart for moving goods.

The northern route enters Watershed Crossroads, where clearly marked branches continue to Glasswater Port and Emberglen. This is Rootmaze's only portal into that shared map. An optional branch near the gate reaches a fishing platform and a hidden item clearing, but required travel stays obvious.

### Waterstep Market

Waterstep is a morning market arranged on raised platforms over a shallow stream. Glasswater tools, Lilyreed fish, Rootmaze herbs, and Honeyreach fruit circulate here before traveling toward Emberglen.

Permanent stalls include:

- A produce and herb stall
- A flexible-rope merchant
- A creature-feed vendor
- A Glasswater card trader
- A small food counter serving root cakes and river tea

The stall frames remain in place, but colored awnings and goods change as trade routes reopen.

### Glasswater Pump House

This blue-roofed building manages the flexible channels designed with Glasswater engineers. Water wheels raise clean water into storage tanks while overflow runs through garden beds.

The Pump House contains valves used during one environmental quest. Restoring different channels can open optional stepping-stone paths without disabling access to any shop or required building.

### Reedwright Workshop

The workshop produces flexible pipes, woven bridge surfaces, baskets, and waterproof card cases. Its exterior is marked by long bundles of reeds hung beneath a curved roof.

The reedwright can repair a damaged bridge during the main Rootmaze story and later sell cosmetic green-and-blue card cases. A rear yard provides a recognizable home for one Water-focused local duelist.

## Eastern district: Homeward Walk

### Homeward Walk

Homeward Walk is the town's broadest and most stable internal road. Gold leaf markers line a packed-earth path suitable for Mercer's supply carts. It curves back through Greatroot Common to Northwater Gate, where the single external portal enters Watershed Crossroads and its Emberglen branch.

A carved sign shows the distance to Emberglen and reminds travelers that all roads through Rootmaze are shared paths. The walk and Northwater Gate remain accessible after the player's first arrival, allowing immediate backtracking to Mercer without creating a second Rootmaze connector portal.

### Creature Rest House

The Creature Rest House is an open-sided clinic and shelter rather than a conventional stable. It contains pools, nesting boxes, shaded soil beds, perches, and quiet card-storage cabinets. Different spaces accommodate creatures from several civilizations without presenting them as ordinary livestock.

The building is staffed by local caretakers, with Briar assisting during the displaced-echo storyline. Its functions include:

- The main reunion scenes between creatures and bonded duelists
- Creature-care side quests
- A healer or rest service
- Dialogue explaining race and evolution relationships
- Visible occupants that change as the Rootmaze conflict is resolved

### Lineage Nursery

The nursery is a training garden for duelists learning how base creatures relate to evolution creatures. Wooden family trees display card races and evolution lines without suggesting that the physical cards are being bred.

Practice circles allow tutorial or challenge duels built around evolving a creature, preserving evolution material, or choosing between two possible evolution plans. Toma sometimes brings Beetles here, while Oren uses it for advanced lessons after the Crest match.

### Roadroot Storehouses

Several low warehouses hold lumber, dried herbs, bridge pieces, and trade goods bound for Emberglen or Cinderrail. Their roofs are covered with soil so the grove continues across them.

One storehouse becomes temporarily inaccessible when frightened creatures redirect a root during the story. Reopening it creates a useful shortcut between Homeward Walk and Southroot Green.

## Residential clearings

### Canopy House — Oren's home

Oren lives in a modest elevated home supported between three trees. It has no formal office or trophy hall. The largest room contains route sketches, creature-care notes, mismatched cups, and several unfinished conversations written on hanging scraps of bark-paper.

A lower outdoor table serves as Oren's public meeting place. Before the Verdant trial, the player finds Oren here when they are not responding to a creature disturbance. After the Crest match, the house becomes the location for rematch dialogue and regional advice.

### Fern's Forager Hut

Fern's small hut stands beside drying racks, berry baskets, and labeled herb beds. A roof vent releases the smell of whatever plant is currently being tested.

The hut acts as the starting point for gathering quests. A trail behind it leads into the Wildgrove and changes as optional paths open.

### Toma's Beetle House

Toma maintains a long, low home with screened windows and sheltered insect enclosures. Wooden Beetle carvings make it visually distinct from the other residences.

The rear enclosure supports creature-care dialogue and a side quest about relocating a frightened Beetle family. Toma can be challenged in the yard without blocking the residential path.

### Moss's Path Warden Lodge

Moss lives in a narrow lodge surrounded by stacks of signs, bridge hooks, and painted trail markers. Half the signs point nowhere because the paths they described have moved.

The lodge is the player's source for current route information. During the main story, Moss updates the map after each reunited creature. After restoration, the lodge can provide hints for undiscovered clearings and optional collectibles.

### Steward homes

Several smaller family homes form loose rings around shared gardens. They should not all look identical: some sit inside hollow roots, some use timber frames, and others stand on short platforms above water.

At least two homes should be enterable. Suggested interiors include:

- A communal kitchen and seed pantry
- A family room displaying an inherited Beast Folk deck
- A small weaving workshop
- A child's room with hand-drawn creature maps

Other houses can remain exterior-only while still showing laundry, tools, gardens, and residents performing daily routines.

## Southern district: Southroot Green

### Southroot Green

This broad clearing is the town's agricultural and social lawn. Community gardens occupy the sunny edges, while the center remains open for markets, creature exercise, and seasonal festivals.

Honeyreach traders use the southern side of the Green. During its harvest season, amber banners and fruit carts make the optional-town route especially visible.

### Honeyreach road

The road to Honeyreach begins beneath an arch of flowering roots marked with amber ribbons. It is always optional and never lies between the player and the Nature Seal or Verdant Arena.

A small covered rest platform at the exit can host a Honeyreach merchant before the player visits that town, quietly advertising the side route without turning it into a quest requirement.

### Forager huts and seed sheds

Small work huts store tools, seeds, drying racks, and emergency supplies. These buildings make the town's economy visible and provide locations for minor NPCs rather than existing only as decoration.

One locked seed shed may open after the player completes Fern's gathering quest, revealing a card reward or cosmetic item.

## Western district: Heartroot Sanctuary

### Heartroot Spring

The oldest living root forms an arch over a clear spring west of town. The five main Rootmaze streams begin here, though only the Nature resonance is concentrated in the sanctuary itself.

The spring clearing should feel peaceful rather than grand. Flat stones, hanging seed charms, and quiet creature nests distinguish it from the busier central district.

### Nature Resonance Sanctuary

The Nature Seal rests inside a chamber formed naturally beneath Heartroot. Residents added a light wooden walkway and observation platform but did not carve the root into a temple.

During the Rootmaze crisis:

- Frightened creature echoes cause roots to block the direct approach
- Three alternate trails open as creatures are reunited
- False memories appear as mismatched tracks and displaced nesting signs
- The sanctuary becomes reachable without requiring the Verdant Crest duel first

After restoration, the chamber remains open for story dialogue and optional visual changes during Act III.

### Verdant Arena

Oren's official arena is an open meadow beyond the sanctuary. Spectators sit on terraced roots, and shallow water channels mark the duel boundary. No permanent walls separate summoned creatures from the surrounding grove.

The arena complex includes:

- A registration shelter
- Two preparation platforms
- A public deck-check table
- Terraced spectator seating
- A carved post displaying the Verdant Crest symbol
- A rear path to Oren's optional master-rematch clearing

The player may visit the arena early, but the official challenge begins only after completing Oren's route trial.

### Wildgrove trails

The Wildgrove is a set of optional forest paths surrounding the sanctuary and arena. It contains gathering spots, creature encounters, trainer clearings, and shortcuts back toward Fern's hut or Southroot Green.

These paths may change during story stages. None should trap the player or remove access to a required service. A stable outer path always returns to the Verdant Arena entrance.

## Cinderrail connection

The Cinderrail road leaves southeast of the Creature Rest House and Lineage Nursery. Red markers appear gradually among the green roots as the path approaches the industrial region.

A timber loading platform near the exit transfers herbs, flexible lumber, and creature-care supplies onto Cinderrail carts. Fire-resistant water barrels and metal bridge braces show the practical exchange between the two towns.

This road is bidirectional and remains available after opening. It creates a regional loop rather than forcing every return trip through Emberglen.

## Suggested NPC placement

- **Oren:** Greatroot Common initially; Canopy House or Verdant Arena after story milestones
- **Fern:** Outside the Forager Hut, later on Wildgrove gathering paths
- **Toma:** Beetle House yard or Lineage Nursery
- **Moss:** Greatroot map board, Path Warden Lodge, or beside a newly changed route
- **Briar:** Creature Rest House during the Rootmaze story
- **Rook:** Damaged bridge or Cinderrail loading platform during his visit
- **Card merchant:** Root and Ripple Card Exchange
- **Innkeeper:** Wayfarer's Nest kitchen or entrance
- **Minor residents:** Waterstep stalls, Southroot gardens, shared home clearings, and sanctuary approach

NPCs who challenge on sight should patrol open clearings rather than narrow bridges or shop entrances.

## Story-state changes

### Before the Rootmaze conflict

- Main public routes are stable
- Creature Rest House has only a few ordinary occupants
- Heartroot Spring is clearly visible from the western path
- Trade stalls operate normally

### During the false-memory crisis

- Secondary roots block several shortcuts
- Movable signs disagree with one another
- Displaced creatures occupy the Rest House
- Some market stalls close while residents search the grove
- The sanctuary approach divides into three creature-related routes

### After restoring the Nature Seal

- Stable loops connect every major district
- Reunited creatures return to their usual clearings
- Waterstep and Roadroot trade inventories expand
- Oren's Crest trial and advanced rematches become available
- A direct regional shortcut to Cinderrail opens
- Honeyreach festival decorations may appear near the southern exit

### During Act III

- Some leaf colors temporarily drain near blank zones
- Residents post hand-written directions when enchanted signs lose their text
- Essential shops, the inn, and all regional exits remain usable
- Restored optional quests add color and creatures back to affected clearings

## Implementation notes

- Use a stable central loop for Greatroot Common, Commons Hall, the shop, inn, and regional exits.
- Reserve shifting routes for quest branches and shortcuts; do not randomly move collision geometry during ordinary traversal.
- Distinguish important structures with roof shape, banners, props, and surrounding terrain rather than large floating labels.
- Keep bridges at least two tiles wide where NPCs may move, preventing the player and wandering duelists from blocking one another.
- Provide one non-shop route between every pair of major districts so crowds cannot trap the player.
- The map may be larger than the 20-by-12 viewport, with Greatroot Common acting as the visual and navigational center.
- Outdoor tiles should use grass, path, water, trees, dense forest, and wooden-building assets consistently with the conventions in `AGENTS.md`.
