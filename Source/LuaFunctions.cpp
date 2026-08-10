#include "LuaFunctions.h"
#include "LuaTrace.h"

Duel* ActiveDuel;

static Card* cardFromLua(lua_State* L, int argument)
{
	if (ActiveDuel == NULL)
		return NULL;
	int cid = static_cast<int>(lua_tointeger(L, argument));
	if (cid < 0 || cid >= static_cast<int>(ActiveDuel->mCardList.size()))
		return NULL;
	return ActiveDuel->mCardList[cid];
}

static Modifier* modifierFromLua(lua_State* L, int cardArgument, int modifierArgument)
{
	Card* card = cardFromLua(L, cardArgument);
	if (card == NULL)
		return NULL;
	int modifier = static_cast<int>(lua_tointeger(L, modifierArgument));
	if (modifier < 0 || modifier >= static_cast<int>(card->mModifiers.size()))
		return NULL;
	return card->mModifiers[modifier];
}

static bool validPlayer(int player)
{
	return player == 0 || player == 1;
}

static bool validZone(int zone)
{
	return zone >= ZONE_HAND && zone <= ZONE_EVOLVED;
}

static int printstr(lua_State* L)
{
	printf("%s", lua_tostring(L, 1));
	return 0;
}

static int printint(lua_State* L)
{
	printf("%lld", static_cast<long long>(lua_tointeger(L, 1)));
	return 0;
}

static int setMessageString(lua_State* L)
{
	ActiveDuel->mCurrentMessage.addValue(lua_tostring(L, 1), lua_tostring(L, 2));
	return 0;
}

static int setMessageInt(lua_State* L)
{
	ActiveDuel->mCurrentMessage.addValue(lua_tostring(L, 1), std::to_string(lua_tointeger(L, 2)));
	return 0;
}

static int getMessageString(lua_State* L)
{
	lua_pushstring(L, ActiveDuel->mCurrentMessage.getString(lua_tostring(L, 1)).c_str());
	return 1;
}

static int getMessageInt(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mCurrentMessage.getInt(lua_tostring(L, 1)));
	return 1;
}

static int getMessageType(lua_State* L)
{
	lua_pushstring(L, ActiveDuel->mCurrentMessage.getType().c_str());
	return 1;
}

static int getDuelStateInt(lua_State* L)
{
	int fallback = lua_gettop(L) >= 3 ? static_cast<int>(lua_tointeger(L, 3)) : 0;
	const char* name = lua_tostring(L, 1);
	if (ActiveDuel == NULL || name == NULL)
	{
		lua_pushinteger(L, fallback);
		return 1;
	}
	lua_pushinteger(L, ActiveDuel->getLuaRuleState(name, static_cast<int>(lua_tointeger(L, 2)), fallback));
	return 1;
}

static int setDuelStateInt(lua_State* L)
{
	const char* name = lua_tostring(L, 1);
	if (ActiveDuel != NULL && name != NULL)
		ActiveDuel->setLuaRuleState(name, static_cast<int>(lua_tointeger(L, 2)),
			static_cast<int>(lua_tointeger(L, 3)));
	return 0;
}

static int clearDuelState(lua_State* L)
{
	const char* name = lua_tostring(L, 1);
	if (ActiveDuel != NULL && name != NULL)
		ActiveDuel->clearLuaRuleState(name, static_cast<int>(lua_tointeger(L, 2)));
	return 0;
}

static int getShieldsBrokenThisTurn(lua_State* L)
{
	int player = static_cast<int>(lua_tointeger(L, 1));
	lua_pushinteger(L, ActiveDuel == NULL ? 0 : ActiveDuel->getShieldsBrokenThisTurn(player));
	return 1;
}

static int getModifierStateInt(lua_State* L)
{
	int fallback = lua_gettop(L) >= 4 ? static_cast<int>(lua_tointeger(L, 4)) : 0;
	Modifier* modifier = modifierFromLua(L, 1, 2);
	const char* name = lua_tostring(L, 3);
	if (modifier == NULL || name == NULL)
	{
		lua_pushinteger(L, fallback);
		return 1;
	}
	lua_pushinteger(L, modifier->getLuaRuleState(name, fallback));
	return 1;
}

static int setModifierStateInt(lua_State* L)
{
	Modifier* modifier = modifierFromLua(L, 1, 2);
	const char* name = lua_tostring(L, 3);
	if (modifier != NULL && name != NULL)
		modifier->setLuaRuleState(name, static_cast<int>(lua_tointeger(L, 4)));
	return 0;
}

