# Stormbreak Plateau

## Layout purpose

Stormbreak Plateau should feel like one town distributed across several highland
shelves rather than a settlement built around a single square. Stone shelters,
signal towers, courier flags, rope rails, and counterweighted bridges bind the
separate neighborhoods into one community. The wind is always visible in cloth,
grass, steam, and machinery, but traversal remains readable and safe.

The town is a required second-tier region containing the Tempest Crest. It also
hosts the false-weather investigation in which copied courier memories conceal
real resonance storms. Unlike Gloam Quarry and Sunspire Cloister, Stormbreak does
not contain a Resonance Seal; its contribution to the Hollow finale is its
restored signal network and Natasha Gale's knowledge of the stolen routes.

Courier Rise connects the southwestern shelf to Dragon Keep. Prism Pass
connects the western shelf directly to Mirror Arena, and Springline Trail climbs
from the upper settlement to the optional town of Cloudrest Peak. There is no
direct road from Stormbreak to Gloam Quarry.

**Implementation status:** Planned. The suggested surface footprint is roughly
96 by 64 tiles within the future seamless overworld. Major shelter interiors,
the Forecast Chamber, and tower machinery can use separate interior maps. The
three tower states should alter optional bridge links without changing the
region's permanent external connections.

## Navigation principles

- **Breakwind Camp** is the central landmark and safe return point. Every major
  tower route reconnects to it through a fixed path.
- The town has three elevations: the lower arrival shelf, the inhabited middle
  shelves, and the exposed tower rim. Elevation is communicated through cliff
  walls, ramps, stairs, and background views rather than platforming or falling.
- Broad stone switchbacks form a permanent loop. Moving wind bridges provide
  shortcuts and puzzle routes, but never become the only way back from a tower.
- Red flame flags mark Ember Tower, white-and-gold sun flags mark Beacon Tower,
  and green forked flags mark Shepherd Tower. Teal courier flags always point
  back toward Breakwind Camp.
- Signal posts face the next safe landmark. Their cloth tails make direction
  readable even when false weather reduces the distant view.
- Shelter doors are recessed into cliff faces and framed by bright paint. Major
  buildings use distinct roof cloth, tower silhouettes, or machinery so they do
  not disappear into the stone.
- NPC patrols stay on broad terraces. Couriers may cross bridges, but they must
  not stop in doorways, on single-width stairs, or on bridge switch tiles.
- Severe wind and lightning are visual and narrative effects. They must not push
  the player, alter input, obscure collision boundaries, or cause random damage.

## Schematic layout

```text
                                      Springline Trail
                                         to Cloudrest
                                               |
                                      [Upper Shelter Road]
                                               |
                 [Ember Tower] ==== [High Relay Spine] ==== [Beacon Tower]
                       |                 |       |                 |
                [Forge Shelter]    [Forecast Chamber]      [Lens House]
                       \                 |                /
                        \          [Tempest Arena]       /
                         \                |             /
                    [West Shelf] -- [Breakwind Camp] -- [East Shelf]
                         |            /     |     \           |
                 [Prism Gate] [Card Shop] [Galehouse] [Bridgewright Yard]
                         |            \     |     /           |
                Prism Pass         [Undercliff Row]     [Shepherd Lift]
               to Mirror Arena            |                   |
                                          |            [Shepherd Tower]
                                  [Lower Courier Ward]          |
                                          |              [Grazing Shelf]
                                    [Courier Gate]
                                          |
                                      Courier Rise
                                    to Dragon Keep
```

The diagram is schematic. The final town should use staggered shelves and curved
switchbacks rather than a rectangular road grid. `====` marks reconfigurable
upper bridge links; the ordinary paths below them remain permanent.

## Central shelf

### Breakwind Camp

Breakwind Camp occupies the widest protected shelf beneath a hooked cliff. It
began as an emergency assembly point and became the practical center of town.
Three large signal masts rise from its outer edge, one aligned with each tower.
Their flags show tower state at a glance.

The camp contains:

