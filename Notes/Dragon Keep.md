# Dragon Keep

## Layout purpose

Dragon Keep should feel like a working castle-town built to manage roads and
resonance rather than a royal palace or another arena district. Its walls contain
public offices, relay machinery, archives, workshops, homes, and sheltered
caravan yards. The central Fivefold Court hosts Kestrel's official duel, but the
Keep exists for many reasons when no match is taking place.

The Keep is the mandatory midpoint Crest region. It awards the Confluence Crest,
restores access to the second open-world tier, and later becomes the staging
ground for entering the Hollow Archive. It contains no Resonance Seal. Instead,
it stands over the **Confluence**, where all five civilization currents pass
close enough to be compared and relayed while remaining distinct.

The location connects south to Emberglen through Fivefold Highway. Its western
gate reaches Ribbonway Crossroads, its northeastern gate reaches Stormbreak
through Courier Rise, its eastern gate reaches Sunmirror Causeway, and its
northern Archive Gate leads toward the Hollow Archive. Ribbonway and Sunmirror
provide two different routes to Mirror Arena. There is no direct Dragon Keep road
to Gloam Quarry.

**Implementation status:** Planned. A surface footprint of roughly 96 by 64
tiles should provide enough space for the castle, lower bailey, outer wards, and
five clearly separated road approaches. The relay tower, Warden Hall, Dragon
Gallery, and Confluence undercroft can use interior maps. Exterior roads should
join the seamless overworld through adjacent walkable tiles rather than portals.

The metaphysical rules behind the Confluence, Dragon calibration, Crests, echoes,
and relay are defined in `Notes/WorldPhysics.md`.

## Navigation principles

- The **Relay Watchtower** is visible above every exterior district and acts as
  the main visual anchor.
- The **Fivefold Court** is the central navigation hub inside the walls. All
  public castle routes reconnect to it.
- Five large external gates have distinct colors and silhouettes. They should
  never be confused with the three smaller gatehouses used for Kestrel's trial.
- The south gate and lower bailey remain accessible before the midpoint match.
  Progression closes specific outward checkpoints, not the entire town.
- The outer wall forms a walkable loop with frequent stairs back to street level.
  No required destination should depend on following the entire wall circuit.
- Public roads are at least three tiles wide near shops, NPC routes, and gates.
  Guards and caravans must not obstruct single-width passages.
- Confluence channels appear through instruments, glass seams, colored inlays,
  and subtle environmental effects. The castle should not sit above a giant
  glowing magical pit.
- Dragon decoration uses five different historical silhouettes rather than
  repeating one statue. The Keep commemorates Dragon-linked cards; it does not
  house tame physical dragons.
- Closed routes remain visibly intact. Gate notices and staffed checkpoints
  explain progression without implying that geography changes when a Crest is
  won.

## Schematic layout

```text
                                      [Archive Approach]
                                              |
                                      [Archive Gate]
                                              |
                              [Old Records]--[North Ward]
                                      |       |
            [Ribbonway]--[West Gate]--+--[Relay Watchtower]--[East Gate]--[Sunmirror]
                              |       |       |       |             |
                        [Banner Ward]  | [Fivefold Court]      [Scholar Ward]
                              |       |    /    |    \             |
                              |    [Scale] [Furnace] [Wing]         |
                              |       \       |       /             |
                         [Dragon Gallery]--[Warden Hall]--[Relay Workshop]
                                  |          |             |
                           [Wall Homes]--[Inner Bailey]--[Courier Ward]
                                  |          |             |
                         [Card Foundry] [Five Roads Market] [Northeast Gate]
                                  |          |             |
                            [Wayfarer Inn]--[Lower Bailey]--+--[Courier Rise]
                                              |
                                         [South Gate]
                                              |
                                      Fivefold Highway
                                        to Emberglen

                      Beneath the central ward: Confluence Undercroft
                  [Five Current Galleries]--[Separator Hall]--[Baseline Archive]
                                              |
                                  [Confluence Observatory]
```

This diagram is schematic rather than tile-accurate. The final map should use
offset walls, layered terraces, angled gate roads, and irregular old foundations
so the Keep feels rebuilt over centuries instead of planned as a perfect grid.

## Lower bailey and castle-town

### South Relay Gate

Fivefold Highway enters through the broadest of the Keep's gates. Two staggered
gate arches break wind and control carts without creating a narrow player
corridor. A dragon head carved above the inner arch holds five colored route
lamps in its jaws.

