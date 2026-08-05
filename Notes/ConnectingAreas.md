# Connecting Areas

## Purpose

Connecting areas are playable routes between settlements, arenas, sanctuaries, and major story locations. They make the campaign feel like one continuous world instead of a collection of destinations selected from a menu.

Each connector in this document states exactly which destinations it joins. Most have two exits, while shared junction regions have three. Unless a profile explicitly says otherwise, travel between every pair of exits is bidirectional after its progression gate opens and remains open for the rest of the game.

`Notes/WorldMap.md` remains authoritative for planned regional adjacency. `World/World.json` and `World/Maps/*.json` remain authoritative for connectors that have actually been implemented.

Names attached to planned connectors are working region names. Their exits and bidirectional topology are more important than preserving a particular road name during implementation.

## Status terms

- **Implemented:** The connector is a traversable region of the seamless native exterior.
- **Planned:** The connector belongs to the agreed world topology but does not yet have a playable exterior region.
- **Locked:** The physical route may be visible, but story progression prevents crossing its outer checkpoint.
- **Open:** The player may traverse the route in both directions.

Connecting areas are not towns. They may contain trainers, collectibles, shelters, overlooks, and short side encounters, but should not contain a Circuit Crest, Resonance Seal, irreplaceable shop, or other service that forces repeated travel through a long route.

## Scale and adventure standard

Connecting areas are destinations in their own right, not narrow loading corridors. The implemented routes use roughly twice the width and twice the height of their original layouts, producing four times the explorable tile area. New connectors should use this expedition scale as their baseline unless geography gives them a good reason to be even larger.

Every connector should provide a legible main route for repeat travel and enough uncertainty to make the first crossing feel like an adventure. Aim for at least two optional loops, two or more route duelists, remote shard or treasure sites, a shelter or campsite, and one environmental story told through terrain. Bridges, ridges, ruins, wetlands, or route machinery should create meaningful subareas. A player following the main markers must never become trapped, while a player who leaves the road should find something worth the detour.

The unknown comes from exploration and fixed discoveries rather than mandatory random battles. Once a player learns a route, its landmarks and shortcuts should make the return trip quicker and more confident.

Walking near a named landmark records it permanently in the player's route discoveries and grants a small one-time gold reward. The Esc menu shows found landmarks by name and leaves undiscovered sites hidden. These rewards are intentionally modest: they make curiosity worthwhile without requiring players to scour every tile before continuing the story.

## Route overview

| Connecting area | Connects | Availability | Status |
| --- | --- | --- | --- |
| Old Road | Emberglen ↔ Cinderrail Foundry | After Act I | Implemented |
| Watershed Crossroads | Emberglen ↔ Glasswater Port ↔ Rootmaze Commons | After Act I | Implemented |
| Blackstone Road | Emberglen ↔ Gloam Quarry | After Confluence relay | Implemented |
| Fivefold Highway | Emberglen ↔ Dragon Keep | Midpoint route | Planned |
| Lilyreed Ferry | Glasswater Port ↔ Lilyreed Village | First optional tier | Planned |
| Rootrail Greenway | Rootmaze Commons ↔ Cinderrail Foundry | First open-world tier | Planned |
| Honeyroot Trail | Rootmaze Commons ↔ Honeyreach | First optional tier | Planned |
| The Lowlands | Cinderrail Foundry ↔ Gloam Quarry | After Confluence relay | Planned |
| Offshoot Path | Cinderrail Foundry ↔ Clayhearth | First optional tier | Planned |
| Western Crossroads | Dragon Keep ↔ Mirror Arena ↔ Ribbonfair | After Confluence relay | Planned |
| Courier Rise | Dragon Keep ↔ Stormbreak Plateau | After Confluence relay | Planned |
| Eastern Crossroads | Dragon Keep ↔ Mirror Arena ↔ Sunspire Cloister | After Confluence relay | Planned |
| Archive Approach | Dragon Keep ↔ Hollow Archive | Hollow finale | Planned |
| Prism Pass | Mirror Arena ↔ Stormbreak Plateau | Second open-world tier | Planned |
| Path to Crown Gate | Mirror Arena ↔ Crown Gate | After Hollow finale | Planned |
| Springline Trail | Stormbreak Plateau ↔ Cloudrest Peak | Second optional tier | Planned |
| Lantern Steps | Gloam Quarry ↔ Lanternfen | Second optional tier | Planned |

## Central routes from Emberglen

### Old Road