- A relief map showing fixed roads and current wind-bridge links
- A circular public duel platform set flush with the stone
- Three tower-status lamps colored red, gold, and green
- Benches enclosed by waist-high windbreaks
- A repair board listing damaged bridges, late couriers, and supply requests
- Emergency lockers containing rope, blankets, lamps, and preserved food
- Direction stones for Courier Rise, Prism Pass, and Springline Trail

The player arrives here shortly after entering from Courier Rise. Natasha gives
the tower briefing beside the relief map and allows the towers to be attempted
in any order. Between towers, this remains the reliable place to edit a deck,
rest, and ask for updated route information.

### Galehouse Relay Office

The Galehouse stands directly against the sheltering cliff behind the camp. Its
lower floor is a courier office; its upper room contains the controls that
combine tower readings into a regional forecast. A low, rounded roof prevents
the wind from lifting it, while four painted cables make it visible from every
approach.

The public floor contains:

- A dispatch counter and numbered message slots
- A wall map of active and delayed courier routes
- Lockers for visiting runners
- A compact telegraph-like resonance relay
- Natasha's weathered desk and route notebooks
- A waiting bench used by story NPCs from earlier regions

The upper operations room initially reports convincing but incorrect forecasts.
After all three towers are restored, comparing their raw signals exposes the
blank pattern hidden inside the combined report.

### Crosswind Card Supply

Stormbreak's card shop occupies a squat stone building beneath a striped teal
awning. It supplies working couriers as much as competitive duelists, so its
stock favors compact decks, sturdy card cases, and cards useful after an
unpredictable shield break.

The shop specializes in:

- Fire, Light, and Nature Wave Strikers
- Low-cost creatures that help establish formation thresholds
- Shield triggers and defensive creatures that preserve a formation
- Speed attackers for converting a stable board into pressure
- Multi-civilization support imported through Dragon Keep and Mirror

The shop expands after each tower restoration. Its final stock should support a
player who wants to experiment with Wave Strikers before fighting Natasha,
without requiring the player to copy her exact list.

### The Tethered Kettle

The local inn and dining hall is named for the heavy chain holding its outdoor
kettle in place. Sleeping rooms are carved into the rear cliff, while the front
room has thick shutters and a sunken communal hearth.

The inn provides:

- A rest point and future fast-travel marker
- A kitchen where stranded travelers appear during the tower crisis
- A message board for courier races and missing-package side quests
- Rematch dialogue from tower guards after the Tempest Crest is earned
- A storm cellar used during one false-weather story scene

Its sheltered courtyard should remain busy in every story state, emphasizing
that Stormbreak responds to isolation by gathering rather than retreating.

### Camp Supply Arcade

Canvas stalls line the inner wall of Breakwind Camp. Merchants sell climbing
rope, preserved meals, weatherproof clothes, bridge fittings, medicinal herbs,
and small imported goods. Every stall can close behind a stone shutter during a
storm.

One stall exchanges old route tokens and courier stamps for modest card or gold
rewards. Another offers cosmetic card backs patterned after the three tower
flags. These are optional services and should not compete with Mercer's role as
the main general card merchant.

## Lower courier ward

### Courier Gate

Courier Rise enters Stormbreak through a broad southwestern cut protected by
two overlapping stone walls. The gate has no door; alternating walls break the
wind while leaving carts and travelers a clear path.

Teal route bands lead directly uphill to Breakwind Camp. A staffed shelter
records arrivals and warns the player about the unreliable forecasts. The road
back to Dragon Keep remains open after the second tier is unlocked, including
during every stage of Stormbreak's local crisis.

### Dispatch Hall

The Dispatch Hall sorts letters, card shipments, medicine, and Circuit records
before runners carry them across the plateau. Long tables are bolted to the
floor, and hanging route boards can be flipped without removing them from their
chains.

During the story, the player can compare delayed parcels with the predicted
weather. Several delays contradict the official Galehouse report, providing an
early clue that the false storms are targeted around particular remembered
journeys.

### Pack Court

Pack Court is a sheltered loading yard with handcarts, cargo nets, rain covers,
and marked bays for each regional road. It should feel functional rather than
like an adventurer stable. Deliveries from previous towns visibly change as
their routes reopen.

Suggested environmental details include Glasswater crates, Cinderrail fittings,
Rootmaze rope, Mirror training records, and parcels addressed to Mercer. A small
delivery quest can send the player to any already accessible region without
making Stormbreak's main progression depend on it.