static int createChoice(lua_State* L)
{
	// A choice may be created from inside a message callback after that callback
	// queued more work. Drain the work, but preserve the outer callback's message
	// context when the nested dispatch returns.
	Message outerMessage = ActiveDuel->mCurrentMessage;
	ActiveDuel->dispatchAllMessages(); //first resolve all pending messages
	ActiveDuel->mCurrentMessage = outerMessage;
	//lua_pushvalue(L, -1);
	assert(L == LuaCards);
	lua_pushvalue(L, 5);
	int vref = luaL_ref(L, LUA_REGISTRYINDEX);
	//lua_pushvalue(L, 6);
	//int aref = luaL_ref(L, LUA_REGISTRYINDEX);
	//cout << "ref: " << vref << " " << aref << endl;
	bool hasExplicitAiPreference = lua_gettop(L) >= 6 && lua_isnumber(L, 6);
	int aiPreferredSelection = hasExplicitAiPreference ?
		static_cast<int>(lua_tointeger(L, 6)) : RETURN_NOTHING;
	ActiveDuel->addChoice(lua_tostring(L, 1), lua_tointeger(L, 2), lua_tointeger(L, 3),
		lua_tointeger(L, 4), vref, -1, aiPreferredSelection);
	ActiveDuel->checkChoiceValid();
	if (ActiveDuel->mIsChoiceActive && !hasExplicitAiPreference)
	{
		int preferred = ActiveDuel->getCardAiPreferredChoice(ActiveDuel->mChoiceCard);
		bool legalButton = (preferred == RETURN_BUTTON1 && ActiveDuel->mChoice->mButtonCount >= 1) ||
			(preferred == RETURN_BUTTON2 && ActiveDuel->mChoice->mButtonCount >= 2);
		if (legalButton || (preferred >= 0 && ActiveDuel->choiceCanBeSelected(preferred)))
			ActiveDuel->mChoice->mAiPreferredSelection = preferred;
	}
	
	int result = ActiveDuel->mIsChoiceActive ? ActiveDuel->resolveChoice() : RETURN_NOVALID;
	// A live choice is completed by dispatching a choiceselect message while the
	// Lua callback is suspended. Restore the callback's message before Lua
	// resumes, just as we do after draining work above.
	ActiveDuel->mCurrentMessage = outerMessage;
	lua_pushinteger(L, result);
	return 1;
	//if (ActiveDuel->isChoiceActive) //if choice is still active
	//{
	//	int r = mainLoop(*Window, 1); //wait for selection made by user
	//	lua_pushinteger(L, r);
	//	return 1;
	//}
	//else
	//{
	//	lua_pushinteger(L, RETURN_NOVALID);
	//}
	//luaL_unref(L, LUA_REGISTRYINDEX, ref);
	//return 1;
}

static int createChoiceNoCheck(lua_State* L)
{
	Message outerMessage = ActiveDuel->mCurrentMessage;
	ActiveDuel->dispatchAllMessages(); //first resolve all pending messages
	ActiveDuel->mCurrentMessage = outerMessage;
	//lua_pushvalue(L, -1);
	lua_pushvalue(L, 5);
	int vref = luaL_ref(L, LUA_REGISTRYINDEX);
	//lua_pushvalue(L, 6);
	//int aref = luaL_ref(L, LUA_REGISTRYINDEX);
	//cout << "ref: " << vref << " " << aref << endl;
	int aiPreferredSelection = lua_gettop(L) >= 6 && lua_isnumber(L, 6) ?
		static_cast<int>(lua_tointeger(L, 6)) : RETURN_NOTHING;
	ActiveDuel->addChoice(lua_tostring(L, 1), lua_tointeger(L, 2), lua_tointeger(L, 3),
		lua_tointeger(L, 4), vref, -1, aiPreferredSelection);
	//ActiveDuel->checkChoiceValid();
	//if (ActiveDuel->isChoiceActive) //if choice is still active
	//{
	//	int r = mainLoop(*Window, 1); //wait for selection made by user
	//	lua_pushinteger(L, r);
	//	return 1;
	//}
	//else
	//{
	//	lua_pushinteger(L, RETURN_NOVALID);
	//}
	//luaL_unref(L, LUA_REGISTRYINDEX, ref);
	//return 1;
	int result = ActiveDuel->mIsChoiceActive ? ActiveDuel->resolveChoice() : RETURN_NOVALID;
	ActiveDuel->mCurrentMessage = outerMessage;
	lua_pushinteger(L, result);
	return 1;
	//return 0;
}

