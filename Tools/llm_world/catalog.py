"""RTP catalog semantics, matching Source/App/RtpTilesetRenderer.cpp.

The integration tests compare this port against the native implementation. Keep
collision and composite placement here so inspection, painting and rendering use
the same rules. Asset names are loaded from the game's actual metadata files.
"""
from dataclasses import dataclass, replace
from functools import lru_cache
from pathlib import Path
import re

LAYERS = ("ground", "decoration", "foreground")
COUNTS = {"A1": 16, "A2": 32, "A3": 32, "A4": 48, "A5": 128, "B": 256, "C": 256}
FAMILIES = ("Dungeon", "Inside", "Outside", "World")
# canonical index: (source x, source y, width in tiles, aliases)
TREES = {
    93: (160, 352, 1, (93, 94, 101)),
    112: (0, 448, 2, (112, 113, 114, 115, 120, 121, 122, 123)),
    128: (256, 0, 2, (128, 129, 130, 131, 136, 137, 138, 139)),
    157: (416, 96, 1, (157, 158, 165)),
    168: (256, 160, 1, (168, 169, 176, 177)),
    172: (384, 160, 1, (172, 173, 180)),
}
LAMPS = {227: (211, 219, 227), 251: (235, 243, 251)}
ALIASES = {a: key for key, v in TREES.items() for a in v[3]}
ALIASES.update({a: key for key, v in LAMPS.items() for a in v})
# index: (surface, background, automatically selected)
SURFACES = {
    0: ("meadow", None, False), 1: ("dirt", "meadow", True),
    2: ("road", "meadow", True), 3: ("cobbles", None, False),
    8: ("dirt", None, False), 9: ("meadow", "dirt", False),
    10: ("road", "dirt", True), 11: ("cobbles", "snow", True),
    16: ("sand", None, False), 17: ("meadow", "sand", True),
    18: ("road", "sand", True), 19: ("cobbles", None, False),
    24: ("snow", None, False), 25: ("dirt", "snow", True),
    26: ("road", "snow", True), 27: ("carpet", None, False),
}


class WorldError(ValueError):
    """An actionable authoring error, reported as JSON by the CLI."""


def integer(value, label, low=None, high=None):
    if type(value) is not int or (low is not None and value < low) or (high is not None and value > high):
        raise WorldError(f"{label} must be an integer" + (f" in {low}..{high}" if high is not None else ""))
    return value


@dataclass(frozen=True)
class Tile:
    tileset: str
    sheet: str
    index: int
    layer: str
    tint: tuple = (255, 255, 255)

    @property
    def key(self):
        return f"{self.tileset}/{self.sheet}/{self.index}"

    def json(self):
        return dict(tileset=self.tileset, sheet=self.sheet, index=self.index,
                    layer=self.layer, tint=list(self.tint))