### Weather Clinic

The clinic treats lightning burns, falls, exposure, and exhaustion. It is built
partly underground and marked by a white windsock bearing a blue cross. The
waiting room doubles as the town's safest public shelter.

Recovered couriers provide fragments of route dialogue here. None can remember
the dangerous storm accurately until the towers are repaired, but their physical
injuries prove that the official clear-weather forecast was false.

## Residential middle shelves

### Undercliff Row

Most homes occupy a long shelf immediately below Breakwind Camp. Their rear
rooms extend into the rock, while small front rooms and gardens sit behind
shared wind walls. Roofs are low timber frames covered in blue, rust, or green
weather cloth.

At least three homes should be enterable. Suggested interiors include:

- A courier family kitchen with maps marked by several generations
- A bridge worker's home full of scale models and spare pulleys
- A retired duelist's room displaying an incomplete early Wave Striker set
- A communal laundry and drying room warmed by a protected vent

Exterior details should vary between houses: tethered flower boxes, painted
door lintels, folded bridge cloth, children's windsocks, and stone benches.

### Commons Kitchen

Because separate household fires are dangerous in high wind, several families
share a large kitchen enclosed behind two wind doors. Bread ovens, soup pots,
and drying racks make it one of the warmest social spaces in Stormbreak.

The kitchen becomes a relief station when a false forecast strands travelers.
After restoration, it hosts a celebration meal whose attendees depend on which
other second-tier regions the player completed first.

### Windwright School

Children learn ordinary subjects alongside route symbols, first aid, flag code,
and bridge safety. The schoolyard contains short practice masts whose flags can
be arranged into simple messages.

A tutorial puzzle here teaches the three tower colors before the player reaches
the upper routes. Optional students use beginner formation decks, showing the
Wave Striker threshold in a low-stakes setting.

### Natasha's route room

Natasha lives in two small rooms above a communal equipment store. Her home is
practical rather than ceremonial: one wall is covered in route sketches, one in
repair schedules, and the remaining space is occupied by boots, messenger bags,
and half-built decks.

The player finds Natasha here only during quiet story states. After earning the
Tempest Crest, the route room becomes the location for rematch registration and
dialogue about whichever roads or Hollow regions remain unresolved.

## Bridgewright district

### Bridgewright Yard

The eastern middle shelf holds the machinery used to maintain Stormbreak's
fixed spans and wind bridges. Test frames, cable drums, counterweights, cloth
panels, and spare anchors fill a broad fenced yard.

Wind bridges are real physical structures: narrow but sturdy decks are folded
against anchor towers until a counterweight and wind vane pull them into place.
Resonance controls coordinate the locks but do not create an invisible magical
floor. This gives each bridge a clear silhouette and an understandable state.

The yard contains a permanent route back to Breakwind Camp. Changing tower
alignment may connect it to one upper shelf or another, but never closes its
ordinary entrance.

### Cable House

The Cable House stores labeled replacement lines and maintains tension records
for every bridge. Its interior offers a compact machinery puzzle in which the
player follows mismatched maintenance dates to identify a tower receiving false
instructions.

After the crisis, the cable keeper sells cosmetic flag colors and provides hints
for optional high-shelf collectibles.

### Shepherd Lift

A counterweighted cargo platform links the eastern settlement to the grazing
shelf below Shepherd Tower. It operates as a short scripted transition rather
than free-moving platform gameplay. Stairs and a switchback path provide a
permanent alternative.

The lift may become a shortcut after the first nearby repair, but no objective
should require the player to wait for it or board it while an NPC occupies the
landing.

## The three signal towers

The towers can be restored in any order. Each route includes a local problem, a
guardian duel, and a signal-alignment interaction. Completing a tower changes
the status lamps at Breakwind Camp and adds its raw reading to the Galehouse.

Each tower should have a fixed walking route back to the center. Activating a
tower may retract one upper shortcut while extending another, but the game must
validate the bridge state so the player, NPCs, and required objectives always
remain connected through permanent paths.

### Ember Tower — western formation route

