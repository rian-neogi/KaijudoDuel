#pragma once

#include "MessageManager.h"
#include "LuaInclude.h"

#include <string>
#include <unordered_map>

extern lua_State* LuaCards;

class Modifier
{
public:
	//vector<string> func;
	int mFuncRef;
	std::unordered_map<std::string, int> mLuaRuleState;

	Modifier();
	Modifier(int ref);
	~Modifier();

	Modifier(const Modifier&) = delete;
	Modifier& operator=(const Modifier&) = delete;

	Modifier* clone() const;
	void setfunc(int ref);
	int getLuaRuleState(const std::string& name, int fallback) const;
	void setLuaRuleState(const std::string& name, int value);
	int handleMessage(int cid, int mid, Message& msg);
};

void registerLua(lua_State* L);