Before the midpoint story, the south road and gate remain open. Travelers can
visit the lower bailey, shop, duel local trainers, and return to Emberglen. The
checkpoint concerns the outer network beyond the Keep, not access to the Keep
itself.

The gate complex contains:

- A road-condition board covering all five external approaches
- A Circuit registration desk
- Cargo inspection bays beside, not across, the player path
- A sheltered guard room
- Public stairs to the southern wall walk
- A visible relay cable climbing toward the central watchtower

### Lower Bailey

The lower bailey is a large paved arrival court inside the south wall. Caravan
lanes curve around a pedestrian island rather than cutting through its center.
The player should immediately see signs for Five Roads Market, the inn, the card
shop, and the uphill route to the inner castle.

The central island includes:

- A relief map of Dragon Keep and its five gates
- A public duel table for ordinary NPC challenges
- Covered benches for delayed travelers
- Water troughs and hand pumps
- A route-status mast repeating the tower's signals at street level
- A notice board for deliveries, escorts, and road repairs

The lower bailey remains lively in every story state. During a relay closure it
fills with delayed caravans rather than becoming an empty disaster zone.

### Five Roads Market

The Keep's main market occupies stone arcades between the lower and inner
baileys. Permanent shops sit in the wall while temporary stalls use marked bays
that keep the central street clear.

Goods arriving from every available region should be visible:

- Emberglen food, cloth, and general card supplies
- Glasswater instruments and waterproof cases
- Rootmaze timber, rope, and herbs
- Cinderrail fittings and heat-resistant ceramics
- Mirror analysis sheets and Stormbreak courier equipment after the outer tier
  opens

Inventory and stall dressing can change with story progression. The market is a
visual summary of connected roads, not a substitute for the specialized shops in
other towns.

### The Five Roads House

The main inn occupies an old storehouse whose five wings were added at different
times. Its sign shows a sleeping dragon curled around a milestone. Interior
floors do not quite align, giving it character without confusing navigation.

The inn provides:

- A rest point and future fast-travel marker
- A communal dining room for travelers from several regions
- A contract board for route quests
- Temporary rooms for returning allies during the Hollow finale
- Dialogue that changes according to which first- and second-tier roads are open

An enclosed rear yard hosts casual duels and caravan meals. It should not be
called or dressed as an arena.

### Wyrm and Waystone Cardhouse

Dragon Keep's primary card shop stands beside the road into the inner bailey. A
red stone dragon winds around its doorway, while five small route plaques show
that non-Fire customers are welcome.

Its specialist stock includes:

- Dragonoids and Fire Birds used as early setup
- Armored Dragons and Dragon evolution creatures
- Fire removal and mana-denial cards
- Cards that protect or recover evolution material
- A rotating shelf of coherent mixed-civilization imports

The shop's deck clinic focuses on the turns before a large finisher. Staff ask
what a deck does when it does not draw its most expensive Dragon, reinforcing
Kestrel's Circuit lesson without giving away his exact list.

### Freight Hall

The Freight Hall records cargo moving through the Keep without collecting a
lord's toll. Its ledgers track road condition, destination, dangerous materials,
and delayed shipments. Taxes and ordinary trade fees may exist elsewhere, but
the castle gate is not a feudal extortion point.

During the relay crisis, mismatches between these physical ledgers and the
watchtower's current route reports provide the first evidence that old signal
paths have been reactivated.

## Inner bailey

### Inner Bailey

The inner bailey is a quieter service court between the public market and the
central ward. Kitchens, clinics, workshops, and warden offices open onto covered
walkways. A broad stair rises toward Fivefold Court, while side routes reach the
Dragon Gallery and Courier Ward.

The court contains:

- A public well monitored for cross-current interference
- Shift boards for relay and gate staff
- Practice targets used by junior wardens
- A small memorial to travelers rescued during past road disasters
- An equipment return desk

It should feel like the workplace that keeps the castle operating, not a second
grand plaza.

### Warden Hall

Warden Hall serves as town hall, road authority, and emergency command center.
Its long meeting room contains a five-part road table whose sections can be
replaced as routes change.

Public rooms include:

- The road warden's reception desk
- A hearing room for right-of-way and trade disputes
- A map chamber used during the midpoint crisis
- Storage for gate seals and emergency notices
- Kestrel's small office overlooking the Fivefold Court

Kestrel is a road warden rather than a lord. Residents can challenge his
decisions, records are publicly reviewed, and authority passes to another
qualified warden when he travels.

### Common Kitchen and Shelter Hall