Ember Tower stands on a dark red outcrop above the road to Mirror. Its signal
fire creates rising air that keeps western courier flags visible through rain.
The approach passes the Forge Shelter and several exposed switchbacks.

Its guardian uses an aggressive Fire-led Wave Striker formation. The local
challenge teaches that reaching three members can turn modest creatures into a
decisive attack, but overextending into shield triggers remains dangerous.

Restoring Ember Tower:

- Lights the red mast at Breakwind Camp
- Opens a high shortcut toward Prism Gate
- Adds pressure and temperature readings to the Galehouse
- Reveals that a predicted western storm never existed

### Beacon Tower — northern protection route

Beacon Tower occupies the highest inhabited shelf. Polished Light plates flash
messages toward Cloudrest and Dragon Keep, while a deep lightning shelter sits
beneath its base. The route passes Lens House and the upper shelter road.

Its guardian uses Light-led Wave Strikers, blockers, and shield triggers. The
duel emphasizes preserving the third formation member rather than attacking
with every available creature.

Restoring Beacon Tower:

- Lights the gold mast at Breakwind Camp
- Opens an upper connection toward Springline Trail
- Adds lightning and visibility readings to the Galehouse
- Exposes a dangerous real storm omitted from the official forecast

### Shepherd Tower — eastern support route

Shepherd Tower watches the lower grazing shelves and the approach used by remote
households. Its green forked flag is visible from the Bridgewright Yard. The
fixed route uses a broad switchback; the Shepherd Lift becomes an optional
shortcut.

Its guardian uses Nature-led Wave Strikers with mana acceleration and board
support. The duel demonstrates how apparently weak creatures gain value from
the formation around them.

Restoring Shepherd Tower:

- Lights the green mast at Breakwind Camp
- Activates a shortcut between the grazing shelf and east residences
- Adds wind-direction and ground-vibration readings to the Galehouse
- Recovers a signal carrying the final route of a missing courier

### High Relay Spine

The High Relay Spine is a chain of exposed maintenance platforms above the
three tower routes. Its wind bridges are the most visibly reconfigurable part of
the region. They create shortcuts between completed towers and lead to optional
chests, trainers, and observation points.

The Spine is not required to return to camp or reach any external road. Before
all towers are active, some spans remain folded and clearly display their future
destination through colored flags. After restoration, the player can cycle a
stable set of bridge arrangements from the central control post.

## False-weather investigation

### Forecast Chamber

The Forecast Chamber sits between Breakwind Camp and the High Relay Spine. Its
walls display three mechanical traces, one from each tower, and a fourth combined
forecast distributed to the settlement.

Before tower restoration, the combined display appears more complete than the
damaged raw readings. Once all three towers report independently, the player can
see that portions of the combined line were inserted rather than measured.

### Route Memory Gallery

A narrow room beneath the Forecast Chamber preserves obsolete flags, courier
badges, and maps from journeys that ended in rescue, retirement, or death. The
displays are records of service, not a memorial cult like Gloam's Ashvault.

The copied forecasts reproduce exact details from several final journeys. Small
anachronisms—an old tower color, a retired bridge number, and a route closed
years ago—allow Natasha to identify the source as stolen memory rather than
ordinary instrument failure.

### Blank Signal Annex

A maintenance crawlspace behind the combined relay contains an unauthorized
receiver aimed toward the Hollow Archive network. It has no local sender name
and records weather patterns only after matching them to archived courier
routes.

The annex is revealed late in the investigation. It should provide evidence and
story dialogue, not become a dungeon. A fixed Hollow-aligned trainer encounter
can guard the receiver before Natasha disconnects it.

## Tempest Arena district

### Tempest Arena

Natasha's arena occupies a broad natural saddle above Breakwind Camp. Three
signal masts surround a circular stone duel floor. Spectators watch from covered
terraces built into the uphill side, while the open edge looks across the tower
network.

The arena remains visible from the beginning, but registration opens only after
all three towers are restored. This is a civic safety requirement and Circuit
trial, not a magical Crest lock.

The complex includes:

- A registration shelter and deck-editing table
- Three illuminated tower emblems around the duel floor
- Covered spectator terraces with separate entrances
- A referee station connected to the Galehouse
- A rear route allowing staff to reach the arena without crossing the duel floor