static int setChoiceActive(lua_State* L)
{
	ActiveDuel->mIsChoiceActive = lua_tointeger(L, 1);
	if (ActiveDuel->mIsChoiceActive == false)
	{
		ActiveDuel->mChoiceCard = -1;
	}
	return 0;
}

static int isChoiceActive(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mIsChoiceActive);
	return 1;
}

static int getChoice(lua_State* L)
{
	//int r = mainLoop(*Window, 1);
	//lua_pushinteger(L, r);
	return 1;
}

//static int destroyCard(lua_State* L)
//{
//	Message msg("carddestroy");
//	msg.addValue("card", lua_tointeger(L, 1));
//	msg.addValue("zoneto", ZONE_GRAVEYARD);
//	ActiveDuel->MsgMngr.sendMessage(msg);
//	return 0;
//}

static int destroyCreature(lua_State* L)
{
	int cid = lua_tointeger(L, 1);
	Card* card = cardFromLua(L, 1);
	if (card == NULL)
		return 0;
	Message msg("creaturedestroy");
	msg.addValue("creature", cid);
	msg.addValue("zoneto", ZONE_GRAVEYARD);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	if (card->mZone != ZONE_BATTLE)
	{
		printf("WARNING: destroyCreature called on creature that is not in battle zone\n");
	}
	if (card->mType != TYPE_CREATURE)
	{
		printf("WARNING: destroyCreature called on card that is not a creature\n");
	}

	return 0;
}

static int discardCard(lua_State* L)
{
	int cid = lua_tointeger(L, 1);
	Card* card = cardFromLua(L, 1);
	if (card == NULL)
		return 0;
	Message msg("carddiscard");
	msg.addValue("card", cid);
	msg.addValue("zoneto", ZONE_GRAVEYARD);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	if (card->mZone != ZONE_HAND)
	{
		printf("WARNING: discardCard called on card that is not in hand\n");
	}
	return 0;
}

static int destroyMana(lua_State* L)
{
	int cid = lua_tointeger(L, 1);
	Card* card = cardFromLua(L, 1);
	if (card == NULL)
		return 0;
	Message msg("manadestroy");
	msg.addValue("card", cid);
	msg.addValue("zoneto", ZONE_GRAVEYARD);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	if (card->mZone != ZONE_MANA)
	{
		printf("WARNING: destroyMana called on card that is not in mana zone\n");
	}
	return 0;
}

static int discardCardAtRandom(lua_State* L)
{
	int player = lua_tointeger(L, 1);
	if (!validPlayer(player))
		return 0;
	Message msg("carddiscardatrandom");
	msg.addValue("player", player);
	msg.addValue("count", lua_gettop(L) >= 2 ? lua_tointeger(L, 2) : 1);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}


static int moveCard(lua_State* L)
{
	int cid = lua_tointeger(L, 1);
	int zone = lua_tointeger(L, 2);
	if (cardFromLua(L, 1) == NULL || !validZone(zone))
		return 0;
	Message msg("cardmove");
	msg.addValue("card", cid);
	msg.addValue("to", zone);
	msg.addValue("tobottom", lua_gettop(L) >= 3 ? lua_tointeger(L, 3) : 0);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}

static int tapCard(lua_State* L)
{
	if (cardFromLua(L, 1) == NULL)
		return 0;
	Message msg("cardtap");
	msg.addValue("card", lua_tointeger(L, 1));
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}

static int untapCard(lua_State* L)
{
	if (cardFromLua(L, 1) == NULL)
		return 0;
	Message msg("carduntap");
	msg.addValue("card", lua_tointeger(L, 1));
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}

static int drawCards(lua_State* L)
{
	int player = lua_tointeger(L, 1);
	int count = lua_tointeger(L, 2);
	if (!validPlayer(player) || count <= 0)
		return 0;
	ActiveDuel->drawCards(player, count);
	return 0;
}

