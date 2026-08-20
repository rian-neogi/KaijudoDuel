#pragma once

#include "LuaInclude.h"

#include <string>

// Loads the immutable numeric AIParams Lua table. Call once during Lua/card
// initialization, before constructing search configurations.
bool loadAiParams(lua_State* state);
double aiParam(const std::string& path);
double aiParam(const std::string& path, const std::string& personality);
int aiIntParam(const std::string& path);
int aiIntParam(const std::string& path, const std::string& personality);

// Profile names are case-insensitive. "default" is reserved for the unmodified
// base tables. New profiles can be added entirely in Lua as sparse
// evaluation/search/heuristic overrides; unspecified values use the base.
bool hasAiPersonality(const std::string& personality);
bool hasAiDifficulty(const std::string& difficulty);
double aiPersonalityParam(const std::string& personality,
	const std::string& path);
int aiDifficultyIntParam(const std::string& difficulty,
	const std::string& path, int fallback = 0);

// Applies one personality to every ordinary aiParam/aiIntParam lookup on the
// current thread. Scopes may be nested (for example, opponent rollout policy).
class AiPersonalityScope
{
public:
	explicit AiPersonalityScope(const std::string& personality);
	~AiPersonalityScope();

	AiPersonalityScope(const AiPersonalityScope&) = delete;
	AiPersonalityScope& operator=(const AiPersonalityScope&) = delete;

private:
	std::string mPreviousPersonality;
};