Winning awards the Tempest Crest and Natasha's signature reward, Sapian Tark,
Flame Dervish. The arena remains available for rematches afterward.

### Formation Yard

Below the arena, three connected practice circles host Wave Striker lessons. A
player can fight short scenario duels about assembling, protecting, or breaking
a three-creature formation.

These optional challenges should explain Natasha's deck identity through play.
Rewards can include gold, formation support cards, and cosmetic tower flags.

### Challenger Shelter

Visiting duelists sleep and prepare in a long stone shelter beside the arena.
Its deck tables display local advice without revealing Natasha's exact hand or
deck order.

After the Crest match, tower guardians gather here for stronger rematches. The
shelter also gives earlier NPCs a natural place to appear during Act III.

## Grazing and outer shelves

### Grazing Shelf

The eastern low shelf supports hardy grass, small herd shelters, and water tanks
fed by Cloudrest runoff. Stone fences and bright cloth markers keep the route
visible in fog.

This area supplies food and wool rather than serving as a monster enclosure. A
few homes sit far from the central camp, explaining why Stormbreak depends on
signals and couriers even within its own borders.

### Remote shelters

Small emergency huts stand along every tower route. Each contains a lamp, route
map, blankets, and an inward-opening door. Some hold stranded NPCs or optional
items during the false forecasts.

Once discovered, shelters act as recognizable checkpoints, not teleport points.
Their lamps change from amber to blue after the nearest tower is restored.

### Observation ledges

Optional paths lead to fenced overlooks with views toward Dragon Keep, Mirror,
Cloudrest, and the distant Archive road. Collectibles and trainer encounters can
reward exploration without placing valuable objects on visually unsafe cliff
edges.

## Regional exits

### Courier Rise to Dragon Keep

The southwestern gate descends through Courier Rise to Dragon Keep's northeastern
courier road. Teal relay flags and frequent stone shelters distinguish it as
Stormbreak's primary supply route.

This connection opens with the second overworld tier and remains bidirectional.
Nothing in the tower puzzle may close or relocate its boundary approach.

### Prism Gate to Mirror Arena

The western shelf enters Prism Pass through a pair of reflective signal plates.
Silver markers gradually replace Stormbreak's teal flags as the road approaches
Mirror Arena.

The permanent lower road to Prism Gate stays open throughout the tower crisis.
Ember Tower can activate an optional high shortcut, but that shortcut must not
replace the ordinary connection.

### Upper Shelter Road to Cloudrest

The north road climbs into Springline Trail and then Cloudrest Peak. Warm runoff,
steam vents, and brass bells distinguish it from the exposed tower paths.

Cloudrest is optional. Its road never lies between Breakwind Camp and Beacon
Tower, Natasha's arena, or any part of the Hollow investigation.

## Suggested NPC placement

- **Natasha Gale:** At the Breakwind Camp map, then the Galehouse and Tempest
  Arena as the story advances
- **Renn Torchline, Ember guardian:** Forge Shelter and Ember Tower approach;
  aggressive Fire-led Wave Strikers
- **Ilyra Voss, Beacon guardian:** Lens House and Beacon Tower; defensive
  Light-led Wave Strikers
- **Bram Reedflag, Shepherd guardian:** Grazing Shelf and Shepherd Tower;
  Nature-led formation and mana support
- **Mara Tension, bridgewright:** Bridgewright Yard; explains bridge states and
  provides repair side quests
- **Orrin Slate, dispatcher:** Dispatch Hall; tracks contradictions between
  actual arrivals and forecasts
- **Sella North, clinic keeper:** Weather Clinic; introduces injured couriers'
  accounts
- **Tav Kettle, innkeeper:** The Tethered Kettle; rest service and traveler
  dialogue
- **Nim, junior courier:** Moves between Breakwind Camp and Courier Gate before
  returning to Cloudrest after restoration
- **Hollow receiver agent:** Fixed encounter in the Blank Signal Annex; uses a
  deck that imitates one previously fought tower formation

Generic couriers should move between named shelters on broad routes. Bridge
workers remain near machinery, and residential NPCs should not wander onto the
upper tower rim during dangerous story states.

