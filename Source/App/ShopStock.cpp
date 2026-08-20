#include "ShopStock.h"

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
		const std::string& shopId, std::vector<std::string>& cards,
		bool allowEmpty, std::string& error)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		if (!lua_istable(state, -1))
		{
			error = "shop '" + shopId + "' is missing " + key;
			lua_pop(state, 1);
			return false;
		}
		const size_t count = lua_rawlen(state, -1);
		if (!allowEmpty && count == 0)
		{
			error = "shop '" + shopId + "' has empty " + key;
			lua_pop(state, 1);
			return false;
		}
		for (size_t index = 1; index <= count; ++index)
		{
			lua_rawgeti(state, -1, (lua_Integer)index);
			if (!lua_isstring(state, -1))
			{
				error = "shop '" + shopId + "' has a non-string card in " + key;
				lua_pop(state, 2);
				return false;
			}
			const std::string cardName = lua_tostring(state, -1);
			lua_pop(state, 1);
			if (getCardIdFromName(cardName) < 0)
			{
				error = "shop '" + shopId + "' has unknown card '" + cardName + "'";
				lua_pop(state, 1);
				return false;
			}
			cards.push_back(cardName);
		}
		lua_pop(state, 1);
		return true;
	}
}

const ShopStock* ShopStockData::find(const std::string& id) const
{
	for (size_t index = 0; index < shops.size(); ++index)
		if (shops[index].id == id) return &shops[index];
	return NULL;
}

bool loadShopStockFromLua(const std::string& path, ShopStockData& stock,
	std::string& error)
{
	stock = ShopStockData();
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
	lua_getfield(state, root, "shops");
	if (!lua_istable(state, -1) || lua_rawlen(state, -1) == 0)
	{
		error = "stock file needs a non-empty shops array";
		lua_close(state);
		return false;
	}
	std::set<std::string> ids;
	const size_t shopCount = lua_rawlen(state, -1);
	for (size_t index = 1; index <= shopCount; ++index)
	{
		lua_rawgeti(state, -1, (lua_Integer)index);
		const int entry = lua_gettop(state);
		if (!lua_istable(state, entry))
		{
			error = "shop entry " + std::to_string(index) + " is not a table";
			lua_close(state);
			return false;
		}
		ShopStock shop;
		shop.id = luaStringField(state, entry, "id");
		shop.name = luaStringField(state, entry, "name");
		if (shop.id.empty() || shop.name.empty() || !ids.insert(shop.id).second)
		{
			error = "invalid or duplicate shop entry " + std::to_string(index);
			lua_close(state);
			return false;
		}
		if (!readCardList(state, entry, "initial_stock", shop.id,
			shop.initialStock, false, error) ||
			!readCardList(state, entry, "act_iii_bonus", shop.id,
				shop.actThreeBonus, true, error))
		{
			lua_close(state);
			return false;
		}
		stock.shops.push_back(shop);
		lua_pop(state, 1);
	}
	lua_close(state);
	return true;
}
