# Gloam Quarry: direct JSON authoring experiment

This log records obstacles encountered while implementing Gloam Quarry with the
existing game and native World JSON format. It is an input to the proposed LLM
world builder, not a specification for that tool.

## Scope and initial findings

- Use `Notes/Gloam Quarry.md` for the physical town design. Create native terrain,
  buildings, explorable interiors, and road connections using existing assets.
- Preserve existing Lua metadata and game rules. The planned Gloam residents,
  regional shop, Ashen Crest battle, and Darkness Seal quest do not yet exist in
  runtime metadata. A map cannot implement those systems by adding coordinates.
- The working tree already contains an untracked `World (backup)/` directory.
  Leave it untouched.

## Friction observed during work

1. **Conflicting design documents.** `Notes/GameplayOrder.md` makes Gloam an early
   destination accessed through Cinderrail; the detailed town and connector notes
   describe later access and a Confluence-gated Blackstone Road. The runtime also
   has its own gate behavior. Physical authoring needs an explicit connection and
   progression contract. Preserve existing gates during this experiment.
2. **No concise spatial inventory.** The manifest and million-cell exterior must
   be decoded and summarized to find available space, existing road endpoints,
   and entities that must not move. Text searches do not answer these questions.
3. **Design names are not runtime entities.** Vey, Dema, Ivo, Rell, the regional
   merchant, and their content are described in prose but absent from Lua. Native
   positions must reference real metadata; inventing position IDs is invalid.

4. **Asset discovery requires code and image inspection.** Metadata repeats names
   such as “Wall (Stone)” across visibly different tiles. B/C indices are ordered
   in two 8-column pages, not ordinary image row order; autotile indices refer to
   assembled blocks. A searchable, labeled visual catalog would remove repeated
   manual cross-referencing.
5. **No headless map preview or collision query.** The CLI offers the interactive
   builder and a broad smoke test, but no crop renderer or walkability export.
   A temporary `/tmp/gloam-authoring/inspect.cpp` harness uses the existing native
   loader, collision rules, sprite renderer, and tile renderer without modifying
   the application. It exports actual asset-based previews and collision masks.
6. **The spatial manifest is incomplete as an authoring index.** Existing painted
   terrain extends beyond named regions. Empty region space cannot be assumed
   empty terrain; every proposed footprint and connector must be inspected.

7. **No reusable building or furnishing assemblies.** Roofs, facades, windows,
   chimneys, stairs, tents, beds, and shelves had to be assembled manually. Several
   sprites have two vertically adjacent pieces; their logical indices are eight
   apart. A named asset alone does not describe a complete building or object.
8. **Every spatial operation needs explicit boundary handling.** A three-tile
   path to the bottom edge of the Ashvault initially attempted to write beyond
   the map. The temporary authoring script caught this with an assertion; the
   brush was changed to clip its footprint to the map bounds.
9. **Portal authoring has four distinct positions.** Each interior connection
   needs an exterior door origin, an interior arrival, a separate interior exit
   origin, and an exterior return cell. Reusing the arrival as the return origin
   violates native endpoint uniqueness and can create immediate return triggers.
10. **Serialization obscures small edits.** A local brush changes long row-major
    RLE runs in a million-cell layer. A temporary deterministic Python recipe is
    used to decode, edit, and re-encode JSON; the game still consumes native JSON
    directly. Raw diffs need spatial checks to establish what actually changed.

11. **Region rectangles do not follow irregular roads.** The first Kiln Descent
    region overlapped the existing Old Road rectangle even though its road ran
    through free terrain. Native loading rejected the overlap. The approach now
    uses two adjacent, non-overlapping regions without changing existing regions.
12. **Walkable endpoints do not guarantee usable doors.** Native structural and
    placement validation passed, but a flood-fill with visible doors and fixtures
    treated as obstacles found the Ashvault's door apron isolated behind its
    retaining wall. Two broad approaches were cut around the arch. The same check
    found a hoist-yard landmark on a retaining face and an enclosed five-cell
    niche behind the inn counter; both were corrected.
13. **Visual selection needs iteration.** Tiles named “Cobblestones” produced
    green paving or sandy-looking surfaces, and the first slate floor was too
    pale. Engine-rendered previews led to different paving references, darker
    ground tints, and blue slate roofs. These choices cannot be judged from tile
    names or successful JSON loading alone.

14. **Overlapping brushes can damage visual continuity without blocking travel.**
    A side loop's dirt shoulder painted over portions of an existing paved road.
    Walking checks still passed. Rendering exposed the broken paving, and road
    surfaces were reapplied after all shoulders and clearings were finished.
15. **Layer composition is easy to get wrong.** Putting plates and books on the
    decoration layer replaced the tables underneath them. These small surface
    props now use explicit foreground entries above the blocked table tiles.
    A future assembly definition should encode those relative layers itself.
16. **Roads leaving previously painted land need surrounding terrain.** The new
    southern freight loop initially crossed empty canvas with no surrounding
    quarry face. Rendering caught this; blocked rock terrain now fills the new
    connector's southern extent around the traversable route.

17. **Raw JSON is a poor review surface.** The existing writer puts an entire
    layer's RLE on one line. Reading a few lines can return tens of thousands of
    tokens. A first serialization also changed formatting unnecessarily. Final
    output matches the existing formatting and preserves old palette indices;
    decoded spatial comparisons establish the actual scope of the edit.

## Implemented map content

- Gloam occupies `(488,850)` through `(615,945)` on `overworld`: 128 columns by
  96 rows. It has an upper quarry and civic office, a market/residential terrace,
  and a lower civic/memorial terrace, with broad ramps between elevations.
