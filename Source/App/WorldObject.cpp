#include "WorldObject.h"

#include "Game/Deck.h"
#include "LuaInclude.h"

#include <cctype>
#include <fstream>
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

	bool parseAppearance(const std::string& value,
		std::string& spriteSheet, int& spriteIndex)
	{
		const size_t separator = value.find_last_of('-');
		if (separator == std::string::npos || separator == 0 ||
			separator + 1 >= value.size()) return false;
		const std::string sheetName = value.substr(0, separator);
		for (size_t i = 0; i < sheetName.size(); ++i)
		{
			const unsigned char character = (unsigned char)sheetName[i];
			if (!std::isalnum(character) && character != '_' &&
				character != '!' && character != '$') return false;
		}
		int luaIndex = 0;
		for (size_t i = separator + 1; i < value.size(); ++i)
		{
			const unsigned char character = (unsigned char)value[i];
			if (!std::isdigit(character)) return false;
			luaIndex = luaIndex * 10 + (character - '0');
			if (luaIndex > 8) return false;
		}
		const bool singleCharacter = sheetName.find('$') != std::string::npos;
		if (luaIndex < 1 || luaIndex > (singleCharacter ? 1 : 8)) return false;

		spriteSheet = "Resources/Graphics/Characters/" + sheetName + ".png";
		std::ifstream file(spriteSheet.c_str(), std::ios::binary);
		if (!file.good()) return false;
		spriteIndex = luaIndex - 1;
		return true;
	}

	bool booleanField(lua_State* state, int table, const char* key)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		bool value = lua_toboolean(state, -1) != 0;
		lua_pop(state, 1);
		return value;
	}

	int integerField(lua_State* state, int table, const char* key, int fallback)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		int value = lua_isinteger(state, -1) ? (int)lua_tointeger(state, -1) : fallback;
		lua_pop(state, 1);
		return value;
	}

	bool parseKind(const std::string& value, WorldObjectKind& kind)
	{
		if (value == "signpost") kind = WorldObjectKind::Signpost;
		else if (value == "chest") kind = WorldObjectKind::Chest;
		else if (value == "cuttable_bush") kind = WorldObjectKind::CuttableBush;
		else if (value == "smashable_rock") kind = WorldObjectKind::SmashableRock;
		else if (value == "environment") kind = WorldObjectKind::Environment;
		else return false;
		return true;
	}

	bool readTemplates(lua_State* state, std::vector<WorldObjectTemplate>& templates,
		std::string& error)
	{
		lua_getglobal(state, "WorldObjectTemplates");
		if (!lua_istable(state, -1))
		{
			error = "WorldObjectTemplates must be an array";
			lua_pop(state, 1);
			return false;
		}
		std::set<std::string> ids;
		const size_t count = lua_rawlen(state, -1);
		for (size_t index = 1; index <= count; ++index)
		{
			lua_rawgeti(state, -1, (lua_Integer)index);
			if (!lua_istable(state, -1))
			{
				error = "object template " + std::to_string(index) + " is not a table";
				lua_pop(state, 2);
				return false;
			}
			WorldObjectTemplate objectTemplate;
			objectTemplate.id = stringField(state, -1, "id");
			WorldObject& object = objectTemplate.object;
			object.templateId = objectTemplate.id;
			object.name = stringField(state, -1, "name");
			object.text = stringField(state, -1, "text");
			object.openedText = stringField(state, -1, "opened_text");
			object.animated = booleanField(state, -1, "animated");
			object.spriteRow = integerField(state, -1, "frame_row", 0);
			const std::string kindName = stringField(state, -1, "kind");
			const std::string appearance = stringField(state, -1, "appearance");
			bool validAppearance = appearance.empty() ||
				parseAppearance(appearance, object.spriteSheet, object.spriteIndex);
			if (objectTemplate.id.empty() || object.name.empty() ||
				object.text.empty() || !parseKind(kindName, object.kind) ||
				!validAppearance || object.spriteRow < 0 || object.spriteRow > 3 ||
				!ids.insert(objectTemplate.id).second)
			{
				error = "object template " + std::to_string(index) +
					" has invalid or duplicate metadata";
				lua_pop(state, 2);
				return false;
			}
			if ((object.kind == WorldObjectKind::Chest ||
				object.kind == WorldObjectKind::Environment) && object.spriteSheet.empty())
			{
				error = "object template '" + objectTemplate.id + "' needs an appearance";
				lua_pop(state, 2);
				return false;
			}
			if (object.kind == WorldObjectKind::Chest && object.openedText.empty())
			{
				error = "chest template '" + objectTemplate.id +
					"' needs opened_text";
				lua_pop(state, 2);
				return false;
			}
			templates.push_back(objectTemplate);
			lua_pop(state, 1);
		}
		lua_pop(state, 1);
		if (templates.empty())
		{
			error = "WorldObjectTemplates must contain at least one template";
			return false;
		}
		return true;
	}
}

bool loadWorldObjectsFromLua(const std::string& path,
	std::vector<WorldObject>& objects, std::vector<WorldObjectTemplate>& templates,
	std::string& error)
{
	objects.clear();
	templates.clear();
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
		const std::string kind = stringField(state, -1, "kind");
		if (kind == "signpost") object.kind = WorldObjectKind::Signpost;
		else if (kind == "deck_chest")
		{
			object.kind = WorldObjectKind::DeckChest;
			const std::string appearance = stringField(state, -1, "appearance");
			if (!parseAppearance(appearance, object.spriteSheet, object.spriteIndex))
			{
				error = "deck chest '" + object.id + "' has invalid appearance '" +
					appearance + "'";
				lua_close(state);
				return false;
			}

			lua_getfield(state, -1, "reward");
			if (!lua_istable(state, -1))
			{
				error = "deck chest '" + object.id + "' needs a reward table";
				lua_close(state);
				return false;
			}
			const std::string rewardKind = stringField(state, -1, "kind");
			const std::string configuredDeck = stringField(state, -1, "deck");
			object.rewardDeckName = stringField(state, -1, "name");
			lua_pop(state, 1);
			if (rewardKind != "deck")
			{
				error = "deck chest '" + object.id +
					"' has unsupported reward kind '" + rewardKind + "'";
				lua_close(state);
				return false;
			}
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
	if (!readTemplates(state, templates, error))
	{
		lua_close(state);
		return false;
	}
	lua_close(state);
	if (objects.empty())
	{
		error = "object metadata contains no objects";
		return false;
	}
	return true;
}

WorldObject createWorldObject(const WorldObjectTemplate& objectTemplate,
	const std::string& id)
{
	WorldObject object = objectTemplate.object;
	object.id = id;
	object.templateId = objectTemplate.id;
	object.editorCreated = true;
	return object;
}

const char* worldObjectKindName(WorldObjectKind kind)
{
	if (kind == WorldObjectKind::DeckChest) return "Deck Chest";
	if (kind == WorldObjectKind::Chest) return "Chest";
	if (kind == WorldObjectKind::CuttableBush) return "Cuttable Bush";
	if (kind == WorldObjectKind::SmashableRock) return "Smashable Rock";
	if (kind == WorldObjectKind::Environment) return "Environment";
	return "Signpost";
}
