#!/usr/bin/env python3
"""LLM-friendly native world queries and reversible edits. See Tools/WorldBuilder.md."""
import argparse
import json
from pathlib import Path
import sys

from llm_world.catalog import Catalog, FAMILIES, COUNTS, WorldError
from llm_world.edits import Patch, prefab_list
from llm_world.metadata import Metadata
from llm_world.queries import Navigation, cell, free_space, inspect, spatial_diff, summary, validate
from llm_world.storage import World, commit, external_output, read_json, undo

ROOT = Path(__file__).resolve().parent.parent


class Parser(argparse.ArgumentParser):
    def error(self, message):
        raise WorldError(message)


def parser():
    p = Parser(description=__doc__)
    p.add_argument("--world-dir",type=Path,default=ROOT/"World",help="Native World directory; defaults to this repository's World")
    p.add_argument("--graphics-dir",type=Path,default=ROOT/"Resources/Graphics")
    p.add_argument("--lua-dir",type=Path,default=ROOT/"Lua",help="Trusted game Lua metadata directory")
    p.add_argument("--compact",action="store_true",help="One-line JSON output")
    commands = p.add_subparsers(dest="command",required=True,parser_class=Parser)
    def select(c):
        c.add_argument("--map",help="Map ID, default overworld")
        c.add_argument("--region",help="Select a named region in global coordinates")
        c.add_argument("--rect",nargs=4,type=int,metavar=("X","Y","W","H"))
    def gates(c):
        c.add_argument("--open-gate",action="append",choices=("blackstone_gate",),default=[])
    def paginate(c):
        c.add_argument("--limit",type=int,default=20)
        c.add_argument("--offset",type=int,default=0)
    c=commands.add_parser("summary",help="Maps, regions, occupancy, entities and revision");select(c)
    c=commands.add_parser("inspect",help="Bounded ASCII walkability and occupant map");select(c);gates(c)
    c=commands.add_parser("cell",help="Explain layers, collision, occupants and tags at a cell")
    c.add_argument("--map",default="overworld");c.add_argument("--at",nargs=2,type=int,required=True);gates(c)
    c=commands.add_parser("space",help="Find unpainted, unoccupied rectangles");select(c)
    c.add_argument("--size",nargs=2,type=int,required=True,metavar=("W","H"));c.add_argument("--limit",type=int,default=5)
    c.add_argument("--allow-regions",action="store_true",help="Allow candidates overlapping existing region rectangles")
    c=commands.add_parser("catalog",help="Search real asset names, source rectangles, layers and collision")
    c.add_argument("query",nargs="?",default="");c.add_argument("--family",choices=FAMILIES);c.add_argument("--sheet",choices=COUNTS)
    c.add_argument("--aliases",action="store_true");c.add_argument("--png",type=Path);paginate(c)
    c=commands.add_parser("metadata",help="Query actual Lua NPCs, objects, shards and object templates")
    c.add_argument("group",choices=("npcs","objects","shards","templates"));c.add_argument("query",nargs="?",default="");paginate(c)
    commands.add_parser("prefabs",help="List supported building and furniture assemblies")
    c=commands.add_parser("path",help="Find a local route; visible doors require --interact");select(c);gates(c)
    c.add_argument("--from",dest="start",nargs=2,type=int,required=True);c.add_argument("--to",nargs=2,type=int,required=True)
    c.add_argument("--interact",action="store_true");c.add_argument("--cleared",action="append",default=[],help="Object/NPC IDs excluded from static obstacles")
    c.add_argument("--all-steps",action="store_true",help="Return every step instead of only corners")
    c=commands.add_parser("validate",help="Validate native data and metadata; optionally flood-fill from a point");select(c);gates(c)
    c.add_argument("--from",dest="start",nargs=2,type=int,help="Check every selected interaction/arrival is reachable")
    c=commands.add_parser("render",help="Render a PNG using actual game tiles and entity sprites");select(c);gates(c)
    c.add_argument("--output",type=Path,required=True)
    def rendering(c):
        c.add_argument("--tile-size",type=int,default=16,choices=(8,16,32,48))
        for flag in ("grid","collision","labels","regions"):c.add_argument("--"+flag,action="store_true")
    rendering(c)
    c=commands.add_parser("apply",help="Validate and apply a bounded JSON patch, with an undo receipt")
    c.add_argument("patch",type=Path);c.add_argument("--dry-run",action="store_true")
    c.add_argument("--receipt",type=Path,help="New receipt outside World; default temporary history")
    c.add_argument("--preview",type=Path,help="PNG of the candidate world, including in --dry-run")
    select(c);rendering(c)
    c=commands.add_parser("undo",help="Restore an edit byte-for-byte if its revision still matches");c.add_argument("receipt",type=Path)
    c=commands.add_parser("diff",help="Spatial semantic diff against a World directory copy")
    c.add_argument("--against",type=Path,required=True);c.add_argument("--limit",type=int,default=20)
    return p