static int createModifier(lua_State* L)
{
	int uid = lua_tointeger(L, 1);
	if (cardFromLua(L, 1) == NULL)
		return 0;
	lua_pushvalue(L, 2);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	/*Modifier m(ref);
	ActiveDuel->CardList.at(uid)->modifiers.push_back(m);*/
	Message msg("modifiercreate");
	msg.addValue("card", uid);
	msg.addValue("funcref", ref);
	if (lua_istable(L, 3))
	{
		lua_pushnil(L);
		while (lua_next(L, 3) != 0)
		{
			if (lua_type(L, -2) == LUA_TSTRING && lua_isnumber(L, -1))
			{
				const char* name = lua_tostring(L, -2);
				msg.addValue(std::string("state.") + name, static_cast<int>(lua_tointeger(L, -1)));
			}
			lua_pop(L, 1);
		}
	}
	ActiveDuel->mMsgMngr.sendMessage(msg);

	return 0;
}

static int destroyModifier(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	int modifier = lua_tointeger(L, 2);
	if (card == NULL || modifier < 0 || modifier >= static_cast<int>(card->mModifiers.size()))
		return 0;
	Message msg("modifierdestroy");
	msg.addValue("card", lua_tointeger(L, 1));
	msg.addValue("modifier", modifier);
	msg.addValue("funcref", card->mModifiers[modifier]->mFuncRef);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	
	return 0;
}

static int shuffleDeck(lua_State* L)
{
	if (!validPlayer(lua_tointeger(L, 1)))
		return 0;
	Message msg("deckshuffle");
	msg.addValue("player", lua_tointeger(L, 1));
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}

static int openDeck(lua_State* L)
{
	//ActiveDuel->cardsearch.zone = ActiveDuel->getZone(lua_tointeger(L, 1), ZONE_DECK);
	return 0;
}

static int closeDeck(lua_State* L)
{
	//ActiveDuel->cardsearch.zone = NULL;
	return 0;
}

static int flipCard(lua_State* L)
{
	if (cardFromLua(L, 1) == NULL)
		return 0;
	ActiveDuel->flipCard(lua_tointeger(L, 1));
	return 0;
}

static int unflipCard(lua_State* L)
{
	if (cardFromLua(L, 1) == NULL)
		return 0;
	ActiveDuel->unflipCard(lua_tointeger(L, 1));
	return 0;
}

static int setCardVisibility(lua_State* L)
{
	if (cardFromLua(L, 1) == NULL || !validPlayer(lua_tointeger(L, 2)))
		return 0;
	ActiveDuel->setCardVisibility(lua_tointeger(L, 1), lua_tointeger(L, 2), lua_tointeger(L, 3));
	return 0;
}

static int seperateEvolution(lua_State* L)
{
	if (cardFromLua(L, 1) == NULL)
		return 0;
	Message msg("evolutionseperate");
	msg.addValue("evolution", lua_tointeger(L, 1));
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}

static int creatureBreakShield(lua_State* L)
{
	int creature = lua_tointeger(L, 1);
	int shield = lua_tointeger(L, 2);
	Card* shieldCard = cardFromLua(L, 2);
	if (cardFromLua(L, 1) == NULL || shieldCard == NULL || shieldCard->mZone != ZONE_SHIELD)
		return 0;
	Message msg("creaturebreakshield");
	msg.addValue("creature", creature);
	msg.addValue("attacker", creature);
	msg.addValue("defender", shieldCard->mOwner);
	msg.addValue("shield", shield);
	ActiveDuel->mMsgMngr.sendMessage(msg);
	return 0;
}

static int getCardAt(lua_State* L)
{
	int p = lua_tointeger(L, 1);
	int z = lua_tointeger(L, 2);
	int id = lua_tointeger(L, 3);
	Zone* zone = validPlayer(p) && validZone(z) ? ActiveDuel->getZone(p, z) : NULL;
	if (zone == NULL || id < 0 || id >= static_cast<int>(zone->mCards.size()))
	{
		lua_pushinteger(L, -1);
	}
	else
	{
		lua_pushinteger(L, zone->mCards[id]->mUniqueId);
	}

	return 1;
}

static int getTotalCardCount(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mNextUniqueId);
	return 1;
}

static int getZoneSize(lua_State* L)
{
	int p = lua_tointeger(L, 1);
	int z = lua_tointeger(L, 2);
	Zone* zone = validPlayer(p) && validZone(z) ? ActiveDuel->getZone(p, z) : NULL;
	lua_pushinteger(L, zone == NULL ? 0 : static_cast<int>(zone->mCards.size()));

	return 1;
}

static int getTurn(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mTurn);
	return 1;
}

static int getCardName(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushstring(L, card == NULL ? "" : card->mName.c_str());
	return 1;
}

static int getCardZone(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? -1 : card->mZone);
	return 1;
}

static int getCardCiv(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? -1 : card->mCivilization);
	return 1;
}

