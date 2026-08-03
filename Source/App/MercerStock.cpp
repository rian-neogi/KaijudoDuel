#include "MercerStock.h"

#include "Game/Card.h"
#include "LuaInclude.h"

#include <set>

namespace
{
	std::string luaStringField(lua_State* state, int table, const char* key)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		std::string value = lua_isstring(state, -1) ? lua_tostring(state, -1) : "";
		lua_pop(state, 1);
		return value;
	}

	bool readCardList(lua_State* state, int table, const char* key,
		std::vector<std::string>& cards, std::string& error)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		if (!lua_istable(state, -1))
		{
			error = std::string("missing card list '") + key + "'";
			lua_pop(state, 1);
			return false;
		}
		const size_t count = lua_rawlen(state, -1);
		for (size_t i = 1; i <= count; ++i)
		{
			lua_rawgeti(state, -1, (lua_Integer)i);
			if (!lua_isstring(state, -1))
			{
				error = std::string("non-string card in '") + key + "'";
				lua_pop(state, 2);
				return false;
			}
			std::string name = lua_tostring(state, -1);
			lua_pop(state, 1);
			if (getCardIdFromName(name) < 0)
			{
				error = "unknown card '" + name + "'";
				lua_pop(state, 1);
				return false;
			}
			cards.push_back(name);
		}
		lua_pop(state, 1);
		return true;
	}
}

MercerStockData::MercerStockData()
{
	for (int i = 0; i < 5; ++i) prices[i] = 0;
}

bool loadMercerStockFromLua(const std::string& path, MercerStockData& stock,
	std::string& error)
{
	stock = MercerStockData();
	error.clear();
	lua_State* state = luaL_newstate();
	if (state == NULL)
	{
		error = "could not create Lua state";
		return false;
	}
	luaL_openlibs(state);
	if (luaL_loadfile(state, path.c_str()) != LUA_OK || lua_pcall(state, 0, 1, 0) != LUA_OK)
	{
		const char* message = lua_tostring(state, -1);
		error = message == NULL ? "unknown Lua error" : message;
		lua_close(state);
		return false;
	}
	if (!lua_istable(state, -1))
	{
		error = "stock file must return a table";
		lua_close(state);
		return false;
	}
	const int root = lua_gettop(state);
	lua_getfield(state, root, "prices");
	if (!lua_istable(state, -1) || lua_rawlen(state, -1) != 5)
	{
		error = "prices must contain five tiers";
		lua_close(state);
		return false;
	}
	for (int tier = 1; tier <= 5; ++tier)
	{
		lua_rawgeti(state, -1, tier);
		stock.prices[tier - 1] = lua_isnumber(state, -1) ? (int)lua_tointeger(state, -1) : 0;
		lua_pop(state, 1);
		if (stock.prices[tier - 1] <= 0)
		{
			error = "each price tier must be positive";
			lua_close(state);
			return false;
		}
	}
	lua_pop(state, 1);
	if (!readCardList(state, root, "initial_stock", stock.initialStock, error))
	{
		lua_close(state);
		return false;
	}

	lua_getfield(state, root, "shards");
	if (!lua_istable(state, -1))
	{
		error = "missing shard list";
		lua_close(state);
		return false;
	}
	std::set<std::string> ids;
	const size_t shardCount = lua_rawlen(state, -1);
	for (size_t i = 1; i <= shardCount; ++i)
	{
		lua_rawgeti(state, -1, (lua_Integer)i);
		const int shardTable = lua_gettop(state);
		if (!lua_istable(state, shardTable))
		{
			error = "shard entry is not a table";
			lua_close(state);
			return false;
		}
		MercerShard shard;
		shard.id = luaStringField(state, shardTable, "id");
		shard.name = luaStringField(state, shardTable, "name");
		shard.x = 0;
		shard.y = 0;
		if (shard.id.empty() || shard.name.empty() || !ids.insert(shard.id).second)
		{
			error = "invalid or duplicate shard entry " + std::to_string(i);
			lua_close(state);
			return false;
		}
		if (!readCardList(state, shardTable, "stock", shard.stock, error))
		{
			error = "shard '" + shard.id + "': " + error;
			lua_close(state);
			return false;
		}
		stock.shards.push_back(shard);
		lua_pop(state, 1);
	}
	lua_close(state);
	return true;
}