The castle maintains a large kitchen and sleeping hall for stranded travelers.
It can support the town when several routes close simultaneously. Thick doors
divide the room into manageable sections without making it feel like military
barracks.

During story crises, NPCs from inaccessible roads gather here and provide clues
about the last valid messages they received.

### Relay Clinic

The clinic treats ordinary travel injuries as well as resonance feedback from
damaged equipment. Shielded cabinets hold cards that became unstable near the
Confluence but are not blank enough to require archive intervention.

The clinic makes the relay's risk visible without suggesting routine work is
constantly lethal. Most patients have fatigue, burns from overheated conductors,
or headaches caused by bad calibration.

## Central ward

### Fivefold Court

The Keep's former muster yard is now its main civic square and official Crest
venue. Five colored stone channels enter beneath separate arches, curve around
the perimeter, and remain visibly distinct. They are instrument housings, not
open rivers of magic.

The Court contains:

- A circular sanctioned duel floor at its center
- Public galleries built into two wall sections
- Five route-status pillars
- A removable speaking platform for civic meetings
- Direct public paths to Warden Hall and the Circuit Registry
- A staff passage that never crosses the active duel floor

Markets, announcements, and public votes use the Court when no official match
is scheduled. Kestrel's duel should therefore transform a familiar civic space
for a special event instead of revealing that the entire castle was secretly an
arena.

### Circuit Registry

The Registry certifies Crest challenges, records sanctioned results, and checks
that deck lists follow ordinary Circuit rules. It occupies an accessible ground
floor beside the Court rather than a remote tower chamber.

The player can inspect the prerequisites for Kestrel's match here. Before the
Tidal, Forge, and Verdant Crests are earned, the clerk clearly explains that the
outer roads require both relay repair and midpoint Circuit authorization.

### Relay Watchtower

The tall watchtower grew around the Keep's original signal post. Its lower floors
are square stone; later relay levels use lighter metal frames, colored glass, and
five directional aerials. The tower should be visible but not enterable on every
floor.

Important levels are:

- **Signal floor:** Receives road, weather, and Circuit traffic
- **Translation floor:** Converts a message between civilization alignments
- **Dragon calibration floor:** Applies a strong known reference pattern
- **Observation crown:** Displays the health of every external relay branch
- **Maintenance loft:** Contains cables, cooling vents, and manual cutoffs

The tower transmits reports and prepared messages, not people's unrestricted
thoughts. Its machinery requires ordinary power, cooling, operators, and
physical repair.

### Route Map Hall

The Map Hall wraps around the watchtower's base. Layered wall maps preserve old
roads rather than painting over them whenever a route changes. Kestrel prefers a
single current display and initially considers the old layers clutter.

The Chief Collector exploits precisely those retired paths. Comparing a current
map with preserved physical route plates reveals signals traveling along a branch
that officially no longer exists.

## The three trial gatehouses

Kestrel's Keep trial uses three **internal** gatehouses between the inner bailey
and Fivefold Court. They do not control regional travel. Each has a permanent
pedestrian bypass after its scenario is completed, preventing the trial from
making daily castle navigation inconvenient forever.

The gatehouses may be completed in any order. Each combines a short physical
inspection with a duel focused on one part of Kestrel's Dragon strategy.

### Scale Gate — preserve the foundation

Scale Gate crosses the lowest and most stable current channel. Its walls display
small Dragonoid and Fire Bird carvings beneath a single unfinished Dragon relief.

The player identifies which support anchors must remain active before opening a
heavy relay shutter, then duels a gate warden whose deck punishes careless attacks
with evolution material.

The lesson is that an inexpensive creature may be worth more as the foundation
of a later evolution than as one additional shield attack.

### Furnace Gate — carry the cost

Furnace Gate surrounds a cooling chamber for the Dragon calibration floor. Heat
comes from ordinary relay machinery as well as Fire alignment, so vents and
coolant valves remain physically important.

The player routes a limited amount of power between the gate lock, cooling fans,
and reserve conductors. The guardian duel then tests mana discipline with costly
Dragons and effects that punish charging the wrong card.

The lesson is not simply to save every card for mana. The player must decide
which future play the current mana zone is actually supporting.

### Wing Gate — choose the opening

Wing Gate overlooks Fivefold Court. Two mechanical wings open from its arch when
the relay reports a safe current alignment. Old timing marks reveal that the
corrupted signal is opening it slightly too early.