static int cardHasCivilization(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	int civilization = (int)lua_tointeger(L, 2);
	lua_pushboolean(L, card != NULL && civilization >= CIV_LIGHT &&
		civilization <= CIV_DARKNESS &&
		(card->mCivilizations & (1 << civilization)) != 0);
	return 1;
}

static int getCardCost(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : ActiveDuel->getCardCost(card->mUniqueId));
	return 1;
}

static int getCardIsShieldTrigger(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : ActiveDuel->getIsShieldTrigger(card->mUniqueId));
	return 1;
}

static int loseGame(lua_State* L)
{
	int player = lua_tointeger(L, 1);
	if (player == 0 || player == 1)
		ActiveDuel->mWinner = getOpponent(player);
	return 0;
}

static int getCardType(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? -1 : card->mType);
	return 1;
}

static int getCreatureRace(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	std::string race = card == NULL ? "" : ActiveDuel->getCreatureRace(card->mUniqueId);
	lua_pushstring(L, race.c_str());
	return 1;
}

static int isCreatureOfRace(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	const char* race = lua_tostring(L, 2);
	lua_pushinteger(L, card == NULL || race == NULL ? 0 : ActiveDuel->isCreatureOfRace(card->mUniqueId, race));
	return 1;
}

static int getCardOwner(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? -1 : card->mOwner);
	return 1;
}

static int getCreaturePower(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : ActiveDuel->getCreaturePower(card->mUniqueId));
	return 1;
}

static int getCreatureCanBlock(lua_State* L)
{
	Card* attacker = cardFromLua(L, 1);
	Card* blocker = cardFromLua(L, 2);
	lua_pushinteger(L, attacker == NULL || blocker == NULL ? 0 :
		ActiveDuel->getCreatureCanBlock(attacker->mUniqueId, blocker->mUniqueId));
	return 1;
}

static int getCreatureIsBlocker(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : ActiveDuel->getCreatureIsBlocker(card->mUniqueId));
	return 1;
}

static int getCreatureIsEvolution(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : ActiveDuel->getIsEvolution(card->mUniqueId));
	return 1;
}

static int getCreatureHasTapAbility(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : ActiveDuel->getCreatureHasTapAbility(card->mUniqueId));
	return 1;
}

static int isCardTapped(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : card->mIsTapped);
	return 1;
}

static int getAttacker(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mAttacker);
	return 1;
}

static int getDefender(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mDefender);
	return 1;
}

static int getDefenderType(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->mDefenderType);
	return 1;
}

static int hasOtherCreatureBrokenShieldThisTurn(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card != NULL && ActiveDuel->hasOtherCreatureBrokenShieldThisTurn(card->mUniqueId) ? 1 : 0);
	return 1;
}

static int hasCreatureBrokenShieldThisTurn(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card != NULL && ActiveDuel->hasCreatureBrokenShieldThisTurn(card->mUniqueId) ? 1 : 0);
	return 1;
}

static int getCardsDrawnThisTurn(lua_State* L)
{
	lua_pushinteger(L, ActiveDuel->getCardsDrawnThisTurn(lua_tointeger(L, 1)));
	return 1;
}

static int getEvoStackSize(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	lua_pushinteger(L, card == NULL ? 0 : static_cast<int>(card->mEvoStack.size()));
	return 1;
}

static int getEvoStackAt(lua_State* L)
{
	Card* card = cardFromLua(L, 1);
	int index = lua_tointeger(L, 2);
	lua_pushinteger(L, card == NULL || index < 0 || index >= static_cast<int>(card->mEvoStack.size()) ?
		-1 : card->mEvoStack[index]->mUniqueId);
	return 1;
}

static int tracedLuaBridge(lua_State* L)
{
	int argumentCount = lua_gettop(L);
	const char* function = lua_tostring(L, lua_upvalueindex(2));
	LuaTrace::logBridgeCall(L, function, argumentCount);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	int status = lua_pcall(L, argumentCount, LUA_MULTRET, 0);
	if (status != LUA_OK)
	{
		LuaTrace::logBridgeError(L, function);
		return lua_error(L);
	}
	int resultCount = lua_gettop(L);
	LuaTrace::logBridgeReturn(L, function, resultCount);
	return resultCount;
}

static void registerTracedLuaFunction(lua_State* L, const char* name, lua_CFunction function)
{
	lua_pushcfunction(L, function);
	lua_pushstring(L, name);
	lua_pushcclosure(L, tracedLuaBridge, 2);
	lua_setglobal(L, name);
}

