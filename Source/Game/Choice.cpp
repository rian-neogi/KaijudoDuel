#include "Choice.h"

Choice::Choice() : mInfotext(""), mButtonCount(0)
{
}

Choice::Choice(std::string info, int bcount, int vr, int ar) : mInfotext(info), mButtonCount(bcount), mValidRef(vr), mActionRef(ar), mIsCopy(false)
{
}

Choice::~Choice()
{
	if (!mIsCopy)
	{
		luaL_unref(LuaCards, LUA_REGISTRYINDEX, mValidRef);
		luaL_unref(LuaCards, LUA_REGISTRYINDEX, mActionRef);
		//cout << "unref " << validref << " " << actionref << endl;
	}
}

int Choice::callvalid(int cid, int sid)
{
	int r = -1;
	int stackTop = lua_gettop(LuaCards);
	lua_rawgeti(LuaCards, LUA_REGISTRYINDEX, mValidRef);
	lua_pushinteger(LuaCards, cid);
	lua_pushinteger(LuaCards, sid);
	int status = lua_pcall(LuaCards, 2, 1, 0);
	if (status == LUA_OK)
		r = lua_tointeger(LuaCards, -1);
	else
	{
		const char* error = lua_tostring(LuaCards, -1);
		fprintf(stderr, "Lua choice validation error: %s\n", error == NULL ? "unknown error" : error);
	}
	lua_settop(LuaCards, stackTop);
	if (r == -1)
	{
		//cout << "ERROR callvalid returning -1" << endl;
	}
	return r;
}

void Choice::callaction(int cid, int sid)
{
	int stackTop = lua_gettop(LuaCards);
	lua_rawgeti(LuaCards, LUA_REGISTRYINDEX, mActionRef);
	lua_pushinteger(LuaCards, cid);
	lua_pushinteger(LuaCards, sid);
	int status = lua_pcall(LuaCards, 2, 0, 0);
	if (status != LUA_OK)
	{
		const char* error = lua_tostring(LuaCards, -1);
		fprintf(stderr, "Lua choice action error: %s\n", error == NULL ? "unknown error" : error);
	}
	lua_settop(LuaCards, stackTop);
}

void Choice::copyFrom(Choice* c)
{
	mInfotext = c->mInfotext;
	mButtonCount = c->mButtonCount;
	mValidRef = c->mValidRef;
	mActionRef = c->mActionRef;
	mIsCopy = true;
}