**Connects:** Emberglen's eastern exit ↔ Cinderrail Foundry's western station road  
**Availability:** Opens after Act I  
**Status:** Implemented as the `old_road` region of the seamless native `overworld` map

The Old Road is a former primary freight route whose traffic declined as newer links opened. It gradually changes from Emberglen woodland into the rocky ridge surrounding Cinderrail. Its expanded 96-by-48 map includes a winding cobbled path, several stream crossings, timber bridges, an old maintenance checkpoint, waystones, forest banks, exposed stone, and a broken freight cut.

Rook maintains the western bridge and checkpoint. His position gives the connector a familiar character without turning it into another settlement. Beyond him, northern and southern loops leave the maintained road for abandoned works, rockfall country, and Wayfarer Camp. Cairn challenges explorers near the abandoned cut, where a weathered freight chest contains the complete Fire/Nature Wayfarer's Cache deck. Mara Flintway watches the remote camp and eastern loop. Four civilization shards sit well away from the shortest crossing. The road's western gate returns to Emberglen, while its eastern gate enters Cinderrail near the gold-marked arrival road.

The route discoveries are the Abandoned Freight Cut, Rook's Checkpoint, and Wayfarer Camp.

The Emberglen entrance remains locked until the Act I investigation and boss confrontation are complete. Returning from Cinderrail or the Old Road toward Emberglen is never restricted after the route opens.

### Watershed Crossroads

**Connects:** Emberglen's western road ↔ Glasswater Port's Watershed Gate ↔ Rootmaze Commons' Northwater Gate  
**Availability:** Opens after Act I  
**Status:** Implemented as the `watershed_crossroads` region of the seamless native `overworld` map

Watershed Crossroads is one continuous three-way region rather than three separate roads. Its central landmark is an old toll shelter beside the point where Emberglen's drainage stream divides between Glasswater's canals and Rootmaze's root-supported wetlands. Gold orchard markers identify Emberglen, blue tide markers identify Glasswater, and green leaf signs identify Rootmaze.

The expanded 128-by-72 region sits directly west of Emberglen. The Emberglen branch passes through orchards and a caravan pull-off. The Glasswater branch crosses broad canal bridges beneath a lighthouse silhouette and joins the city's Watershed Gate directly, while the Rootmaze branch follows shallow streams beneath living roots into Northwater Gate. Between them lie a northern pool trail, a southern marsh loop, drowned foundations, and a creature shelter around the old toll house.

Tern Ford patrols the unpredictable western crossing and Vale Reed surveys the southern pools. Four civilization shards reward leaving the route markers and exploring the remote corners. The central shelter and colored signs always provide a reliable way to reorient, so the wetland feels unknown on the first journey without becoming tedious on later crossings. This single region replaces Tideglass Road, Homeward Walk, and Northwater Greenway.

The route discoveries are Northwater Pools, the Old Toll Shelter, and the Drowned Foundations.

### Blackstone Road

**Connects:** Emberglen's southern road ↔ Gloam Quarry's western caravan yard  
**Availability:** Opens after the Confluence relay restores the outer tier  
**Status:** Implemented as the `blackstone_road` region of the seamless native `overworld` map

Blackstone Road is the longest direct route back from Gloam to the central hub. Orchard walls and timber bridges give way to dark retaining blocks, carved mileage records, and gold lantern cages maintained jointly by both towns.

The 96-by-96 route contains a sheltered midpoint camp, three optional exploration loops, an abandoned stone-loading platform, and an overlook toward the quarry. Its northern relay gate is physically closed at first, with a checkpoint sign explaining the closure. Earning the Confluence Crest restores the relay and opens the gate permanently.

### Fivefold Highway

**Connects:** Emberglen's northern trunk road ↔ Dragon Keep's southern relay gate

**Availability:** Reachable after Act I; its outer checkpoint advances at the campaign midpoint  
**Status:** Planned

The Fivefold Highway is the broadest central connector and the main supply road between Mercer's hub and Dragon Keep. Five narrow route bands appear and separate as side roads branch toward other regions.

The map should include milestone plazas, a caravan shelter, relay lines, and a visible but initially closed outer checkpoint. Travel between Emberglen and Dragon Keep remains available while the checkpoint beyond the Keep controls access to the second open-world tier.

## First-tier regional loop

### The Lowlands

**Connects:** Cinderrail Foundry's southern First Kiln road ↔ Gloam Quarry's eastern freight switchback  
**Availability:** Opens after the Confluence relay restores the outer tier  
**Status:** Planned

