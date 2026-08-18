#pragma once

#include "LuaInclude.h"
#include "Game/Message.h"

#include <string>

namespace LuaTrace
{
	class ScopedSuppression
	{
	public:
		ScopedSuppression();
		~ScopedSuppression();

		ScopedSuppression(const ScopedSuppression&) = delete;
		ScopedSuppression& operator=(const ScopedSuppression&) = delete;
	};

	void setEnabled(bool enabled);
	bool isEnabled();
	void logCallback(const char* direction, const char* callback, const std::string& subject,
		int cardId, const Message& message, const char* error = NULL);
	void logBridgeCall(lua_State* state, const char* function, int argumentCount);
	void logBridgeReturn(lua_State* state, const char* function, int resultCount);
	void logBridgeError(lua_State* state, const char* function);
}
