#include "AiParams.h"

#include <cmath>
#include <cctype>
#include <cstdio>
#include <limits>
#include <unordered_map>

namespace
{
	std::unordered_map<std::string, double>& values()
	{
		static std::unordered_map<std::string, double> result;
		return result;
	}

	std::string normalized(const std::string& value)
	{
		std::string result = value;
		for (size_t i = 0; i < result.size(); ++i)
			result[i] = (char)std::tolower((unsigned char)result[i]);
		return result;
	}

	bool hasProfile(const std::string& group, const std::string& name)
	{
		const std::string prefix = group + "." + normalized(name) + ".";
		for (std::unordered_map<std::string, double>::const_iterator value =
			values().begin(); value != values().end(); ++value)
		{
			if (value->first.compare(0, prefix.size(), prefix) == 0) return true;
		}
		return false;
	}

	bool readTable(lua_State* state, int table, const std::string& prefix)
	{
		table = lua_absindex(state, table);
		lua_pushnil(state);
		while (lua_next(state, table) != 0)
		{
			if (!lua_isstring(state, -2))
			{
				lua_pop(state, 2);
				return false;
			}
			std::string key = prefix.empty() ? lua_tostring(state, -2) :
				prefix + "." + lua_tostring(state, -2);
			if (lua_istable(state, -1))
			{
				if (!readTable(state, -1, key))
				{
					lua_pop(state, 2);
					return false;
				}
			}
			else if (lua_isnumber(state, -1))
				values()[key] = lua_tonumber(state, -1);
			else
			{
				fprintf(stderr, "AI parameter '%s' must be numeric or a table.\n", key.c_str());
				lua_pop(state, 2);
				return false;
			}
			lua_pop(state, 1);
		}
		return true;
	}
}

bool loadAiParams(lua_State* state)
{
	if (state == NULL) return false;
	int stackTop = lua_gettop(state);
	values().clear();
	lua_getglobal(state, "AIParams");
	bool loaded = lua_istable(state, -1) && readTable(state, -1, "");
	lua_settop(state, stackTop);
	if (!loaded || values().empty())
	{
		fprintf(stderr, "Unable to load numeric AIParams from Lua/AIParams.lua.\n");
		values().clear();
		return false;
	}
	return true;
}

double aiParam(const std::string& path)
{
	std::unordered_map<std::string, double>::const_iterator value = values().find(path);
	if (value != values().end()) return value->second;
	fprintf(stderr, "Missing required AI parameter '%s'.\n", path.c_str());
	return 0.0;
}

int aiIntParam(const std::string& path)
{
	double value = aiParam(path);
	if (!std::isfinite(value) || value < std::numeric_limits<int>::min() ||
		value > std::numeric_limits<int>::max())
		return 0;
	return static_cast<int>(std::lround(value));
}

std::uint32_t aiSeedParam(const std::string& path)
{
	double value = aiParam(path);
	if (!std::isfinite(value) || value < 0.0 ||
		value > std::numeric_limits<std::uint32_t>::max())
		return 0;
	return static_cast<std::uint32_t>(value);
}

bool hasAiPersonality(const std::string& personality)
{
	return hasProfile("personalities", personality);
}

bool hasAiDifficulty(const std::string& difficulty)
{
	return hasProfile("difficulties", difficulty);
}

double aiPersonalityParam(const std::string& personality,
	const std::string& path, double fallback)
{
	const std::string key = "personalities." + normalized(personality) + "." + path;
	std::unordered_map<std::string, double>::const_iterator value = values().find(key);
	return value == values().end() ? fallback : value->second;
}

int aiDifficultyIntParam(const std::string& difficulty,
	const std::string& path, int fallback)
{
	const std::string key = "difficulties." + normalized(difficulty) + "." + path;
	std::unordered_map<std::string, double>::const_iterator value = values().find(key);
	if (value == values().end() || !std::isfinite(value->second) ||
		value->second < std::numeric_limits<int>::min() ||
		value->second > std::numeric_limits<int>::max())
		return fallback;
	return static_cast<int>(std::lround(value->second));
}
