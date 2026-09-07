"""Read the authoritative Lua tables through the game's Lua 5.4 dependency."""
import ctypes as C
import ctypes.util
from pathlib import Path

from .catalog import WorldError


class Metadata:
    def __init__(self, directory):
        self.directory = Path(directory).resolve()
        self._groups = None

    @property
    def groups(self):
        if self._groups is None:
            self._groups = self.load()
        return self._groups

    def load(self):
        library = ctypes.util.find_library("lua5.4")
        if not library:
            raise WorldError("Lua 5.4 is required to query/validate real entity metadata (the same library the game uses)")
        lua = C.CDLL(library)
        signatures = {
            "luaL_newstate": (C.c_void_p, []), "luaL_openlibs": (None, [C.c_void_p]),
            "lua_close": (None, [C.c_void_p]), "luaL_loadfilex": (C.c_int, [C.c_void_p, C.c_char_p, C.c_char_p]),
            "lua_pcallk": (C.c_int, [C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_ssize_t, C.c_void_p]),
            "lua_type": (C.c_int, [C.c_void_p, C.c_int]), "lua_absindex": (C.c_int, [C.c_void_p, C.c_int]),
            "lua_tolstring": (C.c_void_p, [C.c_void_p, C.c_int, C.POINTER(C.c_size_t)]),
            "lua_toboolean": (C.c_int, [C.c_void_p, C.c_int]),
            "lua_tonumberx": (C.c_double, [C.c_void_p, C.c_int, C.c_void_p]),
            "lua_pushnil": (None, [C.c_void_p]), "lua_next": (C.c_int, [C.c_void_p, C.c_int]),
            "lua_settop": (None, [C.c_void_p, C.c_int]), "lua_getglobal": (C.c_int, [C.c_void_p, C.c_char_p]),
        }
        for name, (result, args) in signatures.items():
            fn = getattr(lua, name); fn.restype, fn.argtypes = result, args

        def string(state, index):
            length = C.c_size_t()
            pointer = lua.lua_tolstring(state, index, C.byref(length))
            return C.string_at(pointer, length.value).decode("utf-8") if pointer else ""

        def value(state, index, depth=0):
            if depth > 20:
                raise WorldError("Lua metadata is too deeply nested")
            kind = lua.lua_type(state, index)
            if kind == 0:
                return None
            if kind == 1:
                return bool(lua.lua_toboolean(state, index))
            if kind == 3:
                n = lua.lua_tonumberx(state, index, None)
                return int(n) if n.is_integer() else n
            if kind == 4:
                return string(state, index)
            if kind != 5:
                return None
            result = {}
            index = lua.lua_absindex(state, index)
            lua.lua_pushnil(state)
            while lua.lua_next(state, index):
                key, item = value(state, -2, depth + 1), value(state, -1, depth + 1)
                if not isinstance(key, (str, int)):
                    raise WorldError("Lua metadata has a non-string/non-integer key")
                result[key] = item
                lua.lua_settop(state, -2)
            if result and set(result) == set(range(1, len(result) + 1)):
                return [result[i] for i in range(1, len(result) + 1)]
            return result

        def file(name, globals=()):
            state = lua.luaL_newstate()
            if not state:
                raise WorldError("Could not allocate a Lua state")
            try:
                lua.luaL_openlibs(state)
                path = self.directory / name
                if lua.luaL_loadfilex(state, str(path).encode(), None) or lua.lua_pcallk(state, 0, 1, 0, 0, None):
                    raise WorldError(f"{name}: {string(state, -1)}")
                result = value(state, -1)
                extras = {}
                for key in globals:
                    lua.lua_getglobal(state, key.encode()); extras[key] = value(state, -1)
                    lua.lua_settop(state, -2)
                return result, extras
            finally:
                lua.lua_close(state)

        npcs, _ = file("Npcs.lua")
        objects, extra = file("Objects.lua", ("WorldObjectTemplates",))
        stock, _ = file("MercerStock.lua")
        groups = dict(npcs=npcs, objects=objects, templates=extra["WorldObjectTemplates"], shards=stock["shards"])
        result = {}
        for group, rows in groups.items():
            if rows == {}:
                rows = []
            if not isinstance(rows, list):
                raise WorldError(f"Lua {group} must be an array")
            result[group] = {}
            for row in rows:
                if not isinstance(row, dict) or not isinstance(row.get("id"), str) or not row["id"] or row["id"] in result[group]:
                    raise WorldError(f"Invalid or duplicate Lua {group} ID")
                result[group][row["id"]] = row
        return result

    def objects(self, world):
        result = dict(self.groups["objects"])
        for definition in world.doc["entities"].get("object_definitions", []):
            template = self.groups["templates"].get(definition["template"])
            if template and definition["id"] not in result:
                result[definition["id"]] = dict(template, id=definition["id"], template=definition["template"])
        return result

    def query(self, world, group, query=""):
        groups = self.groups
        rows = self.objects(world) if group == "objects" else groups[group]
        positions = {p["id"]: p for p in world.doc["entities"].get(group, [])}
        fields = ("id", "name", "kind", "appearance", "template", "frame_row", "animated", "options", "sight", "shop_stock", "crest")
        return [dict({k: row[k] for k in fields if k in row}, position=positions.get(id))
                for id, row in rows.items() if query.lower() in (id + " " + row.get("name", "")).lower()]
