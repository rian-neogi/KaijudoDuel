#!/usr/bin/env python3
"""Authoring regressions, always on temporary copies; never edits the live World."""
from copy import deepcopy
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch as mock_patch

from llm_world.catalog import Catalog, WorldError, LAYERS
from llm_world.edits import Patch
from llm_world.metadata import Metadata
from llm_world.queries import Navigation, cell, free_space, spatial_diff, validate
from llm_world.storage import World, MapData, commit, undo, encode_document, read_json
import llm_world.storage as storage

ROOT = Path(__file__).resolve().parent.parent


def fixture(directory, catalog):
    root=Path(directory)/"World"; (root/"Maps").mkdir(parents=True)
    m=MapData.create("overworld","Test World",20,16,False,catalog)
    for y in range(m.height):
        for x in range(m.width):m.set("ground",x,y,catalog.parse("Outside/A2/0"))
    (root/"Maps/overworld.json").write_bytes(m.encoded())
    doc=dict(format="kaijudo-world",version=1,maps=["Maps/overworld.json"],regions=[],portals=[],start=dict(map="overworld",x=1,y=1),
             entities=dict(npcs=[dict(id="npc",map="overworld",x=2,y=2)],objects=[dict(id="chest",map="overworld",x=3,y=2)],shards=[dict(id="shard",map="overworld",x=4,y=2)]))
    (root/"World.json").write_bytes(encode_document(doc))
    lua=Path(directory)/"Lua";lua.mkdir()
    (lua/"Npcs.lua").write_text('return {{ id="npc", name="Test NPC", appearance="Actor2-3", kind="town" }}')
    (lua/"Objects.lua").write_text('WorldObjectTemplates={{id="chest",name="Chest",kind="chest",appearance="!Chest-1"}}\nreturn {{id="chest",name="Authored Chest",kind="chest",appearance="!Chest-1"}}')
    (lua/"MercerStock.lua").write_text('return {shards={{id="shard",name="Shard"}}}')
    return root,lua


class AuthoringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.catalog=Catalog(ROOT/"Resources/Graphics")

    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root,self.lua=fixture(self.temp.name,self.catalog)
        self.world=World(self.root,self.catalog);self.metadata=Metadata(self.lua)
        self.before=dict(self.world.before)

    def document(self,ops,**extra):
        return dict(version=1,scope=[dict(map="overworld",rect=[0,0,20,16])],operations=ops,**extra)

    def apply(self,doc):
        req=Patch(self.world,self.metadata,doc).execute()
        checked=validate(self.world,self.metadata,requirements=req)
        self.assertTrue(checked["valid"],checked)
        return commit(self.world,self.world.changed_files())

    def cli(self,*args,code=0):
        result=subprocess.run([sys.executable,str(ROOT/"Tools/world_builder.py"),"--world-dir",str(self.root),"--lua-dir",str(self.lua),"--compact",*map(str,args)],cwd=self.temp.name,text=True,capture_output=True)
        self.assertEqual(result.returncode,code,result.stdout+result.stderr)
        self.assertFalse(result.stderr,result.stderr)
        return json.loads(result.stdout)

    def assert_disk_unchanged(self):
        self.assertEqual(World(self.root,self.catalog).before,self.before)

    def test_real_world_metadata_and_gloam_access(self):
        world=World(ROOT/"World",self.catalog)
        m,bounds=world.selection(region="gloam")
        result=validate(world,Metadata(ROOT/"Lua"),[545,889],m.id,bounds)
        self.assertTrue(result["valid"],result)
        # Check access to every current entrance and fixture, independent of town size.
        self.assertGreater(result["navigation"]["reachable_cells"],0)
        self.assertFalse(world.changed_files())

    def test_json_rle_layer_and_duplicate_key_errors(self):
        doc=read_json(self.before["Maps/overworld.json"])
        cases=[]
        bad=deepcopy(doc);bad["layers"]["ground"]["data"]=[[1,319]];cases.append(bad)
        bad=deepcopy(doc);bad["layers"]["ground"]["data"]=[[1,321]];cases.append(bad)
        bad=deepcopy(doc);bad["layers"]["ground"]["data"]=[[99,320]];cases.append(bad)
        bad=deepcopy(doc);bad["palette"][1]["layer"]="foreground";cases.append(bad)
        bad=deepcopy(doc);bad["width"]=True;cases.append(bad)
        bad=deepcopy(doc);del bad["palette"][1]["tint"];cases.append(bad)
        for bad in cases:
            with self.subTest(bad=bad):
                with self.assertRaises(WorldError):MapData(bad,self.catalog)
        with self.assertRaises(WorldError):read_json('{"a":1,"a":2}')
        with self.assertRaises(WorldError):read_json('{"a":NaN}')

    def test_noop_preserves_noncanonical_runs_tags_and_unknown_fields(self):
        doc=read_json(self.before["Maps/overworld.json"])
        doc["layers"]["ground"]["data"]=[[1,100],[1,220]]
        doc["tags"]=[dict(x=9,y=9,value="later"),dict(x=0,y=0,value="earlier")]
        doc["author_note"]="preserve me"
        (self.root/"Maps/overworld.json").write_bytes(json.dumps(doc).encode())
        world=World(self.root,self.catalog)
        world.map("overworld").set("ground",5,5,self.catalog.parse("Outside/A2/0"))
        self.assertEqual(world.changed_files(),{})
        world.map("overworld").set("ground",5,5,self.catalog.parse("Outside/A2/8"))
        saved=read_json(world.changed_files()["Maps/overworld.json"])
        self.assertEqual(saved["tags"],doc["tags"]);self.assertEqual(saved["author_note"],"preserve me")

    def test_scope_protect_bounds_unknown_fields_and_final_validation(self):
        for op,extra in [
            (dict(op="fill",map="overworld",rect=[19,15,2,1],tile="Outside/A2/8"),{}),
            (dict(op="fill",map="overworld",rect=[9,9,1,1],tile="Outside/A2/8"),dict(protect=[dict(map="overworld",rect=[9,9,1,1])])),
            (dict(op="fill",map="overworld",rect=[9,9,1,1],tile="Outside/A2/8",typo=True),{}),
        ]:
            with self.subTest(op=op):
                with self.assertRaises(WorldError):Patch(self.world,self.metadata,self.document([op],**extra)).execute()
        doc=self.document([dict(op="fill",map="overworld",rect=[9,9,1,1],tile="Outside/A2/8")])
        doc["scope"][0]["rect"]=[0,0,5,5]
        with self.assertRaises(WorldError):Patch(self.world,self.metadata,doc).execute()
        self.assert_disk_unchanged()
        path=Path(self.temp.name)/"bad.json"
        path.write_text(json.dumps(self.document([dict(op="fill",map="overworld",rect=[2,2,1,1],tile="Outside/A4/8")])))
        result=self.cli("apply",path,code=1)
        self.assertEqual(result["written"],[])
        self.assertEqual(result["validation"]["issues"][0]["code"],"blocked_position")
        self.assert_disk_unchanged()

    def test_dry_run_preview_apply_undo_is_byte_exact(self):
        example=ROOT/"Tools/examples/workshop.patch.json"
        dry=self.cli("apply",example,"--dry-run")
        self.assertEqual(set(dry["would_write"]),{"World.json","Maps/llm_workshop_demo.json"})
        self.assert_disk_unchanged()
        result=self.cli("apply",example)
        self.assertTrue((self.root/"Maps/llm_workshop_demo.json").exists())
        self.cli("undo",result["receipt"])
        self.assertFalse((self.root/"Maps/llm_workshop_demo.json").exists())
        self.assert_disk_unchanged()

    def test_frames_anchors_tint_and_spatial_diff(self):
        before=World(self.root,self.catalog)
        doc=self.document([dict(op="fill",frame="town",rect=[1,1,2,2],tile=dict(tileset="Outside",sheet="A2",index=8,tint=[100,120,140])),
                           dict(op="entity_move",group="npcs",id="npc",position=dict(frame="town",at="plaza"))],
                          frames={"town":dict(map="overworld",origin=[10,5])},anchors={"plaza":dict(frame="town",at=[2,2])})
        self.apply(doc)
        diff=spatial_diff(before,self.world)
        self.assertEqual(diff["maps"][0]["changed_cells"],4)
        self.assertEqual(diff["maps"][0]["rect"],[11,6,2,2])
        self.assertEqual(diff["manifest"]["/entities/npcs"]["changed"],1)
        self.assertEqual(self.world.map("overworld").get("ground",12,7).tint,(100,120,140))

    def test_roads_keep_intersection_surfaces_and_clip_is_explicit(self):
        op=dict(op="road_network",map="overworld",paths=[[[2,8],[16,8]],[[9,4],[9,12]]],tile="Outside/A2/3",width=3,shoulder="Outside/A2/8",shoulder_width=1)
        self.apply(self.document([op]))
        m=self.world.map("overworld")
        for x in range(2,17):self.assertEqual(m.get("ground",x,8).key,"Outside/A2/3")
        for y in range(4,13):self.assertEqual(m.get("ground",9,y).key,"Outside/A2/3")
        self.assertEqual(m.get("ground",5,6).key,"Outside/A2/8")
        op=dict(op="path",map="overworld",points=[[19,10],[19,15]],width=3,tile="Outside/A2/3")
        with self.assertRaises(WorldError):Patch(self.world,self.metadata,self.document([op])).execute()
        op["clip"]=True
        Patch(self.world,self.metadata,self.document([op])).execute()
        # Reject distant endpoints before expanding a potentially enormous line.
        op["points"]=[[19,10],[1000000000,15]]
        with self.assertRaisesRegex(WorldError,"outside"):
            Patch(self.world,self.metadata,self.document([op])).execute()

    def test_tree_aliases_spacing_and_furniture_layers(self):
        ops=[dict(op="fill",map="overworld",rect=[8,8,1,1],tile="Outside/B/114"),
             dict(op="stamp",prefab="table_setting",map="overworld",at=[12,8],base="Inside/B/96",top="Inside/C/52")]
        self.apply(self.document(ops))
        m=self.world.map("overworld")
        self.assertEqual(m.get("decoration",8,8).key,"Outside/B/112")
        self.assertEqual(m.get("decoration",12,8).key,"Inside/B/96")
        self.assertEqual(m.get("foreground",12,8).key,"Inside/C/52")
        with self.assertRaisesRegex(WorldError,"culled"):
            Patch(self.world,self.metadata,self.document([dict(op="fill",map="overworld",rect=[9,8,1,1],tile="Outside/B/112")])).execute()

    def test_paired_entrance_and_enclosed_approach(self):
        doc=read_json((ROOT/"Tools/examples/workshop.patch.json").read_bytes())
        doc["scope"].append(dict(map="overworld",rect=[0,0,20,16]))
        doc["operations"].append(dict(op="entrance",appearance="!Door3-5",outside_door=dict(map="overworld",at=[10,5]),outside_return=dict(map="overworld",at=[10,6]),
                                     inside_arrival=dict(frame="room",at=[5,7]),inside_exit=dict(frame="room",at=[5,9])))
        self.apply(doc)
        self.assertEqual(len(self.world.doc["portals"]),2)
        nav=Navigation(self.world,self.world.map("overworld"))
        self.assertFalse(nav.path([1,1],[10,5])["found"])
        self.assertTrue(nav.path([1,1],[10,5],True)["found"])
        ops=[dict(op="fill",map="overworld",rect=r,tile="Outside/A4/8") for r in ([9,4,3,1],[9,7,3,1],[9,5,1,2],[11,5,1,2])]
        Patch(self.world,self.metadata,self.document(ops)).execute()
        self.assertTrue(validate(self.world,self.metadata)["valid"])
        result=validate(self.world,self.metadata,[1,1],"overworld")
        self.assertFalse(result["valid"])
        self.assertIn("unreachable",[i["code"] for i in result["issues"]])

    def test_legacy_composite_edits_require_scoped_normalization(self):
        m=self.world.map("overworld")
        m.set("foreground",8,8,self.catalog.parse("Outside/B/114",layer="foreground",canonical=False))
        erase=dict(op="erase",map="overworld",rect=[8,8,1,1],layer="decoration")
        with self.assertRaisesRegex(WorldError,"normalize"):
            Patch(self.world,self.metadata,self.document([erase])).execute()
        ops=[dict(op="normalize",map="overworld"),erase]
        self.apply(self.document(ops))
        self.assertIsNone(m.get("decoration",8,8));self.assertIsNone(m.get("foreground",8,8))
        self.assertEqual(m.collision(8,8)[0],"walkable")

    def test_invisible_portal_terminates_paths_and_old_gate_tags_do_not_block(self):
        m=self.world.map("overworld")
        for y in range(m.height):
            for x in range(m.width):m.set("ground",x,y,None)
        for x in range(8):m.set("ground",x,8,self.catalog.parse("Outside/A2/0"))
        self.world.doc["portals"]=[{"from":dict(map="overworld",x=4,y=8),"to":dict(map="overworld",x=0,y=8)}]
        nav=Navigation(self.world,m)
        self.assertTrue(nav.path([1,8],[4,8])["found"])
        self.assertFalse(nav.path([1,8],[6,8])["found"])
        self.world.doc["portals"]=[];m.tags[4,8]="blackstone_gate"
        self.assertTrue(Navigation(self.world,m).path([1,8],[6,8])["found"])
        self.assertTrue(cell(self.world,m,4,8)["walkable"])
        self.assertEqual(cell(self.world,m,4,8)["blockers"],[])
        m.set("decoration",4,8,self.catalog.parse("Outside/B/103"))
        self.assertFalse(Navigation(self.world,m).path([1,8],[6,8])["found"])

    def test_current_world_has_no_progression_obstacles(self):
        world=World(ROOT/"World",self.catalog)
        objects=Metadata(ROOT/"Lua").objects(world)
        self.assertFalse(any(o.get("kind") in ("cuttable_bush","smashable_rock") for o in objects.values()))
        self.assertFalse(any(tag=="blackstone_gate" for m in world.maps.values() for tag in m.tags.values()))
        m=world.map("overworld")
        for point in ((535,754),(536,754),(537,754),(472,689),(472,690),(366,685)):
            self.assertTrue(cell(world,m,*point)["walkable"],point)

    def test_metadata_templates_unknown_ids_and_overlap(self):
        self.apply(self.document([dict(op="object_add",id="new_chest",template="chest",position=dict(map="overworld",at=[8,8]),appearance="!Chest-4")]))
        self.assertEqual(self.metadata.objects(self.world)["new_chest"]["kind"],"chest")
        with self.assertRaisesRegex(WorldError,"No Lua metadata"):
            Patch(self.world,self.metadata,self.document([dict(op="entity_move",group="npcs",id="design_only",position=dict(map="overworld",at=[8,9]))])).execute()
        self.world.doc["entities"]["objects"][0].update(x=2,y=2)
        self.assertIn("occupied",[i["code"] for i in validate(self.world,self.metadata)["issues"]])

    def test_regions_and_free_space_consider_paint_and_claims(self):
        m=self.world.map("overworld")
        self.assertEqual(free_space(self.world,m,None,3,3),[])
        for y in range(8,16):
            for x in range(8,20):m.set("ground",x,y,None)
        self.world.doc["regions"]=[dict(id="reserved",name="Reserved",kind="town",map="overworld",x=8,y=8,width=6,height=8)]
        free=free_space(self.world,m,None,3,3,1)
        self.assertEqual(free,[[14,8,3,3]])
        self.assertEqual(free_space(self.world,m,None,3,3,1,True),[[8,8,3,3]])
        op=dict(op="region_set",map="overworld",id="overlap",name="Overlap",rect=[13,8,3,3],kind="connector")
        Patch(self.world,self.metadata,self.document([op])).execute()
        self.assertIn("region_overlap",[i["code"] for i in validate(self.world,self.metadata)["issues"]])

    def test_overlapping_copy_and_move_are_snapshot_operations(self):
        m=self.world.map("overworld")
        for x,index in enumerate((0,8,16),8):m.set("ground",x,8,self.catalog.parse(f"Outside/A2/{index}"))
        Patch(self.world,self.metadata,self.document([dict(op="move",map="overworld",source=dict(map="overworld",rect=[8,8,3,1]),at=[9,8],layers=["ground"])] )).execute()
        self.assertIsNone(m.get("ground",8,8))
        self.assertEqual([m.get("ground",x,8).index for x in (9,10,11)],[0,8,16])

    def test_stale_revision_concurrent_write_and_undo_conflict(self):
        doc=self.document([],expected_revision="stale")
        with self.assertRaisesRegex(WorldError,"Stale revision"):Patch(self.world,self.metadata,doc).execute()
        self.world.map("overworld").set("ground",8,8,self.catalog.parse("Outside/A2/8"))
        files=self.world.changed_files()
        (self.root/"World.json").write_bytes(self.before["World.json"]+b"\n")
        with self.assertRaisesRegex(WorldError,"changed"):commit(self.world,files)
        (self.root/"World.json").write_bytes(self.before["World.json"])
        receipt=commit(self.world,files)["receipt"]
        (self.root/"World.json").write_bytes(self.before["World.json"]+b"\n")
        with self.assertRaisesRegex(WorldError,"changed"):undo(World(self.root,self.catalog),receipt)

    def test_failed_multi_file_commit_rolls_back(self):
        example=read_json((ROOT/"Tools/examples/workshop.patch.json").read_bytes())
        Patch(self.world,self.metadata,example).execute()
        real=storage.atomic_write
        def fail_manifest(path,data):
            if Path(path)==self.root/"World.json":raise OSError("simulated disk failure")
            return real(path,data)
        with mock_patch.object(storage,"atomic_write",side_effect=fail_manifest):
            with self.assertRaisesRegex(OSError,"simulated"):commit(self.world,self.world.changed_files())
        self.assert_disk_unchanged();self.assertFalse((self.root/"Maps/llm_workshop_demo.json").exists())

    def test_unsafe_paths_and_output_locations_are_rejected(self):
        for name in ("../outside.json","/tmp/absolute.json","Maps/../../bad.json"):
            with self.assertRaises(WorldError):self.world.path(name)
        link=self.root/"escape";link.symlink_to(Path(self.temp.name))
        with self.assertRaises(WorldError):self.world.path("escape/file")
        self.cli("render","--rect",0,0,4,4,"--output",self.root/"preview.png",code=2)
        self.assertFalse((self.root/"preview.png").exists())

    def test_png_preview_and_contact_sheet(self):
        try:from PIL import Image
        except ImportError:self.skipTest("Optional Pillow is not installed")
        output=Path(self.temp.name)/"preview.png"
        result=self.cli("apply",ROOT/"Tools/examples/workshop.patch.json","--dry-run","--preview",output,"--map","llm_workshop_demo","--grid","--collision","--labels")
        self.assertEqual(result["preview"]["pixels"],[192,160]);self.assert_disk_unchanged()
        with Image.open(output) as image:self.assertEqual(image.size,(192,160))
        sheet=Path(self.temp.name)/"catalog.png"
        self.cli("catalog","bed","--family","Inside","--limit",3,"--png",sheet)
        with Image.open(sheet) as image:self.assertEqual(image.size,(1100,165))


if __name__=="__main__":unittest.main()
