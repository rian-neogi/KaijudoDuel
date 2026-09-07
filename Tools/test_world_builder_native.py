#!/usr/bin/env python3
"""Compare Python authoring semantics with the actual C++ loader and renderer.

The small oracle is compiled in a temporary directory, not shipped as a second
world builder. It links the project's unmodified native implementation.
"""
from dataclasses import replace
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

from llm_world.catalog import Catalog, FAMILIES, COUNTS, LAYERS, Tile, quarter, compatible, transition, source_rect
from llm_world.edits import Patch
from llm_world.metadata import Metadata
from llm_world.storage import World, commit, read_json
from test_world_builder import fixture, ROOT


ORACLE = r'''
#include "App/WorldStorage.h"
#include "App/RtpTilesetRenderer.h"
#include "App/AssetManager.h"
#include <SDL_image.h>
#include <iostream>
int main(int argc, char** argv) {
 if(argc<2) return 2;
 std::string mode=argv[1];
 if(mode=="catalog") {
  for(auto sheet:RtpTilesetRenderer::availableSheets()) for(int i=0;i<sheet.tileCount;++i) {
   RtpTileReference t(sheet.family,sheet.sheet,i);SDL_Rect r;
   RtpTilesetRenderer::paletteTileSource(sheet.sheet,i,r);
   std::cout<<"T "<<(int)sheet.family<<" "<<(int)sheet.sheet<<" "<<i<<" "
    <<(int)RtpTilesetRenderer::inferredLayer(t)<<" "<<(int)RtpTilesetRenderer::collision(t)<<" "
    <<RtpTilesetRenderer::canonicalTileIndex(sheet.family,sheet.sheet,i)<<" "<<r.x<<" "<<r.y<<" "<<r.w<<" "<<r.h<<"\n";
  }
  for(int kind=0;kind<3;++kind)for(int mask=0;mask<256;++mask)for(int q=0;q<4;++q) {
   SDL_Point p;
   if(kind==0) RtpTilesetRenderer::floorQuarterSource(q,mask,p);
   if(kind==1) RtpTilesetRenderer::wallQuarterSource(q,mask,p);
   if(kind==2) RtpTilesetRenderer::waterfallQuarterSource(q,mask,p);
   std::cout<<"Q "<<kind<<" "<<mask<<" "<<q<<" "<<p.x<<" "<<p.y<<"\n";
  }
  for(int a=0;a<32;++a)for(int b=0;b<32;++b) {
   RtpTileReference first(RtpTilesetFamily::Outside,RtpTileSheet::A2,a),second(RtpTilesetFamily::Outside,RtpTileSheet::A2,b),out=first;
   bool found=RtpTilesetRenderer::automaticGroundTransition(first,second,out);
   std::cout<<"G "<<a<<" "<<b<<" "<<RtpTilesetRenderer::autotileCompatible(first,second)<<" "<<(found?out.index:-1)<<"\n";
  }
  return 0;
 }
 if(mode=="load") {
  WorldData world;std::string error;
  if(argc!=4||!WorldStorage::load(argv[2],world,error)||!WorldStorage::save(argv[3],world,error)) {std::cerr<<error;return 1;}
  std::cout<<world.maps.size()<<"\n";return 0;
 }
 if(mode=="draw") {
  SDL_Init(0);IMG_Init(IMG_INIT_PNG);
  SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,768,768,32,SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* renderer=SDL_CreateSoftwareRenderer(surface);
  SDL_SetRenderDrawColor(renderer,23,25,31,255);SDL_RenderClear(renderer);
  {
   AssetManager assets(renderer);RtpTilesetRenderer tiles(renderer,&assets);
   int f,s,i,l,r,g,b,mask,n=0;
   while(std::cin>>f>>s>>i>>l>>r>>g>>b>>mask) {
    RtpTileReference t((RtpTilesetFamily)f,(RtpTileSheet)s,i,(RtpRenderLayer)l,r,g,b);
    SDL_Rect dest={(n%8)*96+64,(n/8)*96+64,32,32};++n;
    if(!tiles.draw(t,mask,dest,0))return 3;
   }
   SDL_RenderPresent(renderer);if(IMG_SavePNG(surface,argv[2])!=0)return 4;
  }
  SDL_DestroyRenderer(renderer);SDL_FreeSurface(surface);IMG_Quit();SDL_Quit();return 0;
 }
 return 2;
}
'''


class NativeParityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler=os.environ.get("WORLD_BUILDER_CXX","g++")
        if not shutil.which(compiler) or not shutil.which("sdl2-config"):
            raise unittest.SkipTest("Native comparisons require the game's C++ compiler and SDL2 development tools")
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        cls.directory=Path(cls.temp.name);cls.oracle=cls.directory/"oracle"
        source=cls.directory/"oracle.cpp";source.write_text(ORACLE)
        flags=shlex.split(subprocess.check_output(["sdl2-config","--cflags","--libs"],text=True))
        command=[compiler,"-std=c++14","-O0","-I",str(ROOT/"Source"),str(source)]
        command += [str(ROOT/"Source/App"/(name+".cpp")) for name in ("RtpTilesetRenderer","AssetManager","WorldData","WorldStorage","CatalogMapStorage")]
        result=subprocess.run(command+flags+["-lSDL2_image","-o",str(cls.oracle)],cwd=ROOT,text=True,capture_output=True)
        if result.returncode:raise RuntimeError(result.stderr)
        cls.catalog=Catalog(ROOT/"Resources/Graphics")

    def native(self,*args,input=None):
        result=subprocess.run([str(self.oracle),*map(str,args)],input=input,cwd=ROOT,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        return result.stdout

    def test_all_catalog_indices_collision_layers_quarters_and_transitions(self):
        counts={"T":0,"Q":0,"G":0}
        for row in self.native("catalog").splitlines():
            kind,*values=row.split();v=list(map(int,values));counts[kind]+=1
            with self.subTest(row=row):
                if kind=="T":
                    f,s,i,layer,collision,canonical,x,y,w,h=v
                    tile=self.catalog.parse(f"{FAMILIES[f]}/{list(COUNTS)[s]}/{i}",canonical=False)
                    self.assertEqual(tile.layer,LAYERS[layer])
                    self.assertEqual(self.catalog.collision(tile),("inherits","walkable","blocked")[collision])
                    self.assertEqual(self.catalog.canonical(tile).index,canonical)
                    self.assertEqual(source_rect(tile),(x,y,w,h))
                elif kind=="Q":
                    format,mask,q,x,y=v
                    self.assertEqual(quarter(("floor","wall","waterfall")[format],q,mask),(x,y))
                else:
                    a,b,match,index=v
                    a,b=Tile("Outside","A2",a,"ground"),Tile("Outside","A2",b,"ground")
                    self.assertEqual(compatible(a,b),bool(match))
                    result=transition(a,b)
                    self.assertEqual(result.index if result else -1,index)
        self.assertEqual(counts["T"],sum(len(names) for names in self.catalog.names.values()))
        self.assertEqual(counts["Q"],3072);self.assertEqual(counts["G"],1024)

    def test_created_world_and_legacy_composites_survive_native_load(self):
        directory=self.directory/"world_test";directory.mkdir()
        root,lua=fixture(directory,self.catalog)
        world=World(root,self.catalog)
        doc=read_json((ROOT/"Tools/examples/workshop.patch.json").read_bytes())
        doc["scope"].append(dict(map="overworld",rect=[0,0,20,16]))
        doc["operations"].append(dict(op="entrance",appearance="!Door3-5",outside_door=dict(map="overworld",at=[10,5]),outside_return=dict(map="overworld",at=[10,6]),
                                     inside_arrival=dict(frame="room",at=[5,7]),inside_exit=dict(frame="room",at=[5,9])))
        Patch(world,Metadata(lua),doc).execute()
        # Include legacy aliases, stacked lamps, layered conflicts and crowded trees.
        m=world.map("overworld")
        for x,y,index,layer in ((6,6,114,"ground"),(6,6,73,"decoration"),(8,8,112,"decoration"),(9,8,128,"decoration"),(0,0,112,"decoration"),
                                (12,7,211,"foreground"),(12,8,219,"foreground"),(12,9,227,"foreground"),(14,9,94,"foreground")):
            m.set(layer,x,y,self.catalog.parse(f"Outside/B/{index}",layer=layer,canonical=False))
        commit(world,world.changed_files())
        out=directory/"roundtrip";(out/"Maps").mkdir(parents=True)
        self.native("load",root/"World.json",out/"World.json")
        restored=World(out,self.catalog)
        for id,a in world.maps.items():
            a=a.view();b=restored.map(id).view()
            self.assertEqual(a.tags,b.tags)
            for layer in LAYERS:
                self.assertEqual([a.palette[i] for i in a.grids[layer]],[b.palette[i] for i in b.grids[layer]],f"{id}/{layer}")
        self.assertEqual(world.doc["portals"],restored.doc["portals"])
        self.assertEqual(world.doc["start"],restored.doc["start"])

    def test_pixels_match_native_renderer(self):
        try:from PIL import Image,ImageChops
        except ImportError:self.skipTest("Optional Pillow is not installed")
        from llm_world.render import Renderer
        renderer=Renderer(self.catalog)
        tiles=[self.catalog.parse(key) for key in ("Outside/A2/0","Outside/A2/2","Outside/A3/1","Inside/A4/0","Inside/A4/8","Outside/A1/0","Outside/A1/5","Outside/B/93","Outside/B/112","Outside/B/128","Outside/B/157","Outside/B/168","Outside/B/172","Outside/B/227","Outside/B/251","Inside/C/52")]
        cases=[(tile,mask) for tile in tiles for mask in (0,255,93)]
        cases += [(replace(tiles[0],tint=(133,180,211)),mask) for mask in (0,255)]
        data="\n".join(" ".join(map(str,(FAMILIES.index(t.tileset),list(COUNTS).index(t.sheet),t.index,LAYERS.index(t.layer),*t.tint,mask))) for t,mask in cases)
        output=self.directory/"native.png";self.native("draw",output,input=data)
        expected=Image.new("RGBA",(768,768),(23,25,31,255))
        for n,(tile,mask) in enumerate(cases):
            sprite=renderer.tile(tile,mask)
            expected.alpha_composite(sprite,((n%8)*96+96-sprite.width,(n//8)*96+96-sprite.height))
        with Image.open(output) as native:
            difference=ImageChops.difference(native.convert("RGB"),expected.convert("RGB"))
            # SDL's software renderer uses fixed-point alpha/color multiplies;
            # its tinted/translucent edge pixels can differ by three RGB levels.
            self.assertLessEqual(max(channel[1] for channel in difference.getextrema()),3)


if __name__=="__main__":unittest.main()