#undef lua_register
#define lua_register(state, name, function) registerTracedLuaFunction(state, name, function)

void registerLua(lua_State* L)
{
	lua_register(L, "printstr", printstr);
	lua_register(L, "printint", printint);

	lua_register(L, "setMessageString", setMessageString);
	lua_register(L, "setMessageInt", setMessageInt);
	lua_register(L, "getMessageString", getMessageString);
	lua_register(L, "getMessageInt", getMessageInt);
	lua_register(L, "getMessageType", getMessageType);
	lua_register(L, "getDuelStateInt", getDuelStateInt);
	lua_register(L, "setDuelStateInt", setDuelStateInt);
	lua_register(L, "clearDuelState", clearDuelState);
	lua_register(L, "getShieldsBrokenThisTurn", getShieldsBrokenThisTurn);

	lua_register(L, "createChoice", createChoice);
	lua_register(L, "createChoiceNoCheck", createChoiceNoCheck);
	lua_register(L, "setChoiceActive", setChoiceActive);
	lua_register(L, "isChoiceActive", isChoiceActive);
	lua_register(L, "getChoice", getChoice);

	lua_register(L, "createModifier", createModifier);
	lua_register(L, "destroyModifier", destroyModifier);
	lua_register(L, "getModifierStateInt", getModifierStateInt);
	lua_register(L, "setModifierStateInt", setModifierStateInt);
	
	lua_register(L, "destroyCreature", destroyCreature);
	lua_register(L, "discardCard", discardCard);
	lua_register(L, "destroyMana", destroyMana);
	lua_register(L, "discardCardAtRandom", discardCardAtRandom);
	lua_register(L, "moveCard", moveCard);
	lua_register(L, "tapCard", tapCard);
	lua_register(L, "untapCard", untapCard);
	lua_register(L, "drawCards", drawCards);
	lua_register(L, "shuffleDeck", shuffleDeck);
	lua_register(L, "openDeck", openDeck);
	lua_register(L, "closeDeck", closeDeck);
	lua_register(L, "flipCard", flipCard);
	lua_register(L, "unflipCard", unflipCard);
	lua_register(L, "setCardVisibility", setCardVisibility);
	lua_register(L, "seperateEvolution", seperateEvolution);
	lua_register(L, "creatureBreakShield", creatureBreakShield);

	lua_register(L, "getCardAt", getCardAt);
	lua_register(L, "getTotalCardCount", getTotalCardCount);
	lua_register(L, "getZoneSize", getZoneSize);
	lua_register(L, "getTurn", getTurn);
	lua_register(L, "getCardName", getCardName);
	lua_register(L, "getCardZone", getCardZone);
	lua_register(L, "getCardCiv", getCardCiv);
	lua_register(L, "cardHasCivilization", cardHasCivilization);
	lua_register(L, "getCardCost", getCardCost);
	lua_register(L, "getCardIsShieldTrigger", getCardIsShieldTrigger);
	lua_register(L, "getCardType", getCardType);
	lua_register(L, "getCreatureRace", getCreatureRace); //returns the full race string of the creature
	lua_register(L, "isCreatureOfRace", isCreatureOfRace);
	lua_register(L, "getCardOwner", getCardOwner);
	lua_register(L, "getCreaturePower", getCreaturePower);
	lua_register(L, "getCreatureCanBlock", getCreatureCanBlock);
	lua_register(L, "getCreatureIsBlocker", getCreatureIsBlocker);
	lua_register(L, "getCreatureIsEvolution", getCreatureIsEvolution);
	lua_register(L, "getCreatureHasTapAbility", getCreatureHasTapAbility);
	lua_register(L, "isCardTapped", isCardTapped);
	lua_register(L, "getAttacker", getAttacker);
	lua_register(L, "getDefender", getDefender);
	lua_register(L, "getDefenderType", getDefenderType);
	lua_register(L, "hasOtherCreatureBrokenShieldThisTurn", hasOtherCreatureBrokenShieldThisTurn);
	lua_register(L, "hasCreatureBrokenShieldThisTurn", hasCreatureBrokenShieldThisTurn);
	lua_register(L, "getCardsDrawnThisTurn", getCardsDrawnThisTurn);
	lua_register(L, "getEvoStackSize", getEvoStackSize);
	lua_register(L, "getEvoStackAt", getEvoStackAt);
	lua_register(L, "loseGame", loseGame);
}