Its guardian uses aggressive Dragon support and exposes tempting but unsafe
attacks. Winning demonstrates when to commit a costly creature and when to keep
the board protected for one more turn.

Completing all three gatehouses opens the formal stair to Fivefold Court and
qualifies the player for Kestrel's match once the story prerequisites are met.

## Dragon history and calibration district

### Dragon Gallery

The Dragon Gallery occupies a broad western hall whose exterior is supported by
five carved dragon buttresses. It preserves the cards, instruments, and written
accounts associated with the first Confluence surge.

The central displays explain:

- Dragons possess high resonance inertia but are not a sixth civilization
- Five Dragon-linked reference cards held distinct alignments during the surge
- The original cards are protected calibration artifacts, not tournament prizes
- Later wardens developed safer nonliving instruments from those readings
- Kestrel's mono-Fire deck supplies the modern high-pressure test signal

The Gallery should not claim that giant physical Dragons once lived in the
castle. Story illustrations show bounded manifestations anchored by the early
wardens.

### Calibration Hall

This reinforced duel chamber is used to test the relay with known card patterns.
Its floor contains extra cooling plates and measurement rings. Public access is
limited, but a viewing gallery allows apprentices to study safe tests.

Kestrel practices here before his Crest match. During the Hollow crisis, the
player discovers that the Chief Collector's false signal appears only while the
loud Dragon calibration is active.

### Scale and Ember Workshop

The workshop repairs duel focuses, card cases, conductor frames, and the
mechanical components of the calibration chamber. It does not manufacture living
echoes or casually duplicate rare Dragons.

An optional craftsman can restore cosmetic card frames or trade common Dragon
support after the player completes a materials quest from Cinderrail.

### Kestrel's deck room

Kestrel stores working deck lists on movable boards rather than preserving a
single trophy deck. Disassembled experiments fill labeled drawers. His official
mono-Fire Dragon deck occupies the only permanent red board during the midpoint
story.

The room communicates both his strength and his flaw: he records what he intends
to build next more carefully than why an older build failed.

## Confluence undercroft

### Undercroft entrance

Public stairs beneath the Dragon Gallery lead to an observation balcony. Secure
service stairs from the watchtower descend farther into the separator machinery.
The player sees the undercroft early but gains full access only during the relay
investigation.

The undercroft should feel like old infrastructure: thick foundations, patched
conduits, mineral seams, drainage channels, and instruments added across several
eras. It is not an ancient magical dungeon filled with unexplained traps.

### Five Current Galleries

Five galleries follow the measurable arms of the Confluence. Each uses materials
suited to its alignment:

- Pale ceramic baffles and promise tablets for Light
- Flowing water gauges and blue glass for Water
- Black memory stone and sealed record niches for Darkness
- Heat-resistant metal and pressure shutters for Fire
- Root-supported masonry and living calibration beds for Nature

The materials help instruments read the currents; they do not imply that every
civilization literally consists of its associated element.

### Separator Hall

Separator Hall keeps the five currents close enough for translation but distinct
enough to preserve identity. Concentric machinery rings adjust physical
conductors and compare their output against archived baselines.

Normal controls cannot combine all five currents into a colorless super-current.
The Curator's modification bypasses safeguards by presenting blank interference
as if it were a valid shared reference.

### Confluence Observatory

The Observatory is a circular chamber beneath Fivefold Court. Five narrow glass
inlays show measurement traces around a plain central stone. The true Confluence
is inferred from instruments rather than visible as a swirling portal.

This chamber hosts the central midpoint investigation scene. Kestrel applies his
Dragon calibration, Rowan's hidden comparison routine separates the noise, and
the player identifies a signal traveling through a retired archive branch.

### Baseline Archive

The Baseline Archive preserves readings from every major relay configuration.
Records are stored on physical plates as well as resonant glass so a corrupted
network cannot rewrite both copies at once.

Kestrel's habit of replacing his active configurations made the archive seem
obsolete. The crisis proves why old measurements must remain understandable, not
merely preserved in a room nobody visits.

### Hidden Collector bypass

A sealed maintenance passage contains the Chief Collector's bypass. It connects
an abandoned translation ring to the Archive branch and masks its output beneath
the Fire Dragon calibration signal.

The bypass is evidence and machinery, not a Resonance Seal. Removing it restores
trustworthy relay comparison but does not repair the five distant sanctuaries or
complete the Hollow storyline.

## Wards along the outer wall

### Banner Ward

The western ward serves analysts, caravan guards, performers, and travelers using
Ribbonway Crossroads. Silver route plates toward Mirror share posts with bright
Ribbonfair cloth, while teal Keep markers remain the reliable base layer.

