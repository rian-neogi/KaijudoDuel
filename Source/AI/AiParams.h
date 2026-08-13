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