Kiln Descent follows the supply route used to move furnace-lining stone from Gloam to Cinderrail. Industrial brick and orange safety lamps gradually give way to black retaining walls and violet memorial lanterns.

The connector should contain freight-cart background lanes, a sealed early-game checkpoint, and a broad quarry overlook. Player paths remain separate from carts and never depend on a lift cycle. Once opened, this link creates a permanent shortcut between the first and second regional loops.

## Optional first-tier branches

### Lilyreed Ferry

**Connects:** Glasswater Port's western ferry pier ↔ Lilyreed Village's eastern landing

**Availability:** First optional tier  
**Status:** Planned

The ferry is a water connector rather than a teleport menu. The player walks onto a small vessel, sees a short lake crossing, and arrives at the opposite dock. Both landings remain available without a fare after the route opens.

The playable segment may include a deckhand trainer, floating cargo, distant reed islands, and a brief weather variation. No random delay or missed schedule should prevent immediate return travel.

### Honeyroot Trail

**Connects:** Rootmaze Commons' southern wildgrove road ↔ Honeyreach's northern orchard gate  
**Availability:** First optional tier  
**Status:** Planned

The trail begins beneath flowering roots and becomes a warm orchard lane lined with amber ribbons. Creature tracks, pollinator shelters, and fruit carts communicate the transition without introducing a mandatory story objective.

Two looping footpaths can provide gathering spots and optional duels, but both must return to the main trail before reaching either portal.

### Offshoot Path

**Connects:** Cinderrail Foundry's eastern courier road ↔ Clayhearth's western kiln gate  
**Availability:** First optional tier  
**Status:** Planned

Offshoot Path carries cooled ceramic material, cookware, pigments, and furnace components. White chimney symbols and stacks of safe refractory tiles distinguish it from Cinderrail's active industrial routes.

The route should include a wagon shelter, clay banks, a cooling yard, and distant kiln smoke. It remains an optional branch and never lies between the player and the Forge Crest or Fire Seal.

## Outer network from Dragon Keep

### Western Crossroads

**Connects:** Dragon Keep's northwestern relay road ↔ Mirror Arena's western practice district ↔ Ribbonfair's main caravan entrance

**Availability:** Opens after the Confluence relay  
**Status:** Planned

Ribbonway Crossroads is a three-exit challenger and entertainment route. Teal relay markers lead toward Dragon Keep, silver reflective signs lead toward Mirror, and painted ribbons lead toward Ribbonfair. A central deck-testing shelter serves as the unmistakable junction landmark.

The Dragon Keep and Mirror branches are disciplined, heavily traveled roads with tactical practice boards. The Ribbonfair branch gradually adds painted wagon tracks, paper streamers, and joke arrows, while a reliable line of road studs preserves clear navigation. Portable stalls may change between visits, but all three portal approaches remain visible. This map replaces Mirrorway and Ribbon Road.

### Courier Rise

**Connects:** Dragon Keep's northeastern courier gate ↔ Stormbreak Plateau's southwestern shelf

**Availability:** Opens after the Confluence relay  
**Status:** Planned

Courier Rise climbs from mixed caravan country into exposed highland. Relay posts, wind flags, rope handrails, and stone shelters become more frequent with elevation.

The route should use switchbacks rather than narrow cliff ledges. Weather may alter flags and background visibility, but the path and portal positions remain stable. A midway signal hut provides shelter and optional courier dialogue.

### Eastern Crossroads

**Connects:** Dragon Keep's eastern ward road ↔ Mirror Arena's northeastern terrace ↔ Sunspire Cloister's lower gate

**Availability:** Opens after the Confluence relay  
**Status:** Planned

Sunmirror Causeway is a three-exit scholarly and warded road. It rises from Dragon Keep across pale stone bridges to a high junction, then divides toward Mirror's analytical terraces and Sunspire's lower gate. Teal relay bands, silver calibration plates, and gold bell markers clearly distinguish the three directions.

The central junction contains shaded reading shelters and calibrated mirrors used jointly by both destinations. The road is required for the Hollow storyline but contains no Crest checkpoint. Defensive wards may appear visually unstable during the Light Seal crisis, while clearly marked safe lanes remain passable between every pair of exits. This map replaces Warden's Causeway and Sunmirror Walk.

### Archive Approach

**Connects:** Dragon Keep's sealed archive branch ↔ Hollow Archive's exterior gate

**Availability:** Opens for the Hollow finale after the required Crests and Resonance Seals  
**Status:** Planned

