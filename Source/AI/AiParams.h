#pragma once

#include "LuaInclude.h"

#include <cstdint>
#include <string>

// Loads the immutable numeric AIParams Lua table. Call once during Lua/card
// initialization, before constructing search configurations.
bool loadAiParams(lua_State* state);
double aiParam(const std::string& path);
int aiIntParam(const std::string& path);
std::uint32_t aiSeedParam(const std::string& path);

// Profile names are case-insensitive. New profiles can be added entirely in
// Lua as long as they contain at least one numeric parameter.
bool hasAiPersonality(const std::string& personality);
bool hasAiDifficulty(const std::string& difficulty);
double aiPersonalityParam(const std::string& personality,
	const std::string& path, double fallback = 0.0);
int aiDifficultyIntParam(const std::string& difficulty,
	const std::string& path, int fallback = 0);
