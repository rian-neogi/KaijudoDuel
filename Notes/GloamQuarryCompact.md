# Gloam Quarry compact layout

Gloam's exterior now uses smaller buildings and shorter streets, taking
Glasswater's house scale and closer spacing as the reference.

- Town bounds: `(508,850,90,72)`, down from `(488,850,128,96)`; 47% less area.
- Canvas Row homes: 6×5 building footprints, down from 9×9. The clinic, shops,
  office, and workshops also have smaller frontages and yards.
- Three quarry terraces, Slatecross, market tents, Hearth Court, the hoist,
  memorial furnaces, Namewall Walk, arena, and Ashvault remain recognizable.
- All fourteen furnished interiors and their interior endpoints are unchanged.
  Twenty-eight directed portals connect the relocated exterior doors and returns.
- Eight exterior flame fixtures moved with their landmarks, retaining their IDs
  and appearances. Six interior flame fixtures stay in place.
- The northern approach still joins Blackstone Road at `(535..537,849..850)`.
  The eastern road crosses the new town boundary at `(597..598,879..881)` and
  meets the existing Kiln Descent road at `(615..616,879..881)`.
- Kiln Descent's region expands west to `(598,755,98,133)` to include the eastern
  approach. Former town margins become surrounding quarry rock. Tile edits stay
  entirely inside Gloam's original rectangle.

[Current overview](GloamQuarryCompact.png)

The town has since gained [fifteen residents](GloamQuarryResidents.md), with a
[populated overview](GloamQuarryResidents.png). The compaction measurements and
patch below record the layout before those NPC placements.

## Useful coordinates

These are walkable approaches; stand south of a visible door and interact north.

| Place | Approach `(x,y)` |
| --- | --- |
| Quarry Office | `(538,866)` |
| Slatecross Terrace | `(545,889)` |
| Cardhouse | `(517,882)` |
| Shale and Spoon | `(559,883)` |
| Canvas Row homes | `(516,892)`, `(526,892)`, `(536,892)` |
| Stoneworks | `(575,891)` |
| Blast Office | `(588,891)` |
| Clinic/bathhouse | `(517,906)` |
| School | `(529,905)` |
| Record House | `(540,906)` |
| Namewall Walk | `(553,904)` |
| Ashen Arena | `(579,905)` |
| Lamplighter Workshop | `(516,917)` |
| Challenger Hall | `(578,919)` |
| Ashvault entrance | `(547,918)` |

## Helper workflow and checks

The edit uses `Tools/world_builder.py` queries, asset previews, scoped patches,
building stamps, roads, fixture moves, region edits, and paired entrances.
[The applied patch](GloamQuarryCompact.patch.json) retains the pre-edit revision
guard; it records this specific migration and should not be replayed on a later
world without review. No palette or RLE data was edited manually.

The candidate was reviewed with `apply --dry-run --preview`, applied to a
temporary world, and checked before application to the live World files.
The patch requires 32 town routes plus 28 automatically checked entrance routes.
The finished town has 4,302 reachable walkable cells with no isolated walkable
cells. Every door, return, and flame fixture is accessible. Each of the fourteen
interiors also passes reachability validation from its arrival cell.

Decoded comparisons confirm that only the overworld tiles inside the old town,
the two region rectangles, twenty-eight portals, and eight object positions
changed. All interior map files, other entity positions, object metadata, and
terrain outside the old town are preserved.

`ctest --test-dir Build --output-on-failure` passes all six suites, including
native world loading and the Python/native tile comparison tests.

Useful commands for the current layout:

```bash
python3 Tools/world_builder.py render --region gloam --output /tmp/gloam.png
python3 Tools/world_builder.py validate --region gloam --from 545 889
python3 Tools/world_builder.py path --region gloam --from 545 889 --to 547 917 --interact
```

## Remaining authoring friction

- Existing portal pairs have indices rather than stable IDs and no move-pair
  command. Relocating a building required removing pairs in descending index
  order and recreating them while preserving interior endpoints.
- Route validation checks destinations, but does not flag unused isolated
  walkable pockets. A full walkable-cell comparison found a thirteen-cell strip
  trapped behind the arena; it was filled with adjoining cliff terrain.
- The real-world helper test assumed at least 8,000 walkable town cells and the
  original fixed rectangle. It now uses the current region and checks access to
  its entrances and fixtures without imposing a minimum town size.

The original direct-JSON experiment and its earlier friction notes remain in
[the implementation log](GloamQuarryImplementationLog.md).