The ward contains small boarding houses, a route rehearsal yard, and a public
deck clinic specializing in coherent mixed decks. Its west gate remains closed
until the Confluence relay is repaired.

### Courier Ward

The northeastern ward contains dispatch desks, fast-message lockers, repair
stores, and a sheltered courtyard for Stormbreak runners. Wind flags become more
prominent toward Courier Rise.

After the outer tier opens, Natasha's couriers move through this ward. Their
reports later help distinguish real Stormbreak weather from the Curator's copied
forecasts.

### Scholar Ward

The eastern ward supports travelers bound for Mirror and Sunspire. Reading rooms,
mirror-calibration shops, legal scribes, and quiet inns distinguish it from the
busier caravan wards.

Gold bell markers lead toward Sunspire while silver plates lead toward Mirror.
Both share Sunmirror Causeway outside the gate, so signs must make the three-exit
connector clear before the player leaves town.

### North Ward

The northern ward began as the Keep's most secure military quarter. It now holds
old relay records, Archive researchers' lodgings, and the heavily controlled
Archive Gate.

The ward is accessible before the Hollow finale, but the road beyond its final
checkpoint remains sealed. This lets the player understand where the Archive is
without entering early or treating its eventual opening as teleportation.

### Wall homes and service towers

Families of wardens, relay workers, traders, and craftspeople live inside thick
wall sections and in houses against the inner curtain. The town should not feel
like a castle occupied only by guards.

At least three residential interiors should be available:

- A multi-generational gatekeeper home with retired route signs
- A relay apprentice dormitory and shared study room
- A caravan family apartment above a storage arcade
- A wall gardener's home beside sheltered food beds

Laundry, cooking smoke, children's route games, and small gardens help soften
the fortress architecture.

## The five external gates

### South Gate — Fivefold Highway and Emberglen

The south gate is always the player's primary approach. Travel between Dragon
Keep and Emberglen remains bidirectional before and after the midpoint. Outer
network closure must never block this return route.

### West Gate — Ribbonway Crossroads

The west gate opens after the Confluence relay is restored. It leads to the
shared route for Mirror Arena and Ribbonfair. Silver-and-painted route markers
make both branches visible without making Ribbonfair appear mandatory.

### Northeast Gate — Courier Rise

The northeastern gate leads toward Stormbreak Plateau. Teal-and-indigo flags,
rope handrails, and weather notices begin inside the Courier Ward before the
terrain climbs outside the Keep.

### East Gate — Sunmirror Causeway

The eastern gate leads to the shared route for Mirror Arena and Sunspire
Cloister. Silver and gold markers remain distinct from the gate onward. Sunspire
is required for the Hollow story but does not contain a Crest.

### Archive Gate — Archive Approach

The northern gate opens only for the Hollow finale after the required Crests and
Resonance Seals are secured. Its mechanism verifies physical authorization,
current stability, and road safety; the Confluence Crest alone is insufficient.

After the finale, Archive Approach remains bidirectional. The player can leave,
return to Mercer, and continue toward Crown Gate through Mirror without losing
access to the restored Archive.

## Suggested NPC placement

- **Kestrel Vane:** Warden Hall, the trial gatehouses, Fivefold Court, and the
  Observatory as story stages advance
- **Relay Master Iona Strake:** Watchtower signal floor and Separator Hall;
  values archived baselines more than Kestrel initially does
- **Archivist Pell Marr:** Route Map Hall and Baseline Archive; identifies the
  retired Collector branch
- **Scale Warden Rusk Talon:** Scale Gate; Dragonoid setup and Dragon evolution
- **Furnace Warden Cala Pyre:** Furnace Gate; high-cost Fire cards and careful
  mana charging
- **Wing Warden Jori Ash:** Wing Gate; speed attackers and commitment timing
- **Mara Fiveways:** Lower Bailey route board; explains gate availability
- **Orla Wyrmglass:** Wyrm and Waystone Cardhouse; Dragon deck advice and shop
  services
- **Tenn Hearth:** Five Roads House; tracks delayed travelers and regional news
- **Chief Collector agent:** Hidden bypass and later Archive Gate encounter;
  masks a control deck behind copied Dragon pressure

Generic road wardens patrol broad wall walks and courts. Relay workers stay near
their assigned instruments. Caravan NPCs should use the lower bailey lanes
without crossing shop doors or blocking the stair to the central ward.

## Story-state changes

