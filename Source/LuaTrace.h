#pragma once

#include "LuaInclude.h"
#include "Game/Message.h"

#include <string>

namespace LuaTrace
{
	void setEnabled(bool enabled);
	bool isEnabled();
	void logCallback(const char* direction, const char* callback, const std::string& subject,
		int cardId, const Message& message, const char* error = NULL);
	void logBridgeCall(lua_State* state, const char* function, int argumentCount);
	void logBridgeReturn(lua_State* state, const char* function, int resultCount);
	void logBridgeError(lua_State* state, const char* function);
}
