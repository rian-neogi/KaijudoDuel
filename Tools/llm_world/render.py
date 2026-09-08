"""Headless asset-based previews. Pillow is needed only by image commands."""
from collections import Counter
from dataclasses import replace
from functools import lru_cache
from pathlib import Path
import re

from .catalog import LAYERS, TREES, LAMPS, WorldError, Tile, source_rect, autotile_origin, quarter, compatible, transition
from .queries import Navigation, positions
from .storage import external_output, inside

NEIGHBORS = ((0,-1),(1,0),(0,1),(-1,0),(-1,-1),(1,-1),(1,1),(-1,1))


class Renderer:
    def __init__(self, catalog):
        try:
            from PIL import Image, ImageDraw, ImageChops
        except ImportError as exc:
            raise WorldError("PNG commands need Pillow: install Tools/requirements-world-builder.txt") from exc
        self.Image, self.Draw, self.Chops = Image, ImageDraw, ImageChops
        self.catalog = catalog

    @lru_cache(maxsize=64)
    def sheet(self, path):
        with self.Image.open(path) as image:
            return image.convert("RGBA")

    def crop(self, tile, bounds):
        x,y,w,h = bounds
        img = self.sheet(str(self.catalog.graphics / "Tilesets" / f"{tile.tileset}_{tile.sheet}.png")).crop((x,y,x+w,y+h))
        if tile.tint != (255,255,255):
            img = self.Chops.multiply(img, self.Image.new("RGBA", img.size, (*tile.tint,255)))
        return img

    @lru_cache(maxsize=8192)
    def tile(self, tile, mask=0):
        if self.catalog.composite(tile):
            w,h = self.catalog.composite(tile)
            image = self.Image.new("RGBA", (w*32,h*32))
            self.composite(image, tile, (w-1)*32, (h-1)*32, "decoration")
            self.composite(image, tile, (w-1)*32, (h-1)*32, "foreground")
            return image
        if tile.sheet in ("A5","B","C"):
            return self.crop(tile, source_rect(tile))
        image = self.Image.new("RGBA", (32,32))
        x,y,kind = autotile_origin(tile)
        for q in range(4):
            X,Y = quarter(kind,q,mask)
            image.paste(self.crop(tile,(x+X*16,y+Y*16,16,16)), (q%2*16,q//2*16))
        return image

    def composite(self, image, tile, x, y, layer):
        if tile.index in TREES:
            sx,sy,w,_ = TREES[tile.index]
            row = 1 if layer == "decoration" else 0
            sprite = self.crop(tile,(sx,sy+row*32,w*32,32))
            image.alpha_composite(sprite,(x-(w-1)*32,y-(1-row)*32))
        elif tile.index in LAMPS:
            for row,index in enumerate(LAMPS[tile.index]):
                if (layer == "decoration") != (row == 2): continue
                sprite = self.crop(tile,source_rect(replace(tile,index=index)))
                image.alpha_composite(sprite,(x,y-(2-row)*32))

    def reference(self, m, tile, x, y):
        mask,scores,candidates = 0,Counter(),{}
        for i,(dx,dy) in enumerate(NEIGHBORS):
            other = m.get(tile.layer,x+dx,y+dy)
            if other is None: continue
            if compatible(tile,other): mask |= 1 << i
            if tile.layer == "ground" and (candidate := transition(tile,other)):
                scores[candidate.index] += 2 if i < 4 else 1
                candidates[candidate.index] = candidate
        if scores:
            best = max(sorted(scores), key=lambda i: scores[i] + int(i == tile.index))
            tile = candidates[best]
        return tile,mask

    def sprite(self, appearance, row=0):
        match = re.fullmatch(r"([A-Za-z0-9_!$]+)-([1-8])",appearance or "")
        if not match: return None
        sheet,index = match.group(1),int(match.group(2))-1
        if "$" in sheet and index: return None
        path = self.catalog.graphics / "Characters" / f"{sheet}.png"
        if not path.exists(): return None
        img = self.sheet(str(path)); cols,rows = (3,4) if "$" in sheet else (12,8)
        w,h = img.width//cols,img.height//rows
        x,y = ((index%4)*3+1)*w,((index//4)*4+row)*h
        result = img.crop((x,y,x+w,y+h))
        return result.resize((32,max(1,32*h//w)),self.Image.Resampling.NEAREST)

    def entity(self, item, metadata, world):
        category,id = item["category"],item["id"]
        if category == "portal_from": return self.sprite(item.get("appearance"))
        if category == "portal_to": return None
        if category == "shards": return self.tile(Tile("Dungeon","B",61,"decoration",(225,190,255)))
        if category == "start": return None
        entry = (metadata.objects(world) if category == "objects" else metadata.groups[category]).get(id,{})
        if category == "objects":
            fallback = {"signpost":"Outside/B/73", "cuttable_bush":"Outside/B/102", "smashable_rock":"Outside/B/103"}
            if entry.get("kind") in fallback: return self.tile(self.catalog.parse(fallback[entry["kind"]]))
        override = next((p["appearance"] for p in world.doc["entities"].get("object_appearances",[]) if p["id"]==id),None)
        return self.sprite(override or entry.get("appearance"),entry.get("frame_row",0))

    def map(self, world, m, bounds, path, metadata, tile_size=16, grid=False, collision=False, labels=False, regions=False):
        x,y,w,h = m.bounds(bounds)
        if w*h*1024 > 20_000_000 or w*h*tile_size*tile_size > 20_000_000:
            raise WorldError("Preview exceeds 20 million pixels; select a smaller --rect")
        path = external_output(world,path)
        view = m.view(); image = self.Image.new("RGBA",(w*32,h*32),(23,25,31,255))
        items = [p for p in positions(world,m.id) if x-2 <= p["x"] <= x+w+2 and y-2 <= p["y"] <= y+h+2]
        missing = []
        # Include anchors below/right of a crop: their canopy can extend into it.
        for layer in LAYERS:
            for Y in range(max(0,y),min(m.height,y+h+2)):
                for X in range(max(0,x),min(m.width,x+w+1)):
                    px,py = (X-x)*32,(Y-y)*32
                    if layer == "foreground" and (t := view.get("decoration",X,Y)) and self.catalog.composite(t):
                        self.composite(image,t,px,py,layer)
                    t = view.get(layer,X,Y)
                    if t is None: continue
                    if self.catalog.composite(t):
                        if layer == "decoration": self.composite(image,t,px,py,layer)
                    else:
                        tile,mask = self.reference(view,t,X,Y)
                        image.alpha_composite(self.tile(tile,mask),(px,py))
            if layer == "decoration":
                for p in sorted(items,key=lambda p:(p["y"],p["x"])):
                    sprite = self.entity(p,metadata,world)
                    if sprite:
                        image.alpha_composite(sprite,((p["x"]-x)*32,(p["y"]-y+1)*32-sprite.height))
                    elif p["category"] in ("npcs","objects"):
                        missing.append(p["id"])
                        self.Draw.Draw(image).ellipse(((p["x"]-x)*32+8,(p["y"]-y)*32+8,(p["x"]-x)*32+24,(p["y"]-y)*32+24),fill="#e3aa39")
        image = image.resize((w*tile_size,h*tile_size),self.Image.Resampling.NEAREST)
        if collision:
            overlay = self.Image.new("RGBA",image.size)
            draw = self.Draw.Draw(overlay); nav = Navigation(world,m,bounds)
            for Y in range(y,y+h):
                for X in range(x,x+w):
                    if not nav.walkable[Y*m.width+X]:
                        draw.rectangle(((X-x)*tile_size,(Y-y)*tile_size,(X-x+1)*tile_size-1,(Y-y+1)*tile_size-1),fill=(245,50,65,80))
            image.alpha_composite(overlay)
        draw = self.Draw.Draw(image)
        if grid:
            for X in range(w+1): draw.line((X*tile_size,0,X*tile_size,h*tile_size),fill=(255,255,255,80))
            for Y in range(h+1): draw.line((0,Y*tile_size,w*tile_size,Y*tile_size),fill=(255,255,255,80))
        if regions:
            for r in world.doc["regions"]:
                if r["map"] != m.id: continue
                X,Y = (r["x"]-x)*tile_size,(r["y"]-y)*tile_size
                draw.rectangle((X,Y,X+r["width"]*tile_size-1,Y+r["height"]*tile_size-1),outline="#ffd95b",width=2)
                draw.text((X+2,Y+2),r["id"],fill="white",stroke_width=1,stroke_fill="black")
        if labels:
            for p in items:
                if not inside(bounds,p["x"],p["y"]): continue
                X,Y=(p["x"]-x)*tile_size,(p["y"]-y)*tile_size
                draw.rectangle((X,Y,X+tile_size-1,Y+tile_size-1),outline="#50efff")
                draw.text((X+2,Y+2),p["id"],fill="white",stroke_width=1,stroke_fill="black")
        path.parent.mkdir(parents=True,exist_ok=True);image.convert("RGB").save(path,format="PNG")
        return dict(output=str(path),map=m.id,rect=list(bounds),pixels=list(image.size),tile_size=tile_size,
                    missing_sprites=missing,note="Static asset preview; no weather, lighting, animation, save state or NPC movement. Unknown legacy sprites use gold markers.")

    def contact_sheet(self, rows, path):
        if not rows: raise WorldError("No catalog results to render")
        columns, cw, ch = 5,220,165
        image = self.Image.new("RGB",(columns*cw,((len(rows)+columns-1)//columns)*ch),(28,31,38))
        draw = self.Draw.Draw(image)
        for i,row in enumerate(rows):
            x,y = (i%columns)*cw,(i//columns)*ch
            tile = self.catalog.parse(row["canonical_key"])
            sprite = self.tile(tile).copy()
            sprite.thumbnail((96,96),self.Image.Resampling.NEAREST)
            image.paste(sprite,(x+8,y+8),sprite)
            # A filled autotile sample beside the isolated one distinguishes terrain materials.
            if tile.sheet in ("A1","A2","A3","A4"):
                inner = self.tile(tile,255).resize((64,64),self.Image.Resampling.NEAREST)
                image.paste(inner,(x+112,y+8),inner)
            draw.text((x+8,y+108),row["key"],fill="white")
            draw.text((x+8,y+124),row["name"][:32],fill="#ddd49e")
            draw.text((x+8,y+140),row["layer"]+" / "+row["collision"],fill="#8ed7e6")
        path=Path(path);path.parent.mkdir(parents=True,exist_ok=True);image.save(path,format="PNG")
        return str(path.resolve())