## Story-state changes

### Before the tower crisis

- All three camp signal lamps flicker or remain dark
- The official forecast confidently reports harmless weather
- Upper wind bridges sit in inconsistent, partially folded positions
- Injured or delayed couriers gather at the clinic and inn
- Natasha remains at Breakwind Camp and allows the towers in any order
- Courier Rise, Prism Pass, and Springline Trail remain physically accessible
  according to their ordinary progression gates

### After each tower restoration

- The matching camp lamp activates
- Its fixed route gains repaired flags and shelter lights
- Its raw weather trace appears in the Forecast Chamber
- One useful high-shelf shortcut opens
- The guardian returns to the central area with updated dialogue
- Shops gain a small selection associated with that tower's civilization

Tower order changes dialogue and the first contradictions the player discovers,
but not which objectives remain available.

### After all three towers

- The Forecast Chamber comparison becomes solvable
- The Tempest Arena opens for Natasha's Crest match
- The High Relay Spine gains a stable set of optional bridge arrangements
- The Blank Signal Annex can be discovered
- A real storm becomes visible in the distance while all essential paths remain
  readable

The Hollow investigation and Crest match may resolve in either order once their
requirements are met, allowing the sporting and existential storylines to remain
parallel.

### After earning the Tempest Crest

- The Crest emblem appears above the arena registration shelter
- Natasha offers increasingly difficult formation rematches
- Tower guardians move to the Formation Yard and Challenger Shelter
- Crosswind Card Supply adds advanced Wave Striker support
- Couriers begin carrying news from whichever other second-tier regions remain

### After disconnecting the blank signal

- Forecasts display raw and combined readings side by side
- Injured couriers regain clearer memories without implying that trauma simply
  vanishes
- The Route Memory Gallery labels the copied journeys with consent from their
  families
- Natasha commits the courier network to the Hollow Archive approach
- False storm effects cease, while ordinary highland weather continues

### During Act III

- Supply crates from restored regions accumulate in Pack Court
- Named allies pass through the Galehouse on their way toward the finale
- The three tower flags carry the player's route order as small colored bands
- Stormbreak remains fully explorable and all three external roads stay open
- Natasha carries match results and urgent messages rather than abandoning her
  town to wait at the final gate

## Implementation notes

- Build the settlement as a surface region with three visually distinct height
  bands. Cliffs are solid tile boundaries; ramps and stairs are ordinary
  walkable tiles rather than physics-driven slopes.
- Keep a permanent connected graph from every tower, external road, arena, and
  service back to Breakwind Camp. Wind-bridge state changes may only add or
  remove secondary edges from that graph.
- Represent bridge configurations with explicit world-story state. Save the
  active configuration and normalize invalid or obsolete values on load.
- Courier Rise, Prism Pass, and Springline Trail connect through adjacent
  walkable boundary tiles on the seamless overworld, not exterior portals.
  Their approaches remain fixed and bidirectional when bridges change.
- Use distinct semantic tile IDs for highland ground, fixed stone path, cliff,
  rope rail, shelter wall, shelter roof, tower machinery, fixed bridge, folded
  wind bridge, and active wind bridge. Do not overload existing glyph meaning
  based on region.
- Bridge animations should complete before collision changes. During the
  animation, both the retracting and extending spans remain non-walkable, and
  the player cannot start the change while occupying either endpoint.
- Weather overlays render behind characters, interaction prompts, and collision
  landmarks. Lightning flashes need a reduced-flash setting and should never
  produce a fully white frame.
- Wind flags may animate independently, but avoid updating every decorative
  object off-screen. Use the existing visible-tile bounds for terrain and cull
  distant animated details as the world approaches its full size.
- Provide broad landings around every NPC, tower control, regional boundary,
  and building entrance. No required interaction belongs on a one-tile bridge.
- Preserve free backtracking. The player can leave Stormbreak between tower
  attempts, visit Mercer, complete Mirror or Gloam first, and return without
  resetting repaired towers.
- Keep the Tempest Crest separate from Hollow progression. Tower restoration
  authorizes Natasha's sporting trial; disconnecting the blank receiver advances
  the Hollow investigation. Neither event should silently grant the other.