def corners(path):
    if len(path)<3:return path
    result=[path[0]]
    for a,b,c in zip(path,path[1:],path[2:]):
        if (b[0]-a[0],b[1]-a[1])!=(c[0]-b[0],c[1]-b[1]):result.append(b)
    return result+[path[-1]]


def run(args):
    catalog = Catalog(args.graphics_dir)
    if args.command == "prefabs":return dict(prefabs=prefab_list()),0
    world = World(args.world_dir,catalog)
    metadata = Metadata(args.lua_dir)
    gates = {id:True for id in getattr(args,"open_gate",[])}
    for field in ("limit","offset"):
        n=getattr(args,field,None)
        if n is not None and (n<0 or (field=="limit" and not 1<=n<=200)):
            raise WorldError("Limit must be 1..200; offset must be non-negative")
    def selection():return world.selection(getattr(args,"map",None),getattr(args,"rect",None),getattr(args,"region",None))
    def render(path):
        from llm_world.render import Renderer
        m,bounds=selection()
        return Renderer(catalog).map(world,m,bounds,path,metadata,args.tile_size,args.grid,args.collision,args.labels,args.regions,gates)
    command=args.command
    if command=="catalog":
        all_rows=catalog.search(args.query,args.family,args.sheet,args.aliases)
        rows=all_rows[args.offset:args.offset+args.limit]
        result=dict(total=len(all_rows),offset=args.offset,results=rows)
        if args.png:
            from llm_world.render import Renderer
            result["png"]=Renderer(catalog).contact_sheet(rows,external_output(world,args.png))
    elif command=="metadata":
        rows=metadata.query(world,args.group,args.query)
        result=dict(total=len(rows),offset=args.offset,results=rows[args.offset:args.offset+args.limit])
    elif command=="summary":
        result=summary(world,*selection()) if args.map or args.rect or args.region else summary(world)
    elif command=="cell":result=cell(world,world.map(args.map),*args.at,gates)
    elif command=="inspect":result=inspect(world,*selection(),gates)
    elif command=="space":
        m,bounds=selection()
        result=dict(map=m.id,rect=list(bounds),available=free_space(world,m,bounds,*args.size,args.limit,args.allow_regions))
    elif command=="path":
        m,bounds=selection()
        result=Navigation(world,m,bounds,gates,args.cleared).path(args.start,args.to,args.interact)
        if not args.all_steps:result["path"]=corners(result["path"]);result["path_encoding"]="orthogonal corners"
        result.update(map=m.id,rect=list(bounds),gate_state=gates,cleared=args.cleared)
        result["revision"]=world.revision
        return result,0 if result["found"] else 1
    elif command=="validate":
        m,bounds=selection()
        result=validate(world,metadata,args.start,m.id,bounds,gates)
        result["revision"]=world.revision
        return result,0 if result["valid"] else 1
    elif command=="render":result=render(args.output)
    elif command=="diff":result=spatial_diff(World(args.against,catalog),world,args.limit)
    elif command=="undo":return undo(world,args.receipt),0
    elif command=="apply":
        before=World(args.world_dir,catalog)
        if world.revision!=before.revision:raise WorldError("World changed during loading; retry")
        document=read_json(sys.stdin.buffer.read() if str(args.patch)=="-" else args.patch.read_bytes())
        patch=Patch(world,metadata,document);requirements=patch.execute()
        gates=document.get("gates",{})
        validation=validate(world,metadata,gates=gates,requirements=requirements)
        result=dict(dry_run=args.dry_run,validation=validation,diff=spatial_diff(before,world))
        if args.preview:result["preview"]=render(args.preview)
        if not validation["valid"]:
            result.update(written=[],revision=world.revision)
            return result,1
        files=world.changed_files()
        if args.dry_run:result.update(would_write=list(files),revision=world.revision)
        else:result.update(commit(world,files,document.get("expected_revision"),args.receipt))
    else:raise WorldError(f"Unknown command: {command}")
    result.setdefault("revision",world.revision)
    return result,0


def main(argv=None):
    try:
        args=parser().parse_args(argv)
        result,code=run(args)
        print(json.dumps(result,indent=None if args.compact else 2,ensure_ascii=False,allow_nan=False))
        return code
    except (ValueError,OSError,KeyError,TypeError,AttributeError,IndexError) as exc:
        print(json.dumps(dict(error=str(exc),type=type(exc).__name__),ensure_ascii=False))
        return 2


if __name__=="__main__":
    sys.exit(main())
