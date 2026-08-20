#include "AiParams.h"

#include <cmath>
#include <cctype>
#include <cstdio>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace
{
	std::unordered_map<std::string, double>& values()
	{
		static std::unordered_map<std::string, double> result;
		return result;
	}

	std::string& activePersonality()
	{
		static thread_local std::string result;
		return result;
	}

	std::unordered_set<std::string>& personalityNames()
	{
		static std::unordered_set<std::string> result;
		return result;
	}

	std::unordered_set<std::string>& difficultyNames()
	{
		static std::unordered_set<std::string> result;
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
		const std::unordered_set<std::string>& names = group == "personalities" ?
			personalityNames() : difficultyNames();
		return names.find(normalized(name)) != names.end();
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
				if (prefix == "personalities")
					personalityNames().insert(normalized(lua_tostring(state, -2)));
				else if (prefix == "difficulties")
					difficultyNames().insert(normalized(lua_tostring(state, -2)));
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

	bool validatePersonalityOverrides()
	{
		const std::string prefix = "personalities.";
		for (std::unordered_map<std::string, double>::const_iterator value =
			values().begin(); value != values().end(); ++value)
		{
			if (value->first.compare(0, prefix.size(), prefix) != 0) continue;
			size_t profileEnd = value->first.find('.', prefix.size());
			if (profileEnd == std::string::npos || profileEnd + 1 >= value->first.size())
				return false;
			std::string basePath = value->first.substr(profileEnd + 1);
			if (values().find(basePath) == values().end())
			{
				fprintf(stderr, "AI personality parameter '%s' has no base parameter.\n",
					value->first.c_str());
				return false;
			}
		}
		return true;
	}

	double baseParam(const std::string& path)
	{
		std::unordered_map<std::string, double>::const_iterator value = values().find(path);
		if (value != values().end()) return value->second;
		fprintf(stderr, "Missing required AI parameter '%s'.\n", path.c_str());
		return 0.0;
	}

	int roundedInt(double value)
	{
		if (!std::isfinite(value) || value < std::numeric_limits<int>::min() ||
			value > std::numeric_limits<int>::max())
			return 0;
		return static_cast<int>(std::lround(value));
	}
}

bool loadAiParams(lua_State* state)
{
	if (state == NULL) return false;
	int stackTop = lua_gettop(state);
	values().clear();
	personalityNames().clear();
	difficultyNames().clear();
	lua_getglobal(state, "AIParams");
	bool loaded = lua_istable(state, -1) && readTable(state, -1, "");
	lua_settop(state, stackTop);
	if (!loaded || values().empty() || !validatePersonalityOverrides())
	{
		fprintf(stderr, "Unable to load numeric AIParams from Lua/AIParams.lua.\n");
		values().clear();
		personalityNames().clear();
		difficultyNames().clear();
		return false;
	}
	return true;
}

double aiParam(const std::string& path)
{
	return aiParam(path, activePersonality());
}

double aiParam(const std::string& path, const std::string& personality)
{
	const std::string profile = normalized(personality);
	if (!profile.empty() && profile != "default")
	{
		const std::string key = "personalities." + profile + "." + path;
		std::unordered_map<std::string, double>::const_iterator value = values().find(key);
		if (value != values().end()) return value->second;
	}
	return baseParam(path);
}

int aiIntParam(const std::string& path)
{
	return roundedInt(aiParam(path));
}

int aiIntParam(const std::string& path, const std::string& personality)
{
	return roundedInt(aiParam(path, personality));
}

bool hasAiPersonality(const std::string& personality)
{
	return normalized(personality) == "default" ||
		hasProfile("personalities", personality);
}

bool hasAiDifficulty(const std::string& difficulty)
{
	return hasProfile("difficulties", difficulty);
}

double aiPersonalityParam(const std::string& personality,
	const std::string& path)
{
	return aiParam(path, personality);
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

AiPersonalityScope::AiPersonalityScope(const std::string& personality)
	: mPreviousPersonality(activePersonality())
{
	activePersonality() = normalized(personality);
}

AiPersonalityScope::~AiPersonalityScope()
{
	activePersonality() = mPreviousPersonality;
}