The Archive Approach follows an ancient road whose milestones have lost names but retain tool marks and repair layers. Regional architecture gradually disappears until only old retaining walls and blank archive pylons remain.

This connector should feel consequential without trapping the player. A return portal remains available before and after the finale, allowing preparation at Emberglen and post-story exploration. Hollow replicas may appear as fixed encounters rather than random interruptions.

## Outer regional loop

### Prism Pass

**Connects:** Mirror Arena's eastern high road ↔ Stormbreak Plateau's western shelf  
**Availability:** Second open-world tier  
**Status:** Planned

Prism Pass crosses a bright, windy ridge where Mirror's reflective markers double as Stormbreak signal plates. Split paths expose different views but rejoin before either settlement.

The route may contain an analyst-versus-courier side duel and rotating light effects. Glare must never obscure path edges, characters, or interaction prompts.

## Optional outer branches

### Springline Trail

**Connects:** Stormbreak Plateau's upper shelter road ↔ Cloudrest's lower bathhouse gate  
**Availability:** Second optional tier  
**Status:** Planned

The trail follows warm runoff from Cloudrest through cold highland stone. Steam vents, courier bells, and sheltered steps communicate that a safe rest town lies ahead.

Weather effects remain cosmetic, and the player can always return to Stormbreak. The route never becomes necessary for the Tempest Crest or Hollow investigation.

### Lantern Steps

**Connects:** Gloam Quarry's southern Ashvault terrace ↔ Lanternfen's northern plank landing  
**Availability:** Second optional tier  
**Status:** Planned

Lantern Steps descend from carved black stone into damp ground and fungal light. Iron memorial lanterns gradually become cultivated green lamps, while paved switchbacks become timber walkways.

The map should contain safe rest landings, shallow water, and the first mischievous remote lantern boats from Lanternfen's side story. The path remains separate from Gloam's required Ashvault and arena routes.

## Final Circuit route

### Crownway

**Connects:** Mirror Arena's northern qualification road ↔ Crown Gate's southern checkpoint  
**Availability:** Opens after the Hollow finale  
**Status:** Planned

Crownway is the formal approach to the Unity Crest and Championship territories. Reflective silver markers gradually divide into five civilization-colored bands that reunite at Crown Gate.

The route should contain competitor lodges, registration milestones, formal gardens, and viewpoints toward the final gate. It opens only when the Hollow crisis is resolved, but afterward remains bidirectional so the Championship never removes access to the rest of the world.

## Implementation conventions

- Give each connector a named snake-case region ID, such as `old_road` or `watershed_crossroads`, inside the seamless exterior map.
- Join exterior destinations with adjacent walkable tiles. Reserve bidirectional portals for interiors and other deliberately separate spaces.
- Portal sources, destinations, NPCs, shards, and player starts require distinct walkable tiles.
- A connector must provide one permanently reliable route between every pair of its exits. Optional loops, shortcuts, lifts, ferries, and changing paths cannot replace those routes.
- Routes should visually blend all of their endpoints. The junction or midpoint is a transition, not an unrelated biome.
- Main paths should be at least two tiles wide near trainers and moving NPCs. One-tile bridges or ledges should be optional and short.
- Encounters should use fixed trainers, visible challenges, or story scenes rather than unavoidable random battles.
- Every connector should offer at least one visual landmark visible from its main route: a bridge, shelter, overlook, signal tower, carved wall, ferry, or unusual tree.
- Connecting maps may contain collectible shards and optional rewards, but those rewards must not permanently disappear when a story gate changes.
- Once a progression gate opens, it should never close an earlier route or prevent backtracking to Mercer in Emberglen.
- The World Builder must be able to edit every connector using the same tile ID, region, entity, and collision rules as towns.

## Suggested implementation order

1. **Watershed Crossroads** to open Emberglen, Glasswater, and Rootmaze through one shared junction.
2. **Rootrail Greenway** to extend the first regional loop toward Cinderrail.
3. **Fivefold Highway** to establish Dragon Keep and the midpoint gate.
4. **Ribbonway Crossroads**, **Sunmirror Causeway**, and **Courier Rise** to open the outer tier.
5. **Prism Pass** to connect the Mirror and Stormbreak regions directly.
6. Optional branches to Lilyreed Village, Honeyreach, Clayhearth, Cloudrest, Lanternfen, and Ribbonfair.
7. **Archive Approach** and **Crownway** when their destination maps and story gates are ready.
