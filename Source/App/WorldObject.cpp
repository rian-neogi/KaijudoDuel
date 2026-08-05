#include "WorldObject.h"

#include "Game/Deck.h"
#include "LuaInclude.h"

#include <set>

namespace
{
	std::string stringField(lua_State* state, int table, const char* key)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		std::string value = lua_type(state, -1) == LUA_TSTRING ?
			lua_tostring(state, -1) : "";
		lua_pop(state, 1);
		return value;
	}
}

bool loadWorldObjectsFromLua(const std::string& path,
	std::vector<WorldObject>& objects, std::string& error)
{
	objects.clear();
	error.clear();
	lua_State* state = luaL_newstate();
	if (state == NULL)
	{
		error = "could not create Lua state";
		return false;
	}
	luaL_openlibs(state);
	if (luaL_loadfile(state, path.c_str()) != LUA_OK ||
		lua_pcall(state, 0, 1, 0) != LUA_OK)
	{
		const char* message = lua_tostring(state, -1);
		error = message == NULL ? "unknown Lua error" : message;
		lua_close(state);
		return false;
	}
	if (!lua_istable(state, -1))
	{
		error = "object metadata must return an array";
		lua_close(state);
		return false;
	}

	std::set<std::string> ids;
	const size_t count = lua_rawlen(state, -1);
	for (size_t index = 1; index <= count; ++index)
	{
		lua_rawgeti(state, -1, (lua_Integer)index);
		if (!lua_istable(state, -1))
		{
			error = "object entry " + std::to_string(index) + " is not a table";
			lua_close(state);
			return false;
		}
		WorldObject object;
		object.id = stringField(state, -1, "id");
		object.name = stringField(state, -1, "name");
		object.text = stringField(state, -1, "text");
		object.openedText = stringField(state, -1, "opened_text");
		object.rewardDeckName = stringField(state, -1, "deck_name");
		object.x = object.y = 0;
		const std::string kind = stringField(state, -1, "kind");
		if (kind == "signpost") object.kind = WorldObjectKind::Signpost;
		else if (kind == "deck_chest")
		{
			object.kind = WorldObjectKind::DeckChest;
			const std::string configuredDeck = stringField(state, -1, "deck");
			if (!resolveDeckPath(configuredDeck, object.rewardDeck))
			{
				error = "deck chest '" + object.id +
					"' cannot find its deck directly or beneath Decks/: " + configuredDeck;
				lua_close(state);
				return false;
			}
		}
		else
		{
			error = "object '" + object.id + "' has unsupported kind '" + kind + "'";
			lua_close(state);
			return false;
		}
		bool validKindFields = object.kind == WorldObjectKind::Signpost ||
			(!object.openedText.empty() && !object.rewardDeck.empty() &&
			 !object.rewardDeckName.empty());
		if (object.id.empty() || object.name.empty() || object.text.empty() ||
			!validKindFields || !ids.insert(object.id).second)
		{
			error = "object entry " + std::to_string(index) +
				" needs a unique id, name, and all fields required by its kind";
			lua_close(state);
			return false;
		}
		objects.push_back(object);
		lua_pop(state, 1);
	}
	lua_close(state);
	if (objects.empty())
	{
		error = "object metadata contains no objects";
		return false;
	}
	return true;
}
