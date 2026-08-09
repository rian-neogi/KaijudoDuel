#include "Modifier.h"
#include "Duel.h"
#include "LuaTrace.h"

lua_State* LuaCards;

Modifier::Modifier() : mFuncRef(LUA_NOREF)
{
}

Modifier::Modifier(int ref) : mFuncRef(ref)
{
	//cout << "ref mod: " << funcref << endl;
}

Modifier::~Modifier()
{
	//cout << "unref mod: " << funcref << endl;
	if (LuaCards != NULL && mFuncRef != LUA_NOREF && mFuncRef != LUA_REFNIL)
		luaL_unref(LuaCards, LUA_REGISTRYINDEX, mFuncRef);
}

Modifier* Modifier::clone() const
{
	if (LuaCards == NULL || mFuncRef == LUA_NOREF || mFuncRef == LUA_REFNIL)
		return NULL;

	int stackTop = lua_gettop(LuaCards);
	lua_rawgeti(LuaCards, LUA_REGISTRYINDEX, mFuncRef);
	if (!lua_isfunction(LuaCards, -1))
	{
		lua_settop(LuaCards, stackTop);
		return NULL;
	}
	int clonedRef = luaL_ref(LuaCards, LUA_REGISTRYINDEX);
	lua_settop(LuaCards, stackTop);

	Modifier* result = new Modifier(clonedRef);
	result->mLuaRuleState = mLuaRuleState;
	return result;
}

void Modifier::setfunc(int ref)
{
	//cout << "ref mod: " << funcref << endl;
	mFuncRef = ref;
}

int Modifier::getLuaRuleState(const std::string& name, int fallback) const
{
	std::unordered_map<std::string, int>::const_iterator value = mLuaRuleState.find(name);
	return value == mLuaRuleState.end() ? fallback : value->second;
}

void Modifier::setLuaRuleState(const std::string& name, int value)
{
	mLuaRuleState[name] = value;
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