class Catalog:
    def __init__(self, graphics):
        self.graphics = Path(graphics).resolve()
        self.names = {}
        for family in FAMILIES:
            for sheet, count in COUNTS.items():
                path = self.graphics / "Tilesets" / f"{family}_{sheet}.txt"
                if not path.exists():
                    continue
                names = [line.split("|")[0] for line in path.read_text(encoding="utf-8-sig").splitlines()]
                if len(names) != count:
                    raise WorldError(f"{path}: expected {count} names, found {len(names)}")
                self.names[family, sheet] = names
        if not self.names:
            raise WorldError(f"No tileset metadata found beneath {self.graphics}")

    def parse(self, value, *, layer=None, tint=None, canonical=True):
        if isinstance(value, str):
            match = re.fullmatch(r"(Dungeon|Inside|Outside|World)/(A[1-5]|B|C)/(\d+)", value)
            if not match:
                raise WorldError(f"Invalid tile {value!r}; use e.g. Outside/A2/0")
            f, s, i = match.groups()
            value = dict(tileset=f, sheet=s, index=int(i))
        if not isinstance(value, dict):
            raise WorldError("A tile must be a catalog key or tile-reference object")
        f, s, i = value.get("tileset"), value.get("sheet"), value.get("index")
        if (f, s) not in self.names:
            raise WorldError(f"Unavailable tileset sheet: {f}/{s}")
        integer(i, "tile index", 0, len(self.names[f, s]) - 1)
        color = tint if tint is not None else value.get("tint", [255, 255, 255])
        if not isinstance(color, (list, tuple)) or len(color) != 3:
            raise WorldError("Tile tint needs three RGB integers")
        for c in color:
            integer(c, "tint channel", 0, 255)
        inferred = self.inferred_layer(f, s, i)
        l = layer if layer is not None else value.get("layer", inferred)
        if l not in LAYERS:
            raise WorldError(f"Unknown render layer: {l}")
        tile = Tile(f, s, i, l, tuple(color))
        return self.canonical(tile) if canonical else tile

    def name(self, tile):
        return self.names[tile.tileset, tile.sheet][tile.index]

    def inferred_layer(self, family, sheet, index):
        if family == "Outside" and sheet == "B":
            for members in LAMPS.values():
                if index in members:
                    return "decoration" if members.index(index) == 2 else "foreground"
        if sheet == "A1":
            return "decoration" if 1 <= index <= 3 else "ground"
        if sheet == "A2":
            return "decoration" if index % 8 >= 4 else "ground"
        if sheet not in ("B", "C"):
            return "ground"
        name = self.names[family, sheet][index]
        return "foreground" if "(Front" in name or "Foreground" in name else "decoration"

    @staticmethod
    def canonical(tile):
        if tile.tileset == "Outside" and tile.sheet == "B" and tile.index in ALIASES:
            return replace(tile, index=ALIASES[tile.index], layer="decoration")
        return tile

    @staticmethod
    def composite(tile):
        if tile.tileset == "Outside" and tile.sheet == "B":
            i = ALIASES.get(tile.index, tile.index)
            if i in TREES:
                return TREES[i][2], 2
            if i in LAMPS:
                return 1, 3
        return None

    @lru_cache(maxsize=16384)
    def collision(self, tile):
        if tile.tileset == "World":
            return "inherits"
        if tile.sheet in ("A1", "A3", "A4"):
            return "blocked"
        name = self.name(tile)
        if name in ("", "Transparent"):
            return "inherits"
        if tile.sheet == "A2":
            return "blocked" if tile.index % 8 >= 4 else "walkable"
        if tile.sheet == "A5":
            return "blocked" if name == "Darkness" or name.startswith(("Broken Bridge", "Cliff", "Ledge")) else "walkable"
        passage = (name in ("Entrance", "Exit", "Gate", "Railroad Ties", "Grass", "Flowers")
                   or "(Gate)" in name or name.startswith(("Cave Entrance", "Mine Entrance", "Rails", "Stairs", "Rug", "Carpet", "Straw Mat"))
                   or "Ladder" in name
                   or ("Bridge" in name and "Bridge Spar" not in name and "Broken Bridge" not in name))
        return "walkable" if passage else "blocked"

    def describe(self, tile):
        canonical = self.canonical(tile)
        footprint = self.composite(canonical) or (1, 1)
        aliases = (TREES[canonical.index][3] if canonical.index in TREES else LAMPS.get(canonical.index, ())) if self.composite(canonical) else ()
        return {"key": tile.key, "name": self.name(tile), "canonical_key": canonical.key,
                "layer": canonical.layer, "collision": self.collision(canonical),
                "visual_bounds": [-(footprint[0] - 1), -(footprint[1] - 1), *footprint],
                "stored_anchor": [0, 0], "source_rect": list(source_rect(tile)),
                "aliases": list(aliases), "tint": list(tile.tint),
                "placement_note": "Large tree anchors on the same row must be at least two columns apart." if footprint[0] == 2 else ""}

    def search(self, query="", family=None, sheet=None, aliases=False):
        results = []
        for (f, s), names in self.names.items():
            if (family and f != family) or (sheet and s != sheet):
                continue
            for i, name in enumerate(names):
                tile = self.parse(f"{f}/{s}/{i}", canonical=False)
                if not aliases and self.canonical(tile).index != i:
                    continue
                if all(word in (name + " " + tile.key).lower() for word in query.lower().split()):
                    results.append(self.describe(tile))
        return results


