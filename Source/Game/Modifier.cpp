#include "Modifier.h"
#include "Duel.h"
#include "LuaTrace.h"

lua_State* LuaCards;

Modifier::Modifier()
{
}

Modifier::Modifier(int ref) : mFuncRef(ref)
{
	//cout << "ref mod: " << funcref << endl;
}

Modifier::~Modifier()
{
	//cout << "unref mod: " << funcref << endl;
	luaL_unref(LuaCards, LUA_REGISTRYINDEX, mFuncRef);
}

void Modifier::setfunc(int ref)
{
	//cout << "ref mod: " << funcref << endl;
	mFuncRef = ref;
}

int Modifier::handleMessage(int cid, int mid, Message& msg)
{
	/*int size = func.size();
	if (size > 0)
	{
		lua_getglobal(LuaCards, func.at(0).c_str());
		for (int i = 1; i < size; i++)
		{
			lua_getfield(LuaCards, -1, func.at(i).c_str());
		}
		lua_pushinteger(LuaCards, cid);
		lua_pushinteger(LuaCards, mid);
		lua_pcall(LuaCards, 2, 0, 0);
		for (int i = 1; i < size; i++)
		{
			lua_pop(LuaCards, 1);
		}
	}
	return 0;*/

	int stackTop = lua_gettop(LuaCards);
	std::string subject = "modifier";
	if (ActiveDuel != NULL && cid >= 0 && cid < static_cast<int>(ActiveDuel->mCardList.size()))
		subject = ActiveDuel->mCardList[cid]->mName + " modifier " + std::to_string(mid);
	const Message& traceMessage = ActiveDuel == NULL ? msg : ActiveDuel->mCurrentMessage;
	LuaTrace::logCallback("engine -> lua", "Modifier", subject, cid, traceMessage);
	lua_rawgeti(LuaCards, LUA_REGISTRYINDEX, mFuncRef);
	lua_pushinteger(LuaCards, cid);
	lua_pushinteger(LuaCards, mid);
	int status = lua_pcall(LuaCards, 2, 0, 0);
	if (status != LUA_OK)
	{
		const char* error = lua_tostring(LuaCards, -1);
		LuaTrace::logCallback("lua -> engine", "Modifier", subject, cid,
			ActiveDuel == NULL ? msg : ActiveDuel->mCurrentMessage, error);
		fprintf(stderr, "Lua modifier error while handling '%s': %s\n",
			msg.getType().c_str(), error == NULL ? "unknown error" : error);
	}
	else
	{
		LuaTrace::logCallback("lua -> engine", "Modifier", subject, cid,
			ActiveDuel == NULL ? msg : ActiveDuel->mCurrentMessage);
	}
	lua_settop(LuaCards, stackTop);
	return 0;
}