### Before the midpoint crisis

- South Gate and the lower castle-town are accessible from Emberglen
- West, northeast, and east outward checkpoints remain staffed but closed
- Archive Gate is visibly sealed
- Fivefold Court hosts ordinary civic activity
- Kestrel is available for local dialogue but not the official Crest match
- The trial gatehouses can be inspected, with their formal challenges locked

### After earning the first-tier Crests

- Corrupted relay reports disagree with physical freight and courier records
- Route lamps display brief impossible combinations
- Delayed caravans gather in the lower bailey and shelter hall
- Kestrel opens the three internal trial gatehouses
- The undercroft observation balcony becomes accessible
- The player can complete the gatehouses in any order

### During relay repair

- Each completed trial supplies a known test condition
- Dragon calibration produces an abnormally loud but apparently valid signal
- Old route plates reveal activity on a retired Archive branch
- The hidden bypass opens as a short investigation area
- All essential Keep services and the road back to Emberglen remain available

### After defeating Kestrel

- The player receives the Confluence Crest and Uberdragon Bajula
- Kestrel and relay staff authorize the restored outer checkpoints
- West, northeast, and east gates open permanently
- Corresponding road-status lamps activate around Fivefold Court
- Regional travelers begin appearing in the market and outer wards
- Kestrel offers stronger mono-Fire Dragon rematches

The Crest match and physical repair are both required by the story, but the Crest
does not magically operate the gates.

### During the second open-world tier

- The market and inn reflect which outer regions the player has visited
- Stormbreak courier reports arrive through the northeast ward
- Mirror analysts compare the false relay signal from two different roads
- Sunspire wardens inspect Light-aligned separator readings
- Gloam remains reachable through Emberglen or Cinderrail, not a new Keep gate

### Before the Hollow finale

- The Chief Collector attacks the Confluence relay and Archive Gate
- Five restored Seal references appear in Separator Hall
- Allies protect individual gates and service districts
- The Archive branch becomes readable but remains physically closed until the
  Collector encounter is resolved
- The player can still leave south to prepare at Mercer

### After the Hollow finale

- Archive Gate and Archive Approach remain open for backtracking
- The Baseline Archive displays both damaged and restored readings
- The Dragon calibration floor resumes ordinary testing under stricter review
- Fivefold Court holds a civic gathering rather than another mandatory duel
- Kestrel sends wardens and supplies toward the Championship route while
  continuing to operate the Keep

## Implementation notes

- Build Dragon Keep as a seamless exterior region with fixed boundary roads.
  Use portals only for true interiors such as the watchtower, Warden Hall, Dragon
  Gallery, and undercroft.
- Suggested surface allocation is approximately 42 by 38 tiles for the walled
  Keep, with the remaining footprint used for lower terraces and five road
  approaches. Preserve generous camera margins around the relay tower silhouette.
- Add stable semantic tile IDs for Keep paving, curtain wall, wall cap or roof,
  gate arch, dragon carving, route inlay, relay machinery, current glass, and
  undercroft floor. Do not reinterpret an existing tile glyph only inside this
  region.
- Castle wall and roof tiles need distinct IDs and art. Gate passages are
  walkable only when their saved story state allows them.
- Save outward checkpoint state separately from the three internal trial states.
  Completing Scale Gate must not accidentally open a regional road.
- The South Gate return to Emberglen remains available in every normal story
  state. Validate this path before accepting a world save.
- Exterior gate changes use doors, barriers, guards, and signs. Never move road
  boundaries or teleport the player when the outer tier opens.
- Fivefold Court's duel floor uses the normal duel transition and ordinary rules.
  It does not apply environmental buffs or hidden Dragon bonuses.
- The Confluence is rendered through restrained animated inlays and instruments.
  Animation must stay behind characters and prompts, respect reduced-flash
  settings, and be culled off-screen.
- Dragon statues are static architecture. Any manifested Dragon appears only
  inside a bounded duel or an explicitly anchored story calibration scene.
- Keep all major NPCs, doors, trial controls, and route signs on broad landings.
  No required interaction belongs on a one-tile wall walk.
- The undercroft must not resemble a sixth sanctuary. It has no Resonance Seal,
  and its machinery compares currents rather than defining their healthy local
  relationships.
- Preserve the distinction between location and phenomenon in code and dialogue:
  `Dragon Keep` is the map region, `the Confluence` is beneath it, and
  `confluence` may remain the stable ID for the Confluence Crest unless a later
  migration explicitly changes saved data.