def source_rect(tile):
    """Source-sheet block, including all frames for A1; not a rendered tile."""
    i, s = tile.index, tile.sheet
    if s in ("B", "C"):
        return ((i // 128 * 8 + i % 8) * 32, i % 128 // 8 * 32, 32, 32)
    if s == "A5":
        return (i % 8 * 32, i // 8 * 32, 32, 32)
    if s == "A2":
        return (i % 8 * 64, i // 8 * 96, 64, 96)
    if s == "A3":
        return (i % 8 * 64, i // 8 * 64, 64, 64)
    if s == "A4":
        row = i // 8
        return (i % 8 * 64, row // 2 * 160 + (96 if row % 2 else 0), 64, 64 if row % 2 else 96)
    if i < 4:
        return (0 if i < 2 else 192, i % 2 * 96, 192 if i < 2 else 64, 96)
    return (i % 8 // 4 * 256 + (192 if i % 2 else 0), i // 8 * 192 + (i % 8 // 2 % 2) * 96, 64 if i % 2 else 192, 96)


def surface(tile):
    return SURFACES.get(tile.index) if (tile.tileset, tile.sheet, tile.layer) == ("Outside", "A2", "ground") else None


def compatible(a, b):
    if (a.tileset, a.sheet, a.layer, a.tint) != (b.tileset, b.sheet, b.layer, b.tint):
        return False
    return a.index == b.index or bool(surface(a) and surface(b) and surface(a)[0] == surface(b)[0])


def transition(a, b):
    sa, sb = surface(a), surface(b)
    if not sa or not sb or a.tint != b.tint or sa[0] == sb[0] or sb[1] == sa[0]:
        return None
    for i, candidate in SURFACES.items():
        selected = sa[1] == sb[0] and i == a.index
        if candidate[:2] == (sa[0], sb[0]) and (candidate[2] or selected):
            return replace(a, index=i)
    return None


def quarter(kind, q, mask):
    n, e, s, w, nw, ne, se, sw = [bool(mask & (1 << i)) for i in range(8)]
    left, top = q % 2 == 0, q < 2
    horizontal, vertical = (w if left else e), (n if top else s)
    if kind in ("wall", "waterfall"):
        x = (2 if horizontal else 0) if left else (1 if horizontal else 3)
        y = (0 if top else 1) if kind == "waterfall" else ((2 if vertical else 0) if top else (1 if vertical else 3))
        return x, y
    if q == 0:
        return ((2, 4) if nw else (2, 0)) if n and w else (0, 4) if n else (2, 2) if w else (0, 2)
    if q == 1:
        return ((1, 4) if ne else (3, 0)) if n and e else (3, 4) if n else (1, 2) if e else (3, 2)
    if q == 2:
        return ((2, 3) if sw else (2, 1)) if s and w else (0, 3) if s else (2, 5) if w else (0, 5)
    return ((1, 3) if se else (3, 1)) if s and e else (3, 3) if s else (1, 5) if e else (3, 5)


def autotile_origin(tile, frame=0):
    i, s = tile.index, tile.sheet
    x, y, _, _ = source_rect(tile)
    if s != "A1":
        return x, y, "wall" if s == "A3" or (s == "A4" and i // 8 % 2) else "floor"
    frame = [0, 1, 2, 1][frame % 4]
    if i < 4:
        return (frame * 64 if i < 2 else 192), (i % 2) * 96, "floor"
    return (x if i % 2 else x + frame * 64), y, "waterfall" if i % 2 else "floor"