- Slatecross, the caravan yard, Stone Market, Hearth Court, the working cut,
  hoist structure, Namewall Walk, memorial furnaces, Ash Courtyard, and the Ashen
  Arena are represented with existing catalog artwork.
- Fourteen furnished interiors add 7,504 tile cells: office, cardhouse, inn,
  three homes, Stoneworks, Blast Office, clinic/bathhouse, school, Record House,
  Lamplighter Workshop, Challenger Hall, and the Ashvault.
- The 56-by-46 Ashvault has an entry gallery, two routes to the sanctuary,
  a memorial side gallery, a maintenance bay, and a Collector annex. Its Seal
  artwork has no invented quest behavior.
- Twenty-eight directed portals provide fourteen visible exterior entrances
  and fourteen intentional interior exit triggers. Arrival and exit cells are
  separate. Fourteen existing-template flame fixtures animate in gameplay.
- The northern town entrance meets Blackstone Road at `(535..537,849..850)`.
  The existing Blackstone gate artwork, tags, and progression behavior remain.
- Cinderrail's southern opening at `(670,732)` leads through Cinderrail Freight
  Road and Kiln Descent to Gloam's eastern approach at `(615,880)`. The route has
  two optional loops and a tent clearing. This new approach is ungated, following
  the early Cinderrail access described in `Notes/GameplayOrder.md`; the conflicting
  later-tier gating in the other documents was not added as new game logic.
- The future Lanternfen connection has a green-lantern outlook inside Gloam.
  It ends at a blocked boundary because Lanternfen is not implemented.

## Finding the area

Launch `./Bin/KaijudoDuel --world-builder`, select the Regions tab, and double-click
the Gloam Quarry row to center it. Interiors are available through the normal map
selector. In gameplay, leave Cinderrail through its southern freight opening and
follow Kiln Descent; Blackstone Road is the second approach when its gate permits.

Useful exterior inspection coordinates:

| Place | Walkable approach `(x,y)` |
| --- | --- |
| Quarry Office | `(538,874)` |
| Slatecross Terrace | `(545,889)` |
| Cardhouse | `(501,890)` |
| Shale and Spoon | `(568,890)` |
| Canvas Row homes | `(500,907)`, `(513,907)`, `(526,907)` |
| Working Cut | `(598,888)` |
| Stoneworks | `(588,905)` |
| Clinic/bathhouse | `(502,926)` |
| Record House | `(537,925)` |
| Namewall Walk | `(551,924)` |
| Ashen Arena | `(586,925)` |
| Ash Courtyard | `(546,937)` |
| Ashvault entrance | `(545,944)`; face north and interact |

[Town overview](GloamQuarryOverview.png) and [Ashvault overview](GloamQuarryAshvault.png)
were rendered with the existing native tile and sprite renderers. They show static
map composition, without the gameplay HUD, NPC population, or weather effects.

## Content that JSON placement cannot supply

There are no new Gloam NPCs, merchant stock, dialogue, readable custom signs,
trial duels, healing services, quest gates, Crest awards, or traversal rewards.
Those require Lua metadata or application behavior beyond this map-authoring
experiment. The shop, clinic, arena, and sanctuary are physical locations ready
for that content. The hoist is a static structure; ramps provide actual travel.
Existing NPCs, shards, objects, rewards, and save-referenced IDs were preserved.

## Verification

- Built the existing application successfully.
- Native loader accepts 18 maps, 10 regions, and 32 directed portals. Every
  entity, player start, portal origin, and arrival has valid supporting collision.
- Used engine-derived collision masks with NPCs, objects, and visible portal
  origins treated as movement obstacles. All 28 named inspection locations and
  both exterior approaches are reachable from Slatecross. Every new exterior
  door has a reachable interaction cell and return cell.
- All unoccupied walkable cells in each new interior are reachable from its
  arrival, including its separate return trigger. This is a geometry check,
  not a manual playthrough or verification of the unimplemented quests.
- Saved and reloaded the full world through the native serializer into a
  temporary directory. All maps retain their tile references, tints, layers, tags,
  and world placement tables.
- Decoded before/after comparison: 18,104 exterior cells changed, all within
  Gloam, the freight approach, or their joining seam. Existing region definitions,
  NPC/shard/object positions, portals, start, gate tags, and old palette entries
  remain identical. Unrelated existing map files and `World (backup)/` were not
  modified.
- Final `ctest --test-dir Build --output-on-failure`: all four tests passed
  (`cli_help`, `headless_ai_duel`, `smoke`, `legacy_world_converter`), in 69.99
  seconds. `git diff --check` also passed.

## Priorities suggested by this experiment

| Priority | Builder capability | Observed problem it addresses |
| --- | --- | --- |
| 1 | Native crop rendering plus collision/interaction queries | The Ashvault doorway was structurally valid but inaccessible; visually broken roads still passed path checks. |
| 2 | Labeled visual catalog and complete assemblies | Repeated wall names, paged tile indices, multi-part furniture, and layer mistakes required manual investigation. |
| 3 | Bounded spatial patches and preservation checks | Existing unregistered terrain, giant RLE lines, and road intersections made edit scope difficult to inspect. |
| 4 | Area briefs with explicit connections and known metadata | Contradictory progression documents and absent Gloam NPC definitions could not be resolved by tile placement. |
| 5 | Named features and paired entrance operations | Building approaches, four-cell portal arrangements, and region rectangles required repeated bookkeeping. |

The temporary authoring and inspection utilities live in `/tmp/gloam-authoring`.
They were used for this experiment only. The persistent implementation consists
of native World JSON files; no game C++ or Lua source was changed.
