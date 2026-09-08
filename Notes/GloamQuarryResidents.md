# Gloam Quarry residents

Fifteen residents populate the compact town: nine outdoors and six in five
interiors. Each has a distinct appearance, greeting, and everyday conversation.
All are town NPCs; duel challenges are voluntary.

| Resident | Role and location | Native position | Options beyond Talk |
| --- | --- | --- | --- |
| Sister Vey (`vey`) | Ashen Crest holder, arena | `overworld (579,902)` | Duel |
| Dema (`dema`) | Quarry blaster, practice apron | `overworld (587,883)` | Duel |
| Rell (`rell`) | Lamplighter, Namewall Walk | `overworld (556,903)` | Duel |
| Ivo (`ivo`) | Epitaph keeper, Record House | `gloam_record_house (8,7)` | Duel |
| Shopkeeper Nera (`gloam_nera`) | Ash and Ember card merchant | `gloam_cardhouse (6,6)` | Trade |
| Steward Ossa (`gloam_ossa`) | Directions at the Slatecross board | `overworld (543,882)` | — |
| Nella (`gloam_nella`) | Produce seller, Stone Market | `overworld (531,881)` | — |
| Beren (`gloam_beren`) | Hoist operator, loading yard | `overworld (561,873)` | — |
| Tovin (`gloam_tovin`) | Stone carver, Stoneworks apron | `overworld (581,891)` | — |
| Eska (`gloam_eska`) | Retired hoist operator, Hearth Court | `overworld (557,889)` | — |
| Mina (`gloam_mina`) | Schoolchild, Hearth Court | `overworld (555,891)` | — |
| Jessa (`gloam_jessa`) | Innkeeper, Shale and Spoon | `gloam_inn (9,13)` | — |
| Orin (`gloam_orin`) | Cook, Shale and Spoon | `gloam_inn (13,6)` | — |
| Iona (`gloam_iona`) | Clinic attendant | `gloam_clinic (8,8)` | — |
| Sella (`gloam_sella`) | Schoolteacher | `gloam_school (10,6)` | — |

[Populated town overview](GloamQuarryResidents.png)

Vey uses the existing `NPC/Sister Vey.txt` deck and awards the Ashen Crest,
Death Phoenix, Avatar of Doom, and Tier 2 gold on victory. Dema, Rell, and Ivo
use the existing `FD Generic 1.txt`, `LD Late.txt`, and `WD Generic 1.txt` decks,
respectively. Their rewards follow the trainer notes: Jack Viper, Shadow of
Doom; Stinger Worm; and Terror Pit, each with Tier 1 gold. Each duelist offers
one rewarded victory. Nera uses the existing `gloam_quarry` regional shop stock.

Dema, Rell, Nella, and Mina wander within their native three-by-three home
areas. Other residents stay at their work or conversation positions. Placement
checks reserve the entire four wandering areas simultaneously and confirm that
doors, arrivals, fixtures, stationary residents, and both town approaches remain
reachable. No roaming area includes a portal endpoint.

The placement was made with the Python helper's `entity_move` operations after
adding metadata to `Lua/Npcs.lua`. The revision-guarded
[placement patch](GloamQuarryResidents.patch.json) records the fifteen additions
and 63 required routes. It writes only `World/World.json`; all map tiles,
existing NPC positions, portals, regions, and other entities are preserved.
The patch applies to the pre-resident world revision and requires these NPC
metadata entries to exist.

The town has 4,293 reachable walking cells with the nine outdoor residents at
their native positions. All fifteen interactions and all fourteen interior
arrivals/exits pass reachability checks. There are no isolated walkable town
cells after placement.

`ctest --test-dir Build --output-on-failure` passes all six suites, including
runtime NPC/deck loading, world loading, and Python/native helper checks.
