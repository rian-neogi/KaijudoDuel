#include "Duel.h"
#include "SoundManager.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <unordered_map>

std::mutex gMutex;

static int optionalMessageInt(const Message& message, const char* key, int fallback)
{
	std::map<std::string, std::string>::const_iterator value = message.map.find(key);
	return value == message.map.end() ? fallback : std::atoi(value->second.c_str());
}

static bool remapCardVector(const std::vector<Card*>& source,
	const std::unordered_map<const Card*, Card*>& cardMap, std::vector<Card*>& result)
{
	result.clear();
	result.reserve(source.size());
	for (std::vector<Card*>::const_iterator card = source.begin(); card != source.end(); ++card)
	{
		std::unordered_map<const Card*, Card*>::const_iterator clone = cardMap.find(*card);
		if (clone == cardMap.end())
			return false;
		result.push_back(clone->second);
	}
	return true;
}

Duel::Duel()
{
	mInputLoopRunning = true;
	mLuaCallbackSuspended = false;
	mAiThinking = false;
	mAiThinkingPlayer = -1;
	mTurn = 0;
	mTurnPhase = TURN_PHASE_MANA;
	mManaUsed = 0;
	mNextUniqueId = 0;

	mPlayerType[0] = PLAYER_HUMAN;
	mPlayerType[1] = PLAYER_AI;

	int Factor[2] = { -1, 1 };
	int Factor2[2] = { 1,0 };
	for (int i = 0; i < 2; i++)
	{
		mDecks[i].mRandomGen = &mRandomGen; //set random function for deck shuffling

		mDecks[i].mOwner = i;
		mBattlezones[i].mOwner = i;
		mGraveyards[i].mOwner = i;
		mHands[i].mOwner = i;
		mManazones[i].mOwner = i;
		mShields[i].mOwner = i;
		mShieldsBrokenThisTurn[i] = 0;
		mCardsDrawnThisTurn[i] = 0;

	//	decks[i].x = ZONE2X;
	//	graveyards[i].x = ZONE2X;
	//	hands[i].x = ZONE1X;
	//	manazones[i].x = ZONE1X;
	//	shields[i].x = ZONE1X;
	//	battlezones[i].x = ZONE1X;

	//	hands[i].y = CENTER + ZONEYOFFSET*Factor[i] * 4 - ZONEYOFFSET*Factor2[i];
	//	decks[i].y = CENTER + ZONEYOFFSET*Factor[i] * 3 - ZONEYOFFSET*Factor2[i];
	//	manazones[i].y = CENTER + ZONEYOFFSET*Factor[i] * 3 - ZONEYOFFSET*Factor2[i];
	//	shields[i].y = CENTER + ZONEYOFFSET*Factor[i] * 2 - ZONEYOFFSET*Factor2[i];
	//	graveyards[i].y = CENTER + ZONEYOFFSET*Factor[i] * 2 - ZONEYOFFSET*Factor2[i];
	//	battlezones[i].y = CENTER + ZONEYOFFSET*Factor[i] - ZONEYOFFSET*Factor2[i];

	//	decks[i].rect = sf::RectangleShape(sf::Vector2f(CARDSEPERATION + CARDZONEOFFSET, CARDSEPERATION + CARDZONEOFFSET));
	//	graveyards[i].rect = sf::RectangleShape(sf::Vector2f(CARDSEPERATION + CARDZONEOFFSET, CARDSEPERATION + CARDZONEOFFSET));
	//	hands[i].rect = sf::RectangleShape(sf::Vector2f(ZONE2X - ZONE1X - 10, CARDSEPERATION + CARDZONEOFFSET));
	//	manazones[i].rect = sf::RectangleShape(sf::Vector2f(ZONE2X - ZONE1X - 10, CARDSEPERATION + CARDZONEOFFSET));
	//	shields[i].rect = sf::RectangleShape(sf::Vector2f(ZONE2X - ZONE1X - 10, CARDSEPERATION + CARDZONEOFFSET));
	//	battlezones[i].rect = sf::RectangleShape(sf::Vector2f(ZONE2X - ZONE1X - 10, CARDSEPERATION + CARDZONEOFFSET));

	//	decks[i].rect.setPosition(decks[i].x, decks[i].y);
	//	hands[i].rect.setPosition(hands[i].x, hands[i].y);
	//	manazones[i].rect.setPosition(manazones[i].x, manazones[i].y);
	//	graveyards[i].rect.setPosition(graveyards[i].x, graveyards[i].y);
	//	battlezones[i].rect.setPosition(battlezones[i].x, battlezones[i].y);
	//	shields[i].rect.setPosition(shields[i].x, shields[i].y);

	//	decks[i].rect.setFillColor(sf::Color(255, 255, 255));
	//	hands[i].rect.setFillColor(sf::Color(255, 255, 255));
	//	manazones[i].rect.setFillColor(sf::Color(255, 255, 255));
	//	graveyards[i].rect.setFillColor(sf::Color(255, 255, 255));
	//	battlezones[i].rect.setFillColor(sf::Color(255, 255, 255));
	//	shields[i].rect.setFillColor(sf::Color(255, 255, 255));

	//	decks[i].rect.setOutlineColor(DECKBORDERCOLOR);
	//	hands[i].rect.setOutlineColor(HANDBORDERCOLOR);
	//	manazones[i].rect.setOutlineColor(MANABORDERCOLOR);
	//	graveyards[i].rect.setOutlineColor(GRAVEYARDBORDERCOLOR);
	//	battlezones[i].rect.setOutlineColor(BATTLEBORDERCOLOR);
	//	shields[i].rect.setOutlineColor(SHIELDBORDERCOLOR);

	//	decks[i].rect.setOutlineThickness(ZONEBORDERSIZE);
	//	hands[i].rect.setOutlineThickness(ZONEBORDERSIZE);
	//	manazones[i].rect.setOutlineThickness(ZONEBORDERSIZE);
	//	graveyards[i].rect.setOutlineThickness(ZONEBORDERSIZE);
	//	battlezones[i].rect.setOutlineThickness(ZONEBORDERSIZE);
	//	shields[i].rect.setOutlineThickness(ZONEBORDERSIZE);
	}

	mAttackphase = PHASE_NONE;
	mAttacker = -1;
	mDefender = -1;
	mDefenderType = -1;
	mBreakCount = -1;
	
	mCastingCard = -1;
	mCastingCivilizations = 0;
	mCastingCost = -1;
	mCastingEvobait = -1;
	mCastingEvobait2 = -1;

	mIsChoiceActive = false;
	mChoice = NULL;
	mChoiceCard = -1;
	mChoicePlayer = -1;
	mChoiceResolverAnswersRemaining = -1;
	mSimulationChoiceFailed = false;
	mZeroPowerCheckPending = false;
	mRaceQueryDepth = 0;

	mWinner = -1;

	mCurrentMoveCount = 0;

	mRandomGen.Randomize();

	mIsSimulation = false;
}

Duel::~Duel()
{
	clearCards();
	if (mChoice != NULL)
		delete mChoice;
}

bool Duel::isCloneable() const
{
	if (mLuaCallbackSuspended.load() || mChoice != NULL || mIsChoiceActive ||
		!mChoiceValidCards.empty() || !mMsgMngr.messages.empty() ||
		mSimulationChoiceFailed || mZeroPowerCheckPending || mRaceQueryDepth != 0)
		return false;
	if (LuaCards == NULL || lua_gettop(LuaCards) != 0)
		return false;
	if (mNextUniqueId != static_cast<int>(mCardList.size()))
		return false;

	for (size_t index = 0; index < mCardList.size(); ++index)
	{
		Card* card = mCardList[index];
		if (card == NULL || card->mUniqueId != static_cast<int>(index))
			return false;
		for (std::vector<Modifier*>::const_iterator modifier = card->mModifiers.begin();
			modifier != card->mModifiers.end(); ++modifier)
		{
			if (*modifier == NULL || (*modifier)->mFuncRef == LUA_NOREF ||
				(*modifier)->mFuncRef == LUA_REFNIL)
				return false;
		}
	}
	return true;
}

bool Duel::copyFrom(const Duel& duel)
{
	if (this == &duel)
		return true;
	if (!duel.isCloneable())
		return false;

	std::vector<std::unique_ptr<Card> > ownedCards;
	ownedCards.reserve(duel.mCardList.size());
	std::unordered_map<const Card*, Card*> cardMap;
	for (std::vector<Card*>::const_iterator source = duel.mCardList.begin();
		source != duel.mCardList.end(); ++source)
	{
		std::unique_ptr<Card> clone(new Card());
		clone->copyStateFrom(**source);
		for (std::vector<Modifier*>::const_iterator modifier = (*source)->mModifiers.begin();
			modifier != (*source)->mModifiers.end(); ++modifier)
		{
			Modifier* clonedModifier = (*modifier)->clone();
			if (clonedModifier == NULL)
				return false;
			clone->mModifiers.push_back(clonedModifier);
		}
		cardMap[*source] = clone.get();
		ownedCards.push_back(std::move(clone));
	}

	for (size_t index = 0; index < duel.mCardList.size(); ++index)
	{
		if (!remapCardVector(duel.mCardList[index]->mEvoStack, cardMap,
			ownedCards[index]->mEvoStack))
			return false;
	}

	std::vector<Card*> deckCards[2];
	std::vector<Card*> handCards[2];
	std::vector<Card*> manaCards[2];
	std::vector<Card*> graveyardCards[2];
	std::vector<Card*> shieldCards[2];
	std::vector<Card*> battleCards[2];
	for (int player = 0; player < 2; ++player)
	{
		if (!remapCardVector(duel.mDecks[player].mCards, cardMap, deckCards[player]) ||
			!remapCardVector(duel.mHands[player].mCards, cardMap, handCards[player]) ||
			!remapCardVector(duel.mManazones[player].mCards, cardMap, manaCards[player]) ||
			!remapCardVector(duel.mGraveyards[player].mCards, cardMap, graveyardCards[player]) ||
			!remapCardVector(duel.mShields[player].mCards, cardMap, shieldCards[player]) ||
			!remapCardVector(duel.mBattlezones[player].mCards, cardMap, battleCards[player]))
			return false;
	}

	clearCards();
	if (mChoice != NULL)
	{
		delete mChoice;
		mChoice = NULL;
	}
	for (std::vector<std::unique_ptr<Card> >::iterator card = ownedCards.begin();
		card != ownedCards.end(); ++card)
		mCardList.push_back(card->release());

	mDeckNames[0] = duel.mDeckNames[0];
	mDeckNames[1] = duel.mDeckNames[1];
	// Execution mode belongs to the destination context, not to the game position.
	// In particular, restoring a live root must not turn a simulation duel live.
	mLuaCallbackSuspended.store(false);
	mSimulationChoiceFailed = false;
	mMessageHistory = duel.mMessageHistory;
	mMoveHistory = duel.mMoveHistory;
	mMovePlayers = duel.mMovePlayers;
	mCurrentMoveCount = duel.mCurrentMoveCount;
	mRandomGen = duel.mRandomGen;

	mAttacker = duel.mAttacker;
	mDefender = duel.mDefender;
	mDefenderType = duel.mDefenderType;
	mBreakCount = duel.mBreakCount;
	mShieldTargets = duel.mShieldTargets;
	mShieldBreakersThisTurn[0] = duel.mShieldBreakersThisTurn[0];
	mShieldBreakersThisTurn[1] = duel.mShieldBreakersThisTurn[1];
	mShieldsBrokenThisTurn[0] = duel.mShieldsBrokenThisTurn[0];
	mShieldsBrokenThisTurn[1] = duel.mShieldsBrokenThisTurn[1];
	mCardsDrawnThisTurn[0] = duel.mCardsDrawnThisTurn[0];
	mCardsDrawnThisTurn[1] = duel.mCardsDrawnThisTurn[1];
	mLuaRuleState = duel.mLuaRuleState;
	mAttackphase = duel.mAttackphase;

	mCastingCard = duel.mCastingCard;
	mCastingCivilizations = duel.mCastingCivilizations;
	mCastingCost = duel.mCastingCost;
	mCastingEvobait = duel.mCastingEvobait;
	mCastingEvobait2 = duel.mCastingEvobait2;
	mCastingManaCards = duel.mCastingManaCards;

	mChoiceCard = duel.mChoiceCard;
	mChoicePlayer = duel.mChoicePlayer;
	mIsChoiceActive = false;
	mChoiceValidCards.clear();
	mZeroPowerCheckPending = false;
	mRaceQueryDepth = 0;
	mWinner = duel.mWinner;
	mNextUniqueId = duel.mNextUniqueId;
	mMsgMngr = duel.mMsgMngr;
	mCurrentMessage = duel.mCurrentMessage;
	mTurn = duel.mTurn;
	mTurnPhase = duel.mTurnPhase;
	mManaUsed = duel.mManaUsed;
	mPlayerType[0] = duel.mPlayerType[0];
	mPlayerType[1] = duel.mPlayerType[1];

	for (int player = 0; player < 2; ++player)
	{
		mDecks[player].mOwner = duel.mDecks[player].mOwner;
		mDecks[player].mRandomGen = &mRandomGen;
		mDecks[player].mCards = deckCards[player];
		mHands[player].mOwner = duel.mHands[player].mOwner;
		mHands[player].mMyPlayer = duel.mHands[player].mMyPlayer;
		mHands[player].mCards = handCards[player];
		mManazones[player].mOwner = duel.mManazones[player].mOwner;
		mManazones[player].mCards = manaCards[player];
		mGraveyards[player].mOwner = duel.mGraveyards[player].mOwner;
		mGraveyards[player].mCards = graveyardCards[player];
		mShields[player].mOwner = duel.mShields[player].mOwner;
		mShields[player].mSlotsUsed = duel.mShields[player].mSlotsUsed;
		mShields[player].mCards = shieldCards[player];
		mBattlezones[player].mOwner = duel.mBattlezones[player].mOwner;
		mBattlezones[player].mCards = battleCards[player];
	}
	return true;
}

int Duel::handleMessage(Message& msg)
{
	MsgHistoryItem k;
	k.msg = msg;
	k.move = mCurrentMoveCount;
	mMessageHistory.push_back(k);
	if (msg.getType() == "cardmove")
	{
		int cid = msg.getInt("card");
		int tozone = msg.getInt("to");
		Card* c = mCardList.at(cid);
		int owner = c->mOwner;
		int fromzone = c->mZone;
		if (fromzone == ZONE_BATTLE && optionalMessageInt(msg, "separateevolution", 0) == 1 &&
			!c->mEvoStack.empty())
			mBattlezones[owner].seperateEvolution(c);
		getZone(owner, c->mZone)->removeCard(c);
		bool isEvolution = tozone == ZONE_BATTLE && getIsEvolution(cid) == 1;
		int evobait = optionalMessageInt(msg, "evobait", -1);
		int evobait2 = optionalMessageInt(msg, "evobait2", -1);
		if (isEvolution) //evolution creatures
		{
			if (evobait == -1)
			{
				getZone(owner, tozone)->addCard(c);
				mBattlezones[owner].addCard(c);
			}
			else if (evobait2 != -1)
			{
				mBattlezones[owner].vortexEvolveCard(c, evobait, evobait2);
			}
			else
			{
				mBattlezones[owner].evolveCard(c, evobait);
			}
		}
		else
		{
			if (tozone == ZONE_DECK && msg.getInt("tobottom") == 1)
				mDecks[owner].addCardToBottom(c);
			else
				getZone(owner, tozone)->addCard(c);
		}
		c->mZone = tozone;
		if (fromzone == ZONE_DECK && tozone == ZONE_HAND &&
			optionalMessageInt(msg, "draw", 0) == 1)
			mCardsDrawnThisTurn[owner]++;
		if (isEvolution && evobait != -1)
		{
			Message evolved("creatureevolve");
			evolved.addValue("evolution", cid);
			evolved.addValue("evobait", evobait);
			evolved.addValue("evobait2", evobait2);
			mMsgMngr.sendMessage(evolved);
		}
		if (tozone == ZONE_MANA &&
			(c->mCivilizations & (c->mCivilizations - 1)) != 0)
		{
			Message tap("cardtap");
			tap.addValue("card", cid);
			mMsgMngr.sendMessage(tap);
		}
		if (!mIsSimulation && fromzone != tozone && SoundMngr != NULL)
			SoundMngr->playSound(isEvolution ? SOUND_EVOLUTION : SOUND_CARD_MOVE);
		if (tozone != ZONE_BATTLE)
		{
			for (int i = c->mEvoStack.size()-1; i >= 0; i--) //move all cards in stack to the zone seperately
			{
				Message m("cardmove");
				m.addValue("card", c->mEvoStack.at(i)->mUniqueId);
				m.addValue("from", c->mEvoStack.at(i)->mZone);
				m.addValue("to", tozone);
				mMsgMngr.sendMessage(m);
				c->mEvoStack.pop_back();
			}
		}
		if (c->mZone == ZONE_BATTLE && c->mType == TYPE_SPELL)
		{
			c->callOnCast(); //cast the spell
			Message m("cardmove");
			m.addValue("card", cid);
			m.addValue("from", mCardList.at(cid)->mZone);
			m.addValue("to", ZONE_GRAVEYARD);
			mMsgMngr.sendMessage(m);
		}
	}
	else if (msg.getType() == "creaturedestroy")
	{
		Message m("cardmove");
		int cid = msg.getInt("creature");
		m.addValue("card", cid);
		m.addValue("from", mCardList.at(cid)->mZone);
		m.addValue("to", msg.getInt("zoneto"));
		mMsgMngr.sendMessage(m);
	}
	else if (msg.getType() == "carddiscard")
	{
		Message m("cardmove");
		int cid = msg.getInt("card");
		m.addValue("card", cid);
		m.addValue("from", mCardList.at(cid)->mZone);
		m.addValue("to", msg.getInt("zoneto"));
		mMsgMngr.sendMessage(m);
	}
	else if (msg.getType() == "manadestroy")
	{
		Message m("cardmove");
		int cid = msg.getInt("card");
		m.addValue("card", cid);
		m.addValue("from", mCardList.at(cid)->mZone);
		m.addValue("to", msg.getInt("zoneto"));
		mMsgMngr.sendMessage(m);
	}
	/*else if (msg.getType() == "carddiscard")
	{
		Message m("cardmove");
		m.addValue("card", msg.getInt("card"));
		m.addValue("to", ZONE_GRAVEYARD);
		MsgMngr.sendMessage(m);
	}*/
	/*else if (msg.getType() == "carddraw") //carddraw is replaced by cardmove with to=ZONE_HAND
	{
		int plyr = msg.getInt("player");
		Message m("cardmove");
		m.addValue("card", decks[plyr].getTopCard());
		m.addValue("to", ZONE_HAND);
		MsgMngr.sendMessage(m);
	}*/
	else if (msg.getType() == "cardplay")
	{
		int cid = msg.getInt("card");
		int eb = optionalMessageInt(msg, "evobait", -1);
		int eb2 = optionalMessageInt(msg, "evobait2", -1);
		
		Message m("cardmove");
		m.addValue("card", cid);
		m.addValue("from", mCardList.at(cid)->mZone);
		m.addValue("to", ZONE_BATTLE);
		m.addValue("evobait", eb);
		m.addValue("evobait2", eb2);
		mMsgMngr.sendMessage(m);
		//if (!isSimulation)
		//	SoundMngr->playSound(SOUND_PLAY);
		/*if (eb != -1)
		{
			Message msg4("creatureevolve");
			msg4.addValue("evolution", cid);
			msg4.addValue("evobait", eb);
			MsgMngr.sendMessage(msg4);
		}*/
	}
	else if (msg.getType() == "cardmana")
	{
		mManaUsed = 1;
		mTurnPhase = TURN_PHASE_MAIN;
		int cid = msg.getInt("card");
		Message m("cardmove");
		m.addValue("card", cid);
		m.addValue("from", mCardList.at(cid)->mZone);
		m.addValue("to", ZONE_MANA);
		mMsgMngr.sendMessage(m);
		//if (!isSimulation)
		//	SoundMngr->playSound(SOUND_PLAY);
	}
	else if (msg.getType() == "creatureattack")
	{
		/*int type = msg.getInt("defendertype");
		if (type == DEFENDER_CREATURE)
		{
			Message m("creaturebattle");
			m.addValue("attacker", msg.getInt("attacker"));
			m.addValue("defender", msg.getInt("defender"));
			MsgMngr.sendMessage(m);
		}
		else if (type == DEFENDER_PLAYER)
		{
			Message m("creaturebreakshield");
			m.addValue("attacker", msg.getInt("attacker"));
			m.addValue("defender", msg.getInt("defender"));
			MsgMngr.sendMessage(m);
		}*/
		mAttacker = msg.getInt("attacker");
		mDefender = msg.getInt("defender");
		mDefenderType = msg.getInt("defendertype");
		if (!mIsSimulation && SoundMngr != NULL)
		{
			int power = getCreaturePower(mAttacker);
			int sound = power >= 10000 ? SOUND_ATTACK_LARGE :
				(power >= 6000 ? SOUND_ATTACK_MEDIUM : SOUND_ATTACK_SMALL);
			SoundMngr->playSound(sound);
		}
		//cout << "attack " << attacker << " " << defender << " " << defendertype << endl;
		//attackphase = PHASE_BLOCK;
		Message m("changeattackphase");
		m.addValue("phase", PHASE_BLOCK);
		m.addValue("oldphase", mAttackphase);
		mMsgMngr.sendMessage(m);
	}
	else if (msg.getType() == "creatureblock")
	{
		Message m("creaturebattle");
		m.addValue("attacker", msg.getInt("attacker"));
		m.addValue("defender", msg.getInt("blocker"));
		m.addValue("blocked", 1);
		mMsgMngr.sendMessage(m);
	}
	else if (msg.getType() == "creaturebreakshield")
	{
		int creature = msg.getInt("creature");
		if (creature >= 0 && creature < static_cast<int>(mCardList.size()))
		{
			int owner = mCardList.at(creature)->mOwner;
			mShieldBreakersThisTurn[owner].insert(creature);
			mShieldsBrokenThisTurn[owner]++;
		}

		Message m("breakshield");
		m.addValue("player", msg.getInt("defender"));
		m.addValue("shield", msg.getInt("shield"));
		m.addValue("attacker", msg.getInt("attacker"));
		m.addValue("cantrigger", 1);
		mMsgMngr.sendMessage(m);
	}
	else if (msg.getType() == "breakshield")
	{
		Message m("cardmove");
		int cid = msg.getInt("shield");
		m.addValue("card", cid);
		m.addValue("from", mCardList.at(cid)->mZone);
		m.addValue("to", ZONE_HAND);
		mMsgMngr.sendMessage(m);
		/*if (msg.getInt("cantrigger") == 1)
		{
			attackphase = PHASE_TRIGGER;
			shieldtargets.push_back(shields[plyr].cards.at(shields[plyr].cards.size() - 1)->UniqueId);
		}*/
	}
	else if (msg.getType() == "creaturebattle")
	{
		battle(msg.getInt("attacker"), msg.getInt("defender"));
	}
	else if (msg.getType() == "creatureusetapability")
	{
		int cid = msg.getInt("creature");

		Message m("cardtap");
		m.addValue("card", cid);
		mMsgMngr.sendMessage(m);

		//mMsgMngr.sendMessage(msg);
	}
	//else if (msg.getType() == "creatureevolve")
	//{
	//	int eb = msg.getInt("evobait");
	//	Card* cid = CardList.at(eb);
	//	//getZone(cid->Owner, cid->Zone)->removeCard(cid);
	//	if (cid->Zone != ZONE_BATTLE)
	//	{
	//		cout << "WARNING: attempting to evolve on card that is not in battlezone" << endl;
	//	}
	//	//battlezones[cid->Owner].removeBait(cid);
	//	cid->Zone = ZONE_EVOLVED;
	//	CardList.at(msg.getInt("evolution"))->evostack.push_back(cid);
	//}
	else if (msg.getType() == "cardtap")
	{
		mCardList.at(msg.getInt("card"))->tap();
		//if (!isSimulation)
		//	SoundMngr->playSound(SOUND_TAP);
	}
	else if (msg.getType() == "carduntap")
	{
		mCardList.at(msg.getInt("card"))->untap();
		//if (!isSimulation)
		//	SoundMngr->playSound(SOUND_UNTAP);
	}
	else if (msg.getType() == "endturn")
	{
		mShieldBreakersThisTurn[0].clear();
		mShieldBreakersThisTurn[1].clear();
		mShieldsBrokenThisTurn[0] = 0;
		mShieldsBrokenThisTurn[1] = 0;
		std::map<std::string, std::string>::const_iterator extra = msg.map.find("extraturn");
		if (extra == msg.map.end() || std::atoi(extra->second.c_str()) != 1)
			mTurn = (mTurn + 1) % 2;
		mManaUsed = 0;
		Message m("startturn");
		m.addValue("player", mTurn);
		mMsgMngr.sendMessage(m);
		//if (!isSimulation)
		//	SoundMngr->playSound(SOUND_ENDTURN);
	}
	else if (msg.getType() == "startturn")
	{
		int plyr = msg.getInt("player");
		mCardsDrawnThisTurn[0] = 0;
		mCardsDrawnThisTurn[1] = 0;
		mTurnPhase = TURN_PHASE_MANA;
		std::vector<Card*>::iterator i;
		for (i = mBattlezones[plyr].mCards.begin(); i != mBattlezones[plyr].mCards.end(); i++) //untap creatures
		{
			Message m("carduntap");
			m.addValue("card", (*i)->mUniqueId);
			mMsgMngr.sendMessage(m);
			(*i)->mSummoningSickness = 0;
		}
		for (i = mManazones[plyr].mCards.begin(); i != mManazones[plyr].mCards.end(); i++) //untap mana
		{
			Message m("carduntap");
			m.addValue("card", (*i)->mUniqueId);
			mMsgMngr.sendMessage(m);
		}
		//Message m("carddraw"); //draw card
		//m.addValue("player", plyr);
		//MsgMngr.sendMessage(m);
		drawCards(plyr, 1);
		Message startDraw("startturndraw");
		startDraw.addValue("player", plyr);
		mMsgMngr.sendMessage(startDraw);
	}
	else if (msg.getType() == "modifiercreate")
	{
		int uid = msg.getInt("card");
		int ref = msg.getInt("funcref");
		Modifier* modifier = new Modifier(ref);
		const std::string statePrefix = "state.";
		for (std::map<std::string, std::string>::const_iterator value = msg.map.begin();
			value != msg.map.end(); ++value)
		{
			if (value->first.compare(0, statePrefix.size(), statePrefix) == 0)
				modifier->setLuaRuleState(value->first.substr(statePrefix.size()), std::atoi(value->second.c_str()));
		}
		mCardList.at(uid)->mModifiers.push_back(modifier);
	}
	else if (msg.getType() == "modifierdestroy")
	{
		int uid = msg.getInt("card");
		int mid = msg.getInt("modifier");
		if (uid < 0 || uid >= static_cast<int>(mCardList.size())) return 0;
		std::vector<Modifier*>& modifiers = mCardList[uid]->mModifiers;
		std::vector<Modifier*>::iterator target = modifiers.end();
		int ref = optionalMessageInt(msg, "funcref", LUA_NOREF);
		if (ref != LUA_NOREF)
		{
			for (std::vector<Modifier*>::iterator modifier = modifiers.begin();
				modifier != modifiers.end(); ++modifier)
			{
				if ((*modifier)->mFuncRef == ref)
				{
					target = modifier;
					break;
				}
			}
		}
		else if (mid >= 0 && mid < static_cast<int>(modifiers.size()))
			target = modifiers.begin() + mid;
		if (target == modifiers.end()) return 0;
		Modifier* modifier = *target;
		modifiers.erase(target);
		delete modifier;
	}
	else if (msg.getType() == "changeattackphase")
	{
		mAttackphase = msg.getInt("phase");
	}
	else if (msg.getType() == "resetattack")
	{
		resetAttack();
	}
	else if (msg.getType() == "deckshuffle")
	{
		mDecks[msg.getInt("player")].shuffle();
	}
	else if (msg.getType() == "carddiscardatrandom")
	{
		int plyr = msg.getInt("player");
		int count = msg.getInt("count");
		if (count < 1)
			count = 1;
		std::vector<Card*> candidates = mHands[plyr].mCards;
		while (count > 0 && !candidates.empty())
		{
			int selected = mRandomGen.Random(candidates.size());
			Message m("carddiscard");
			m.addValue("card", candidates.at(selected)->mUniqueId);
			m.addValue("zoneto", ZONE_GRAVEYARD);
			mMsgMngr.sendMessage(m);
			candidates.erase(candidates.begin() + selected);
			count--;
		}
	}
	else if (msg.getType() == "evolutionseperate")
	{
		int cid = msg.getInt("evolution");
		mBattlezones[mCardList.at(cid)->mOwner].seperateEvolution(mCardList.at(cid));
	}
	return 0;
}

void Duel::undoLastMove()
{
	if (mMessageHistory.size() == 0)
		return;
	for (int i = mMessageHistory.size() - 1; i >= 0; i--)
	{
		MsgHistoryItem m = mMessageHistory.at(i);
		if (m.move == mCurrentMoveCount)
		{
			mMessageHistory.pop_back();
			undoMessage(m.msg);
			//cout << "undoing " << m.msg.getType() << " for " << MoveHistory.at(MoveHistory.size() - 1).getType() << endl;
		}
		else
		{
			break;
		}
	}

	Message m = mMoveHistory.at(mMoveHistory.size() - 1);
	if (m.getType() == "cardplay")
	{
		resetCasting();
	}
	else if (m.getType() == "manatap")
	{

	}
	mMoveHistory.pop_back();
	mCurrentMoveCount--;
	rebuildShieldBreakersThisTurn();
}

void Duel::undoMessage(Message& msg)
{
	std::string type = msg.getType();
	if (type == "cardmove")
	{
		int cid = msg.getInt("card");
		int fromzone = msg.getInt("from");
		int tozone = msg.getInt("to");
		Card* c = mCardList.at(cid);
		int owner = c->mOwner;
		if (tozone == ZONE_BATTLE)
		{
			if (getIsEvolution(cid) == 1 && !c->mEvoStack.empty())
				mBattlezones[owner].seperateEvolution(c);
			mBattlezones[owner].removeCard(c);
		}
		else
		{
			getZone(owner, tozone)->removeCard(c);
		}
		getZone(owner, fromzone)->addCard(c);
		c->mZone = fromzone;
		mWinner = -1;
	}
	else if (msg.getType() == "cardtap")
	{
		mCardList.at(msg.getInt("card"))->untap();
	}
	else if (msg.getType() == "carduntap")
	{
		mCardList.at(msg.getInt("card"))->tap();
	}
	else if (msg.getType() == "endturn")
	{
		std::map<std::string, std::string>::const_iterator extra = msg.map.find("extraturn");
		if (extra == msg.map.end() || std::atoi(extra->second.c_str()) != 1)
			mTurn = (mTurn + 1) % 2;

		int flag = 0;
		for (std::vector<MsgHistoryItem>::reverse_iterator i = mMessageHistory.rbegin(); i != mMessageHistory.rend(); i++)
		{
			if ((*i).msg.getType() == "endturn")
			{
				mManaUsed = 0;
				flag = 1;
				break;
			}
			if ((*i).msg.getType() == "cardmana")
			{
				mManaUsed = 1;
				flag = 1;
				break;
			}
		}
		if (flag == 0)
		{
			mManaUsed = 0;
		}
	}
	else if (msg.getType() == "cardmana")
	{
		mManaUsed = 0;
		mTurnPhase = TURN_PHASE_MANA;
	}
	else if (msg.getType() == "changeattackphase")
	{
		mAttackphase = msg.getInt("oldphase");
	}
}

int Duel::getPlayerToMove()
{
	// The background search owns the shared Lua VM. PHASE_TARGET normally asks
	// card scripts who chooses the shields, so UI queries must use the decision
	// owner captured immediately before the search started.
	if (mAiThinking.load())
		return mAiThinkingPlayer.load();
	if (mIsChoiceActive)
		return mChoicePlayer;
	if (mAttackphase == PHASE_BLOCK || mAttackphase == PHASE_TRIGGER)
		return getOpponent(mTurn);
	if (mAttackphase == PHASE_TARGET)
		return getShieldChooser(mTurn, getOpponent(mTurn));
	return mTurn;
}

std::vector<Message> Duel::getPossibleMoves()
{
	std::vector<Message> moves(0);
	// Enumerating moves invokes many Lua-backed rule queries. Only simulation
	// copies may do that while the background worker owns LuaCards.
	if (mAiThinking.load())
		return moves;
	if (mLuaCallbackSuspended && !mIsChoiceActive)
		return moves;
	int player = getPlayerToMove();
	if (mIsChoiceActive && player == mChoicePlayer)
	{
		if (mChoice->mButtonCount > 0)
		{
			Message msg("choiceselect");
			msg.addValue("selection", RETURN_BUTTON1);
			moves.push_back(msg);
		}
		if (mChoice->mButtonCount > 1)
		{
			Message msg("choiceselect");
			msg.addValue("selection", RETURN_BUTTON2);
			moves.push_back(msg);
		}
		for (std::vector<int>::const_iterator i = mChoiceValidCards.begin(); i != mChoiceValidCards.end(); ++i)
		{
			Message msg("choiceselect");
			msg.addValue("selection", *i);
			moves.push_back(msg);
		}
	}
	else if (mAttackphase == PHASE_TRIGGER && player == getOpponent(mTurn)) //use shield triggers
	{
		for (std::vector<Card*>::iterator i = mHands[getOpponent(mTurn)].mCards.begin(); i != mHands[getOpponent(mTurn)].mCards.end(); i++)
		{
			for (std::vector<int>::iterator j = mShieldTargets.begin(); j != mShieldTargets.end(); j++)
			{
				if (*j == (*i)->mUniqueId)
				{
					if (getIsShieldTrigger(*j) && canUseShieldTrigger(*j) && getCardCanCast(*j))
					{
						Message msg("triggeruse");
						msg.addValue("trigger", *j);
						moves.push_back(msg);
					}
				}
			}
		}
		Message m("triggerskip");
		moves.push_back(m);
	}
	else if (mAttackphase == PHASE_TARGET) //target shields
	{
		for (std::vector<Card*>::iterator i = mShields[getOpponent(mTurn)].mCards.begin(); i != mShields[getOpponent(mTurn)].mCards.end(); i++)
		{
			Message m("targetshield");
			m.addValue("attacker", mAttacker);
			m.addValue("shield", (*i)->mUniqueId);
			moves.push_back(m);
		}
	}
	else if (mAttackphase == PHASE_BLOCK && player == getOpponent(mTurn)) //block
	{
		int forcedBlocker = getCreatureForcedBlocker(mAttacker);
		bool forcedBlockerAvailable = forcedBlocker >= 0
			&& forcedBlocker < static_cast<int>(mCardList.size())
			&& mCardList.at(forcedBlocker)->mZone == ZONE_BATTLE
			&& !mCardList.at(forcedBlocker)->mIsTapped
			&& getCreatureCanBlock(mAttacker, forcedBlocker);
		for (std::vector<Card*>::iterator i = mBattlezones[getOpponent(mTurn)].mCards.begin(); i != mBattlezones[getOpponent(mTurn)].mCards.end(); i++)
		{
			if (getCreatureCanBlock(mAttacker, (*i)->mUniqueId) && (*i)->mIsTapped == false
				&& ((*i)->mUniqueId != mDefender || mDefenderType == DEFENDER_PLAYER))
			{
				/*Message msg2("cardtap");
				msg2.addValue("card", (*i)->UniqueId);
				MsgMngr.sendMessage(msg2);*/

				Message msg("creatureblock");
				msg.addValue("attacker", mAttacker);
				msg.addValue("blocker", (*i)->mUniqueId);
				moves.push_back(msg);
			}
		}
		if (!forcedBlockerAvailable)
		{
			Message m("blockskip");
			moves.push_back(m);
		}
	}
	else if (mCastingCard != -1 && player == mTurn) //tap mana
	{
		for (std::vector<Card*>::iterator i = mManazones[mTurn].mCards.begin(); i != mManazones[mTurn].mCards.end(); i++)
		{
			if ((*i)->mIsTapped == false && canTapManaForCasting((*i)->mUniqueId))
			{
				Message m("manatap");
				m.addValue("card", (*i)->mUniqueId);
				moves.push_back(m);
			}
		}
	}

	// Choices, combat sub-phases, and mana payment are exclusive. Do not
	// append ordinary hand or attack actions until the pending step resolves.
	if (mIsChoiceActive || mAttackphase != PHASE_NONE || mCastingCard != -1)
		return moves;

	if (player != mTurn)
		return moves;

	if (mTurnPhase == TURN_PHASE_MANA)
	{
		if (mManaUsed == 0)
		{
			for (std::vector<Card*>::iterator card = mHands[mTurn].mCards.begin(); card != mHands[mTurn].mCards.end(); card++)
			{
				Message mana("cardmana");
				mana.addValue("card", (*card)->mUniqueId);
				moves.push_back(mana);
			}
		}
	}

	if (mTurnPhase <= TURN_PHASE_MAIN)
	{
		for (std::vector<Card*>::iterator card = mHands[mTurn].mCards.begin(); card != mHands[mTurn].mCards.end(); card++)
		{
			if (getCardCost((*card)->mUniqueId) <= mManazones[mTurn].getUntappedMana() &&
				canPayForCard(mTurn, (*card)->mUniqueId) &&
				getCardCanCast((*card)->mUniqueId) == 1)
			{
				if (getIsEvolution((*card)->mUniqueId) == 1)
				{
					if (getEvolutionBaitCount((*card)->mUniqueId) == 2)
					{
						for (size_t first = 0; first < mBattlezones[mTurn].mCards.size(); ++first)
						{
							for (size_t second = first + 1; second < mBattlezones[mTurn].mCards.size(); ++second)
							{
								int bait = mBattlezones[mTurn].mCards[first]->mUniqueId;
								int bait2 = mBattlezones[mTurn].mCards[second]->mUniqueId;
								if (getCreatureCanVortexEvolve((*card)->mUniqueId, bait, bait2) == 1)
								{
									Message play("cardplay");
									play.addValue("card", (*card)->mUniqueId);
									play.addValue("evobait", bait);
									play.addValue("evobait2", bait2);
									moves.push_back(play);
								}
							}
						}
					}
					else
					{
						for (std::vector<Card*>::iterator bait = mBattlezones[mTurn].mCards.begin(); bait != mBattlezones[mTurn].mCards.end(); bait++)
						{
							if (getCreatureCanEvolve((*card)->mUniqueId, (*bait)->mUniqueId) == 1)
							{
								Message play("cardplay");
								play.addValue("card", (*card)->mUniqueId);
								play.addValue("evobait", (*bait)->mUniqueId);
								play.addValue("evobait2", -1);
								moves.push_back(play);
							}
						}
					}
				}
				else
				{
					Message play("cardplay");
					play.addValue("card", (*card)->mUniqueId);
					play.addValue("evobait", -1);
					play.addValue("evobait2", -1);
					moves.push_back(play);
				}
			}
		}
		for (std::vector<Card*>::iterator creature = mBattlezones[mTurn].mCards.begin(); creature != mBattlezones[mTurn].mCards.end(); creature++)
		{
			if (!(*creature)->mIsTapped &&
				((*creature)->mSummoningSickness == 0 || getIsSpeedAttacker((*creature)->mUniqueId) == 1) &&
				getCreatureHasTapAbility((*creature)->mUniqueId) == 1)
			{
				Message tapAbility("creatureusetapability");
				tapAbility.addValue("creature", (*creature)->mUniqueId);
				moves.push_back(tapAbility);
			}
		}
	}

	if (canEndTurn())
	{
		Message end("endturn");
		end.addValue("player", mTurn);
		moves.push_back(end);
	}
	for (std::vector<Card*>::iterator i = mBattlezones[mTurn].mCards.begin(); i != mBattlezones[mTurn].mCards.end(); i++)
	{
		bool canAttackThisTurn = mCardList.at((*i)->mUniqueId)->mSummoningSickness == 0 ||
			getIsSpeedAttacker((*i)->mUniqueId) == 1;
		int canattack = getCreatureCanAttackPlayers((*i)->mUniqueId);
		if ((canattack == CANATTACK_ALWAYS ||
			(canAttackThisTurn && (canattack == CANATTACK_TAPPED || canattack == CANATTACK_UNTAPPED)))
			&& mCardList.at((*i)->mUniqueId)->mIsTapped == false)
		{
			Message msg("creatureattack");
			msg.addValue("attacker", (*i)->mUniqueId);
			msg.addValue("defender", getOpponent(mTurn));
			msg.addValue("defendertype", DEFENDER_PLAYER);
			moves.push_back(msg);
		}
		for (std::vector<Card*>::iterator j = mBattlezones[getOpponent(mTurn)].mCards.begin(); j != mBattlezones[getOpponent(mTurn)].mCards.end(); j++)
		{
			int canattack = getCreatureCanAttackCreature((*i)->mUniqueId, (*j)->mUniqueId);
			if (((*j)->mIsTapped == true || canattack == CANATTACK_UNTAPPED)
				&& canattack <= CANATTACK_UNTAPPED
				&& canAttackThisTurn
				&& mCardList.at((*i)->mUniqueId)->mIsTapped == false)
			{
				Message msg("creatureattack");
				msg.addValue("attacker", (*i)->mUniqueId);
				msg.addValue("defender", (*j)->mUniqueId);
				msg.addValue("defendertype", DEFENDER_CREATURE);
				moves.push_back(msg);
			}
		}
	}

	return moves;
}

int Duel::handleInterfaceInput(Message& msg)
{
	mCurrentMoveCount++;
	mMoveHistory.push_back(msg);
	mMovePlayers.push_back(getPlayerToMove());
	std::string type = msg.getType();
	if (type == "cardplay")
	{
		int whichCard = msg.getInt("card");
		int manacost = getCardCost(whichCard);
		if (mTurnPhase <= TURN_PHASE_MAIN && mManazones[mTurn].getUntappedMana() >= manacost &&
			canPayForCard(mTurn, whichCard) && getCardCanCast(whichCard)==1) //has appropriate mana
		{
			int eb = optionalMessageInt(msg, "evobait", -1);
			int eb2 = optionalMessageInt(msg, "evobait2", -1);
			int e = getIsEvolution(whichCard);
			int baitCount = e == 1 ? getEvolutionBaitCount(whichCard) : 0;
			bool legalEvolution = e == 1 &&
				((baitCount == 2 &&
					getCreatureCanVortexEvolve(whichCard, eb, eb2) == 1) ||
				 (baitCount != 2 &&
					getCreatureCanEvolve(whichCard, eb) == 1));
			if (legalEvolution || e == 0)
			{
				mTurnPhase = TURN_PHASE_MAIN;
				mCastingCard = whichCard;
				mCastingCivilizations = getCardCivilizations(mCastingCard);
				mCastingCost = getCardCost(mCastingCard);
				mCastingManaCards.clear();
				mCastingEvobait = eb;
				mCastingEvobait2 = eb2;
			}
			//MsgMngr.sendMessage(msg);
		}
	}
	else if (type == "cardmana")
	{
		if (mTurnPhase == TURN_PHASE_MANA && mManaUsed == 0)
		{
			mManaUsed = 1;
			mTurnPhase = TURN_PHASE_MAIN;
			mMsgMngr.sendMessage(msg);
		}
	}
	else if (type == "endturn")
	{
		if (mAttackphase == PHASE_NONE && !mIsChoiceActive && mCastingCard == -1 && canEndTurn())
		{
			nextTurn();
		}
	}
	else if (type == "creatureattack")
	{
		int attck = msg.getInt("attacker");
		int defen = msg.getInt("defender");
		bool canAttackThisTurn = mCardList.at(attck)->mSummoningSickness == 0 ||
			getIsSpeedAttacker(attck) == 1;
		if (msg.getInt("defendertype") == DEFENDER_PLAYER)
		{
			int canattack = getCreatureCanAttackPlayers(attck);
			if ((canattack == CANATTACK_ALWAYS || 
				(canAttackThisTurn && (canattack == CANATTACK_TAPPED || canattack == CANATTACK_UNTAPPED)))
				&& mCardList.at(attck)->mIsTapped == false)
			{
				mTurnPhase = TURN_PHASE_ATTACK;
				Message msg2("cardtap");
				msg2.addValue("card", msg.getInt("attacker"));
				mMsgMngr.sendMessage(msg2);
				mMsgMngr.sendMessage(msg);
			}
		}
		else if (msg.getInt("defendertype") == DEFENDER_CREATURE)
		{
			int canattack = getCreatureCanAttackCreature(attck, defen);
			if ((mCardList.at(defen)->mIsTapped == true || canattack == CANATTACK_UNTAPPED)
				&& canattack <= CANATTACK_UNTAPPED
				&& mCardList.at(attck)->mIsTapped == false
				&& canAttackThisTurn)
			{
				mTurnPhase = TURN_PHASE_ATTACK;
				Message msg2("cardtap");
				msg2.addValue("card", msg.getInt("attacker"));
				mMsgMngr.sendMessage(msg2);
				mMsgMngr.sendMessage(msg);
			}
		}
	}
	else if (type == "creatureblock")
	{
		if (mAttackphase == PHASE_BLOCK)
		{
			int blocker = msg.getInt("blocker");
			if (getCreatureCanBlock(mAttacker, blocker) && mCardList.at(blocker)->mIsTapped == false
				&& (blocker != mDefender || mDefenderType == DEFENDER_PLAYER))
			{
				Message msg2("cardtap");
				msg2.addValue("card", blocker);
				mMsgMngr.sendMessage(msg2);
					Message msg("creaturebattle");
					msg.addValue("attacker", mAttacker);
					msg.addValue("defender", blocker);
					msg.addValue("blocked", 1);
					mMsgMngr.sendMessage(msg);
				//resetAttack();
				Message msg3("resetattack");
				mMsgMngr.sendMessage(msg3);
			}
		}
	}
	else if (type == "blockskip")
	{
		if (mAttackphase == PHASE_BLOCK)
		{
			int forcedBlocker = getCreatureForcedBlocker(mAttacker);
			if (forcedBlocker >= 0 && forcedBlocker < static_cast<int>(mCardList.size())
				&& mCardList.at(forcedBlocker)->mZone == ZONE_BATTLE
				&& !mCardList.at(forcedBlocker)->mIsTapped
				&& getCreatureCanBlock(mAttacker, forcedBlocker))
				return 0;
			Message unblocked("creatureunblocked");
			unblocked.addValue("attacker", mAttacker);
			unblocked.addValue("defender", mDefender);
			unblocked.addValue("defendertype", mDefenderType);
			mMsgMngr.sendMessage(unblocked);
			//printf("mDefenderType: %d\n", mDefenderType);
			if (mDefenderType == DEFENDER_CREATURE)
			{
				Message m("creaturebattle");
				m.addValue("attacker", mAttacker);
				m.addValue("defender", mDefender);
				mMsgMngr.sendMessage(m);
				//resetAttack();
				Message msg3("resetattack");
				mMsgMngr.sendMessage(msg3);
			}
			else if (mDefenderType == DEFENDER_PLAYER)
			{
				//printf("defender: %d\n", mDefender);
				if (mShields[mDefender].mCards.size() == 0)
				{
					mWinner = getOpponent(mDefender);
					if (!mIsSimulation) printf("Winner: %d\n", mWinner);
				}
				else
				{
					//attackphase = PHASE_TARGET;
					//printf("changing phase\n");
					Message m("changeattackphase");
					m.addValue("phase", PHASE_TARGET);
					m.addValue("oldphase", mAttackphase);
					mMsgMngr.sendMessage(m);
					mBreakCount = getCreatureBreaker(mAttacker);
				}
			}
		}
	}
	else if (type == "targetshield")
	{
		if (mAttackphase == PHASE_TARGET)
		{
			int shield = msg.getInt("shield");
			mShieldTargets.push_back(shield);

			if (mShieldTargets.size() >= mBreakCount || mShields[mCardList.at(shield)->mOwner].mCards.size() <= 1)
			{
				Message m("changeattackphase");
				m.addValue("phase", PHASE_TRIGGER);
				m.addValue("oldphase", mAttackphase);
				mMsgMngr.sendMessage(m);
			}

			Message m("creaturebreakshield");
			m.addValue("creature", mAttacker);
			m.addValue("attacker", mAttacker);
			m.addValue("defender", mDefender);
			m.addValue("shield", shield);
			mMsgMngr.sendMessage(m);
		}
	}
	else if (type == "triggeruse")
	{
		if (mAttackphase == PHASE_TRIGGER)
		{
			for (std::vector<int>::iterator j = mShieldTargets.begin(); j != mShieldTargets.end(); j++)
			{
				int trigger = msg.getInt("trigger");
				if (*j == trigger)
				{
					if (getIsShieldTrigger(trigger) && canUseShieldTrigger(trigger) && getCardCanCast(trigger))
					{
						Message used("shieldtriggerused");
						used.addValue("trigger", trigger);
						mMsgMngr.sendMessage(used);
						Message m("cardplay");
						m.addValue("card", trigger);
						m.addValue("evobait", -1);
						m.addValue("evobait2", -1);
						mMsgMngr.sendMessage(m);
					}
				}
			}
		}
	}
	else if (type == "triggerskip")
	{
		if (mAttackphase == PHASE_TRIGGER)
		{
			//resetAttack();
			Message msg3("resetattack");
			mMsgMngr.sendMessage(msg3);
		}
	}
	else if (type == "manatap")
	{
		int card = msg.getInt("card");
		if (mCastingCard != -1 && canTapManaForCasting(card))
		{
			Message msg2("cardtap");
			msg2.addValue("card", card);
			mMsgMngr.sendMessage(msg2);
			mCastingManaCards.push_back(card);
			mCastingCost--;
			if (mCastingCost == 0)
			{
				Message msg3("cardplay");
				msg3.addValue("card", mCastingCard);
				msg3.addValue("evobait", mCastingEvobait);
				msg3.addValue("evobait2", mCastingEvobait2);
				mMsgMngr.sendMessage(msg3);
				resetCasting();
			}
		}
	}
	else if (type == "choiceselect")
	{
		int sid = msg.getInt("selection");
		selectChoice(sid, true);
	}
	else if (type == "creatureusetapability")
	{
		int cid = msg.getInt("creature");
		if (cid >= 0 && cid < static_cast<int>(mCardList.size()) &&
			mTurnPhase <= TURN_PHASE_MAIN &&
			mAttackphase == PHASE_NONE && !mIsChoiceActive && mCastingCard == -1 &&
			getPlayerToMove() == mTurn && mCardList.at(cid)->mOwner == mTurn &&
			mCardList.at(cid)->mZone == ZONE_BATTLE && !mCardList.at(cid)->mIsTapped &&
			(mCardList.at(cid)->mSummoningSickness == 0 || getIsSpeedAttacker(cid) == 1) &&
			getCreatureHasTapAbility(cid) == 1)
		{
			mTurnPhase = TURN_PHASE_MAIN;
			mMsgMngr.sendMessage(msg);
		}
	}
	/*else if (type == "choicebutton1")
	{
		if (choice.buttoncount >= 1 && isChoiceActive)
		{
			int chcard = choiceCard;
			resetChoice();
			choice.callbutton1(chcard);
		}
	}
	else if (type == "choicebutton2")
	{
		if (choice.buttoncount >= 2 && isChoiceActive)
		{
			int chcard = choiceCard;
			resetChoice();
			choice.callbutton2(chcard);
		}
	}*/
	return 0;
}

void Duel::loopInput()
{
	while (mInputLoopRunning)
	{
		if (!mAiThinking.load())
		{
			std::lock_guard<std::mutex> lock(gMutex);
			if (!mAiThinking.load()) dispatchAllMessages();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Duel::stopInputLoop()
{
	mInputLoopRunning = false;
}

int Duel::waitForChoice()
{
	gMutex.unlock();
	int choice = -1;
	while (mInputLoopRunning)
	{
		gMutex.lock();
		if (mMsgMngr.hasMoreMessages())
		{
			Message msg = mMsgMngr.peekMessage();
			mMsgMngr.dispatch();
			dispatchMessage(msg);
			if (msg.getType() == "choiceselect")
			{
				choice = msg.getInt("selection");
				break;
			}
		}
		gMutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (!mInputLoopRunning)
		gMutex.lock();
	//gMutex.lock();
	//printf("break\n");
	return mInputLoopRunning ? choice : RETURN_QUIT;
}

int Duel::resolveChoice()
{
	if (!mIsChoiceActive || mChoice == NULL)
		return RETURN_NOVALID;

	if (mChoiceResolver)
	{
		int choice = mChoiceResolver(*this);
		if (choice != RETURN_NOTHING && selectChoice(choice, false))
		{
			if (mChoiceResolverAnswersRemaining > 0 &&
				--mChoiceResolverAnswersRemaining == 0)
				clearChoiceResolver();
			return choice;
		}
		if (mIsSimulation)
		{
			mSimulationChoiceFailed = true;
			cancelChoice();
			return RETURN_QUIT;
		}
		clearChoiceResolver();
	}

	if (!mIsSimulation)
	{
		mLuaCallbackSuspended = true;
		int choice = waitForChoice();
		mLuaCallbackSuspended = false;
		return choice;
	}

	mSimulationChoiceFailed = true;
	cancelChoice();
	return RETURN_QUIT;
}

void Duel::setChoiceResolver(const ChoiceResolver& resolver, int answersRemaining)
{
	mChoiceResolver = resolver;
	mChoiceResolverAnswersRemaining = answersRemaining;
}

void Duel::clearChoiceResolver()
{
	mChoiceResolver = ChoiceResolver();
	mChoiceResolverAnswersRemaining = -1;
}

bool Duel::hasSimulationChoiceFailure() const
{
	return mSimulationChoiceFailed;
}

void Duel::clearSimulationChoiceFailure()
{
	mSimulationChoiceFailed = false;
}

//void Duel::parseMessages(unsigned int deltatime)
//{
//	if (!isChoiceActive)
//	{
//		bool worldchanged = dispatchAllMessages();
//		if (worldchanged)
//		{
//			for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); i++)
//			{
//				if ((*i)->Zone == ZONE_BATTLE)
//				{
//					(*i)->updatePower(getCreaturePower((*i)->UniqueId));
//				}
//			}
//		}
//	}
//}

bool Duel::dispatchAllMessages()
{
	bool worldchanged = false;
	while (mMsgMngr.hasMoreMessages() || mZeroPowerCheckPending)
	{
		if (!mMsgMngr.hasMoreMessages())
		{
			mZeroPowerCheckPending = false;
			probeBattleZonePower();
			continue;
		}
		Message msg = mMsgMngr.peekMessage();
		mMsgMngr.dispatch();
		dispatchMessage(msg);
		worldchanged = true;
	}
	return worldchanged;
}

void Duel::dispatchMessage(Message& msg)
{
	std::string type = msg.getString("msgtype");
	std::vector<Card*>::iterator i;

	mCurrentMessage = msg;
	if (type == "cardmove" && mCurrentMessage.map.find("from") == mCurrentMessage.map.end())
	{
		int card = optionalMessageInt(mCurrentMessage, "card", -1);
		if (card >= 0 && card < static_cast<int>(mCardList.size()))
			mCurrentMessage.addValue("from", mCardList.at(card)->mZone);
	}
	if (type == "creaturebattle" && mCurrentMessage.map.find("blocked") == mCurrentMessage.map.end())
		mCurrentMessage.addValue("blocked", 0);

	mCurrentMessage.addValue("msgtype", "mod " + type);
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(msg);
	}

	//std::cout << "  mod\n";
	if (mCurrentMessage.getInt("msgContinue") == 0) //do we continue?
		return;

	mCurrentMessage.addValue("msgtype", "pre " + type);
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(msg);
	}

	//std::cout << "  pre\n";
	if (mCurrentMessage.getInt("msgContinue") == 0)
		return;

	mCurrentMessage.addValue("msgtype", type);
	handleMessage(mCurrentMessage);
	//std::cout << "  in\n";

	mCurrentMessage.addValue("msgtype", "post " + type);
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(msg);
	}

	//std::cout << "  post\n";
	bool shouldCheckPower = type == "startturn" || type == "endturn" ||
		type == "creatureattack" || type == "creaturebattle" || type == "resetattack" ||
		type == "creatureusetapability";
	if (type == "cardmove" && mCurrentMessage.getInt("to") == ZONE_BATTLE)
	{
		int cid = mCurrentMessage.getInt("card");
		shouldCheckPower = cid >= 0 && cid < static_cast<int>(mCardList.size()) &&
			(mCardList.at(cid)->mType == TYPE_CREATURE || mCardList.at(cid)->mType == TYPE_SPELL);
	}
	if (shouldCheckPower)
		scheduleZeroPowerCheck();
}

void Duel::scheduleZeroPowerCheck()
{
	if (mZeroPowerCheckPending)
		return;
	mZeroPowerCheckPending = true;
}

void Duel::probeBattleZonePower()
{
	std::vector<int> creatures;
	creatures.reserve(mBattlezones[0].mCards.size() + mBattlezones[1].mCards.size());
	for (int player = 0; player < 2; player++)
	{
		for (std::vector<Card*>::iterator card = mBattlezones[player].mCards.begin();
			card != mBattlezones[player].mCards.end(); card++)
		{
			if ((*card)->mType == TYPE_CREATURE)
				creatures.push_back((*card)->mUniqueId);
		}
	}
	for (std::vector<int>::const_iterator creature = creatures.begin(); creature != creatures.end(); creature++)
	{
		int uid = *creature;
		if (uid >= 0 && uid < static_cast<int>(mCardList.size()) &&
			mCardList.at(uid)->mZone == ZONE_BATTLE && getCreaturePower(uid) == 0)
		{
			Message destroy("creaturedestroy");
			destroy.addValue("creature", uid);
			destroy.addValue("zoneto", ZONE_GRAVEYARD);
			mMsgMngr.sendMessage(destroy);
		}
	}
}

void Duel::addChoice(std::string info, int skip, int card, int player, int validref, int actionref,
	int aiPreferredSelection)
{
	cancelChoice();
	mChoice = new Choice(info, skip, validref, actionref, aiPreferredSelection);
	mChoiceCard = card;
	mChoicePlayer = player;
	mIsChoiceActive = true;
	//cout << "choice set: " << CardList.at(choiceCard)->Name << ": " << info << endl;
}

int Duel::choiceCanBeSelected(int sid) const
{
	return std::find(mChoiceValidCards.begin(), mChoiceValidCards.end(), sid) != mChoiceValidCards.end();
}

bool Duel::selectChoice(int sid, bool sendMessage)
{
	bool legalButton = mChoice != NULL &&
		((sid == RETURN_BUTTON1 && mChoice->mButtonCount >= 1) ||
		 (sid == RETURN_BUTTON2 && mChoice->mButtonCount >= 2));
	if (mChoice == NULL || !((sid >= 0 && choiceCanBeSelected(sid) == 1) || legalButton))
		return false;

	Choice* completedChoice = mChoice;
	mChoice = NULL;
	resetChoice();
	delete completedChoice;
	if (sendMessage)
	{
		Message selected("choiceselect");
		selected.addValue("selection", sid);
		mMsgMngr.sendMessage(selected);
	}
	return true;
}

void Duel::cancelChoice()
{
	if (mChoice != NULL)
	{
		delete mChoice;
		mChoice = NULL;
	}
	resetChoice();
}

void Duel::checkChoiceValid()
{
	mChoiceValidCards.clear();
	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); i++)
	{
		if ((*i)->mZone != ZONE_EVOLVED && mChoice->callvalid(mChoiceCard, (*i)->mUniqueId) == 1)
		{
			if ((*i)->mZone != ZONE_BATTLE || (*i)->mType != TYPE_CREATURE
				|| getCreatureCanBeChosen((*i)->mUniqueId, mChoicePlayer, mChoiceCard) == 1)
				mChoiceValidCards.push_back((*i)->mUniqueId);
		}
	}
	if (mChoiceValidCards.empty()) //no valid targets
	{
		Choice* targetlessChoice = mChoice;
		mChoice = NULL;
		resetChoice();
		delete targetlessChoice;
	}
	else
	{
		int shieldOwner = -1;
		bool onlyOnePlayersShields = true;
		for (std::vector<int>::const_iterator card = mChoiceValidCards.begin();
			card != mChoiceValidCards.end(); ++card)
		{
			if (mCardList.at(*card)->mZone != ZONE_SHIELD)
			{
				onlyOnePlayersShields = false;
				break;
			}
			if (shieldOwner == -1)
				shieldOwner = mCardList.at(*card)->mOwner;
			else if (shieldOwner != mCardList.at(*card)->mOwner)
			{
				onlyOnePlayersShields = false;
				break;
			}
		}
		if (onlyOnePlayersShields && shieldOwner >= 0)
			mChoicePlayer = getShieldChooser(mChoicePlayer, shieldOwner);
	}
}

void Duel::battle(int att, int def)
{
	Card* a = mCardList.at(att);
	Card* d = mCardList.at(def);
	int p1 = getCreaturePower(a->mUniqueId);
	int p2 = getCreaturePower(d->mUniqueId);
	if (p1 >= p2)
	{
		Message msg("creaturedestroy");
		msg.addValue("creature", def);
		msg.addValue("zoneto", ZONE_GRAVEYARD);
		mMsgMngr.sendMessage(msg);
	}
	if (p2 >= p1)
	{
		Message msg("creaturedestroy");
		msg.addValue("creature", att);
		msg.addValue("zoneto", ZONE_GRAVEYARD);
		mMsgMngr.sendMessage(msg);
	}
}

int Duel::getCreaturePower(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturepower");
	mCurrentMessage.addValue("power", mCardList.at(uid)->mPower);
	mCurrentMessage.addValue("creature", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("power");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCreatureBreaker(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturebreaker");
	mCurrentMessage.addValue("breaker", mCardList.at(uid)->mBreaker);
	mCurrentMessage.addValue("creature", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("breaker");
	mCurrentMessage = oldmsg;
	return c;
}

//int Duel::getCreatureCanAttack(int uid)
//{
//	Message oldmsg = currentMessage;
//	currentMessage = Message("get creaturecanattack");
//	currentMessage.addValue("canattack", 1);
//	currentMessage.addValue("creature", uid);
//
//	vector<Card*>::iterator i;
//	for (i = CardList.begin(); i != CardList.end(); i++)
//	{
//		(*i)->handleMessage(currentMessage);
//	}
//	int c = currentMessage.getInt("canattack");
//	currentMessage = oldmsg;
//	return c;
//}

//int Duel::getCreatureCanBeAttacked(int attckr, int dfndr)
//{
//	Message oldmsg = currentMessage;
//	currentMessage = Message("get creaturecanbeattacked");
//	currentMessage.addValue("canbeattacked", 1);
//	currentMessage.addValue("attacker", attckr);
//	currentMessage.addValue("defender", dfndr);
//
//	vector<Card*>::iterator i;
//	for (i = CardList.begin(); i != CardList.end(); i++)
//	{
//		(*i)->handleMessage(currentMessage);
//	}
//	int c = currentMessage.getInt("canbeattacked");
//	currentMessage = oldmsg;
//	return c;
//}

int Duel::getCreatureIsBlocker(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creatureisblocker");
	mCurrentMessage.addValue("isblocker", mCardList.at(uid)->mIsBlocker);
	mCurrentMessage.addValue("creature", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("isblocker");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCreatureCanBlock(int attckr,int blckr)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanblock");
	mCurrentMessage.addValue("canblock", getCreatureIsBlocker(blckr));
	mCurrentMessage.addValue("blocker", blckr);
	mCurrentMessage.addValue("attacker", attckr);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("canblock");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCreatureCanBlockRepeatedly(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanblockrepeatedly");
	mCurrentMessage.addValue("canblockrepeatedly", 0);
	mCurrentMessage.addValue("creature", uid);
	for (std::vector<Card*>::iterator card = mCardList.begin(); card != mCardList.end(); ++card)
		(*card)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("canblockrepeatedly");
	mCurrentMessage = oldmsg;
	return result;
}

//int Duel::getCreatureCanBeBlocked(int uid)
//{
//	Message oldmsg = currentMessage;
//	currentMessage = Message("get creaturecanbeblocked");
//	currentMessage.addValue("canbeblocked", 1);
//	currentMessage.addValue("creature", uid);
//
//	vector<Card*>::iterator i;
//	for (i = CardList.begin(); i != CardList.end(); i++)
//	{
//		(*i)->handleMessage(currentMessage);
//	}
//	int c = currentMessage.getInt("canbeblocked");
//	currentMessage = oldmsg;
//	return c;
//}

int Duel::getCreatureCanAttackPlayers(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanattackplayers");
	mCurrentMessage.addValue("canattack", CANATTACK_TAPPED);
	mCurrentMessage.addValue("attacker", uid);

	int big = CANATTACK_TAPPED;

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
		if (mCurrentMessage.getInt("canattack") > big)
		{
			big = mCurrentMessage.getInt("canattack");
		}
		else
		{
			mCurrentMessage.addValue("canattack", big);
		}
	}
	int c = mCurrentMessage.getInt("canattack");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCreatureCanAttackCreature(int attckr, int dfndr)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanattackcreature");
	mCurrentMessage.addValue("canattack", CANATTACK_TAPPED);
	mCurrentMessage.addValue("attacker", attckr);
	mCurrentMessage.addValue("defender", dfndr);

	int big = CANATTACK_TAPPED;

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
		if (mCurrentMessage.getInt("canattack") > big)
		{
			big = mCurrentMessage.getInt("canattack");
		}
		else
		{
			mCurrentMessage.addValue("canattack", big);
		}
	}
	int c = mCurrentMessage.getInt("canattack");
	mCurrentMessage = oldmsg;
	return c;
}

//int Duel::getCreatureCanAttackCreatures(int uid)
//{
//	Message oldmsg = currentMessage;
//	currentMessage = Message("get creaturecanattackcreatures");
//	currentMessage.addValue("canattack", 1);
//	currentMessage.addValue("creature", uid);
//
//	vector<Card*>::iterator i;
//	for (i = CardList.begin(); i != CardList.end(); i++)
//	{
//		(*i)->handleMessage(currentMessage);
//	}
//	int c = currentMessage.getInt("canattack");
//	currentMessage = oldmsg;
//	return c;
//}

//int Duel::getCreatureCanAttackTarget(int attckr, int dfndr)
//{
//	Message oldmsg = currentMessage;
//	currentMessage = Message("get creaturecanattacktarget");
//	currentMessage.addValue("canattack", 1);
//	currentMessage.addValue("attacker", attckr);
//	currentMessage.addValue("defender", attckr);
//
//	vector<Card*>::iterator i;
//	for (i = CardList.begin(); i != CardList.end(); i++)
//	{
//		(*i)->handleMessage(currentMessage);
//	}
//	int c = currentMessage.getInt("canattack");
//	currentMessage = oldmsg;
//	return c;
//}

//int Duel::getCreatureCanAttackUntappedCreatures(int uid)
//{
//	Message oldmsg = currentMessage;
//	currentMessage = Message("get creaturecanattackuntappedcreatures");
//	currentMessage.addValue("canattackuntappedcreatures", 0);
//	currentMessage.addValue("creature", uid);
//
//	vector<Card*>::iterator i;
//	for (i = CardList.begin(); i != CardList.end(); i++)
//	{
//		(*i)->handleMessage(currentMessage);
//	}
//	int c = currentMessage.getInt("canattackuntappedcreatures");
//	currentMessage = oldmsg;
//	return c;
//}

int Duel::getCardCost(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get cardcost");
	mCurrentMessage.addValue("cost", mCardList.at(uid)->mManaCost);
	mCurrentMessage.addValue("card", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("cost");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getIsShieldTrigger(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get cardshieldtrigger");
	mCurrentMessage.addValue("shieldtrigger", mCardList.at(uid)->mIsShieldTrigger);
	mCurrentMessage.addValue("card", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("shieldtrigger");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::canUseShieldTrigger(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get canuseshieldtrigger");
	mCurrentMessage.addValue("canuse", 1);
	mCurrentMessage.addValue("card", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("canuse");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getIsEvolution(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creatureisevolution");
	mCurrentMessage.addValue("isevolution", 0);
	mCurrentMessage.addValue("creature", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("isevolution");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getIsSpeedAttacker(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creatureisspeedattacker");
	mCurrentMessage.addValue("isspeedattacker", 0);
	mCurrentMessage.addValue("creature", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("isspeedattacker");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCardCanCast(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get cardcancast");
	mCurrentMessage.addValue("cancast", 1);
	mCurrentMessage.addValue("card", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("cancast");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCardAiCanCast(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get cardaicancast");
	mCurrentMessage.addValue("cancast", 1);
	mCurrentMessage.addValue("card", uid);

	for (std::vector<Card*>::iterator card = mCardList.begin(); card != mCardList.end(); ++card)
		(*card)->handleMessage(mCurrentMessage);
	int canCast = mCurrentMessage.getInt("cancast");
	mCurrentMessage = oldmsg;
	return canCast;
}

int Duel::getCardAiPreferredChoice(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get cardaipreferredchoice");
	mCurrentMessage.addValue("selection", RETURN_NOTHING);
	mCurrentMessage.addValue("card", uid);

	for (std::vector<Card*>::iterator card = mCardList.begin(); card != mCardList.end(); ++card)
		(*card)->handleMessage(mCurrentMessage);
	int selection = mCurrentMessage.getInt("selection");
	mCurrentMessage = oldmsg;
	return selection;
}

int Duel::getCardCivilization(int uid)
{
	return mCardList.at(uid)->mCivilization;
}

int Duel::getCardCivilizations(int uid) const
{
	return mCardList.at(uid)->mCivilizations;
}

bool Duel::cardHasCivilization(int uid, int civ) const
{
	return uid >= 0 && uid < static_cast<int>(mCardList.size()) && civ >= CIV_LIGHT &&
		civ <= CIV_HOLLOW && (mCardList.at(uid)->mCivilizations & (1 << civ)) != 0;
}

int Duel::isCreatureOfRace(int uid, std::string race)
{
	std::string r = getCreatureRace(uid);
	//printf("race %s %s\n", r.c_str(), race.c_str());
	int f = r.find(race);
	if (f != std::string::npos)
	{
		if (f == 0 || r.at(f-1) == ' ' || r.at(f-1) == '/')
		{
			int s1 = race.size();
			int s2 = r.size();
			if (s1+f == s2 || r.at(f+s1) == ' ' || r.at(f+s1) == '/')
			{
				return 1;
			}
		}
	}
	return 0;
}

std::string Duel::getCreatureRace(int uid)
{
	if (uid < 0 || uid >= static_cast<int>(mCardList.size()))
		return "";

	// Race-changing modifiers are queried by broadcasting get creaturerace to
	// every card. Some continuous auras inspect creature races on every
	// broadcast, so allowing those nested inspections to broadcast again
	// recursively re-enters the same Lua callbacks until the C stack overflows.
	// Nested checks only need a stable value while the outer race query is
	// being assembled: use its current value for the same creature and the
	// printed race for any other creature.
	if (mRaceQueryDepth > 0)
	{
		if (mCurrentMessage.getType() == "get creaturerace" &&
			optionalMessageInt(mCurrentMessage, "creature", -1) == uid)
		{
			std::map<std::string, std::string>::const_iterator race =
				mCurrentMessage.map.find("race");
			if (race != mCurrentMessage.map.end())
				return race->second;
		}
		return mCardList.at(uid)->mRace;
	}

	++mRaceQueryDepth;
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturerace");
	mCurrentMessage.addValue("race", mCardList.at(uid)->mRace);
	mCurrentMessage.addValue("creature", uid);

	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	std::string race = mCurrentMessage.getString("race");
	mCurrentMessage = oldmsg;
	--mRaceQueryDepth;
	return race;
}

int Duel::getCreatureCanBeChosen(int uid, int chooser, int source)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanbechosen");
	mCurrentMessage.addValue("canchoose", 1);
	mCurrentMessage.addValue("creature", uid);
	mCurrentMessage.addValue("chooser", chooser);
	mCurrentMessage.addValue("source", source);
	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); i++)
		(*i)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("canchoose");
	mCurrentMessage = oldmsg;
	return result;
}

int Duel::getCreatureMustAttack(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturemustattack");
	mCurrentMessage.addValue("mustattack", 0);
	mCurrentMessage.addValue("creature", uid);
	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); i++)
		(*i)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("mustattack");
	mCurrentMessage = oldmsg;
	return result;
}

int Duel::getCreatureForcedBlocker(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creatureforcedblocker");
	mCurrentMessage.addValue("forcedblocker", -1);
	mCurrentMessage.addValue("attacker", uid);
	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); i++)
		(*i)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("forcedblocker");
	mCurrentMessage = oldmsg;
	return result;
}

bool Duel::canCreatureAttackNow(int uid)
{
	if (uid < 0 || uid >= static_cast<int>(mCardList.size())) return false;
	Card* attacker = mCardList.at(uid);
	if (attacker->mZone != ZONE_BATTLE || attacker->mIsTapped) return false;
	int canPlayers = getCreatureCanAttackPlayers(uid);
	if (canPlayers == CANATTACK_ALWAYS
		|| ((attacker->mSummoningSickness == 0 || getIsSpeedAttacker(uid) == 1)
			&& canPlayers <= CANATTACK_UNTAPPED))
		return true;
	if (attacker->mSummoningSickness != 0) return false;
	for (std::vector<Card*>::iterator i = mBattlezones[getOpponent(attacker->mOwner)].mCards.begin();
		i != mBattlezones[getOpponent(attacker->mOwner)].mCards.end(); i++)
	{
		int canCreature = getCreatureCanAttackCreature(uid, (*i)->mUniqueId);
		if (canCreature <= CANATTACK_UNTAPPED
			&& ((*i)->mIsTapped || canCreature == CANATTACK_UNTAPPED))
			return true;
	}
	return false;
}

bool Duel::canEndTurn()
{
	for (std::vector<Card*>::iterator i = mBattlezones[mTurn].mCards.begin(); i != mBattlezones[mTurn].mCards.end(); i++)
	{
		if (getCreatureMustAttack((*i)->mUniqueId) == 1 && canCreatureAttackNow((*i)->mUniqueId))
			return false;
	}
	return true;
}

int Duel::getCreatureCanEvolve(int evo, int bait)
{
	if (evo < 0 || bait < 0 || evo >= static_cast<int>(mCardList.size()) ||
		bait >= static_cast<int>(mCardList.size()) ||
		mCardList[evo]->mOwner != mCardList[bait]->mOwner ||
		mCardList[bait]->mZone != ZONE_BATTLE)
		return 0;

	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanevolve");
	mCurrentMessage.addValue("canevolve", 0);
	mCurrentMessage.addValue("evolution", evo);
	mCurrentMessage.addValue("evobait", bait);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("canevolve");
	mCurrentMessage = oldmsg;
	return c;
}

int Duel::getCreatureCanVortexEvolve(int evo, int bait, int bait2)
{
	if (evo < 0 || bait < 0 || bait2 < 0 || bait == bait2 ||
		evo >= static_cast<int>(mCardList.size()) ||
		bait >= static_cast<int>(mCardList.size()) ||
		bait2 >= static_cast<int>(mCardList.size()) ||
		mCardList[evo]->mOwner != mCardList[bait]->mOwner ||
		mCardList[evo]->mOwner != mCardList[bait2]->mOwner ||
		mCardList[bait]->mZone != ZONE_BATTLE ||
		mCardList[bait2]->mZone != ZONE_BATTLE)
		return 0;

	int baitWildcard = getCreatureCanEvolve(evo, bait);
	int bait2Wildcard = getCreatureCanEvolve(evo, bait2);
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturecanvortexevolve");
	mCurrentMessage.addValue("canevolve", 0);
	mCurrentMessage.addValue("evolution", evo);
	mCurrentMessage.addValue("evobait", bait);
	mCurrentMessage.addValue("evobait2", bait2);
	mCurrentMessage.addValue("evobaitwildcard", baitWildcard);
	mCurrentMessage.addValue("evobait2wildcard", bait2Wildcard);

	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); ++i)
		(*i)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("canevolve");
	mCurrentMessage = oldmsg;
	return result;
}

int Duel::getEvolutionBaitCount(int evo)
{
	if (evo < 0 || evo >= static_cast<int>(mCardList.size()) || getIsEvolution(evo) != 1)
		return 0;

	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creatureevolutionbaitcount");
	mCurrentMessage.addValue("baitcount", 1);
	mCurrentMessage.addValue("evolution", evo);
	for (std::vector<Card*>::iterator i = mCardList.begin(); i != mCardList.end(); ++i)
		(*i)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("baitcount");
	mCurrentMessage = oldmsg;
	return result;
}

int Duel::getShieldChooser(int chooser, int shieldOwner)
{
	if ((chooser != 0 && chooser != 1) || (shieldOwner != 0 && shieldOwner != 1))
		return chooser;

	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get shieldchooser");
	mCurrentMessage.addValue("chooser", chooser);
	mCurrentMessage.addValue("shieldowner", shieldOwner);
	for (std::vector<Card*>::iterator card = mCardList.begin(); card != mCardList.end(); ++card)
		(*card)->handleMessage(mCurrentMessage);
	int result = mCurrentMessage.getInt("chooser");
	mCurrentMessage = oldmsg;
	return result == 0 || result == 1 ? result : chooser;
}

int Duel::getCreatureHasTapAbility(int uid)
{
	Message oldmsg = mCurrentMessage;
	mCurrentMessage = Message("get creaturehastapability");
	mCurrentMessage.addValue("hastapability", 0);
	mCurrentMessage.addValue("creature", uid);

	std::vector<Card*>::iterator i;
	for (i = mCardList.begin(); i != mCardList.end(); i++)
	{
		(*i)->handleMessage(mCurrentMessage);
	}
	int c = mCurrentMessage.getInt("hastapability");
	mCurrentMessage = oldmsg;
	return c;
}

bool Duel::isThereUntappedManaOfCiv(int player,int civ)
{
	if (player < 0 || player > 1 || civ < CIV_LIGHT || civ > CIV_HOLLOW) return false;
	for (std::vector<Card*>::iterator i = mManazones[player].mCards.begin(); i != mManazones[player].mCards.end(); i++)
	{
		if ((*i)->mIsTapped == false && (civ == CIV_HOLLOW ||
			cardHasCivilization((*i)->mUniqueId, civ) ||
			cardHasCivilization((*i)->mUniqueId, CIV_HOLLOW)))
			return true;
	}
	return false;
}

bool Duel::canSatisfyCivilizations(int required, const std::vector<int>& fixedCards,
	const std::vector<int>& availableCards, int futureSlots) const
{
	const int hollow = 1 << CIV_HOLLOW;
	// Hollow cards accept mana of any civilization. Hollow mana is a wildcard
	// for conventional and multi-civilization cards.
	if ((required & hollow) != 0) required = 0;
	if (required == 0) return true;
	if (futureSlots < 0) return false;
	const int maskCount = 1 << (CIV_HOLLOW + 1);
	std::vector<char> fixedCoverage(maskCount, 0);
	fixedCoverage[0] = 1;
	for (size_t cardIndex = 0; cardIndex < fixedCards.size(); ++cardIndex)
	{
		int uid = fixedCards[cardIndex];
		if (uid < 0 || uid >= static_cast<int>(mCardList.size())) return false;
		int colors = (mCardList[uid]->mCivilizations & hollow) != 0 ?
			required : mCardList[uid]->mCivilizations & required;
		std::vector<char> next = fixedCoverage;
		for (int covered = 0; covered < maskCount; ++covered)
		{
			if (!fixedCoverage[covered]) continue;
			for (int civ = CIV_LIGHT; civ <= CIV_HOLLOW; ++civ)
				if ((colors & (1 << civ)) != 0) next[covered | (1 << civ)] = 1;
		}
		fixedCoverage.swap(next);
	}

	std::vector<std::vector<char> > coverage(futureSlots + 1,
		std::vector<char>(maskCount, 0));
	coverage[0] = fixedCoverage;
	for (size_t cardIndex = 0; cardIndex < availableCards.size(); ++cardIndex)
	{
		int uid = availableCards[cardIndex];
		if (uid < 0 || uid >= static_cast<int>(mCardList.size())) continue;
		int colors = (mCardList[uid]->mCivilizations & hollow) != 0 ?
			required : mCardList[uid]->mCivilizations & required;
		std::vector<std::vector<char> > next = coverage;
		for (int used = 0; used < futureSlots; ++used)
		{
			for (int covered = 0; covered < maskCount; ++covered)
			{
				if (!coverage[used][covered]) continue;
				for (int civ = CIV_LIGHT; civ <= CIV_HOLLOW; ++civ)
					if ((colors & (1 << civ)) != 0)
						next[used + 1][covered | (1 << civ)] = 1;
			}
		}
		coverage.swap(next);
	}
	for (int used = 0; used <= futureSlots; ++used)
		if (coverage[used][required]) return true;
	return false;
}

bool Duel::canPayForCard(int player, int uid)
{
	if (player < 0 || player > 1 || uid < 0 || uid >= static_cast<int>(mCardList.size()))
		return false;
	int cost = getCardCost(uid);
	std::vector<int> mana;
	for (size_t i = 0; i < mManazones[player].mCards.size(); ++i)
		if (!mManazones[player].mCards[i]->mIsTapped)
			mana.push_back(mManazones[player].mCards[i]->mUniqueId);
	if (cost < 0 || static_cast<int>(mana.size()) < cost) return false;
	return canSatisfyCivilizations(getCardCivilizations(uid), std::vector<int>(), mana, cost);
}

bool Duel::canTapManaForCasting(int uid)
{
	if (mCastingCard < 0 || mCastingCost <= 0 || uid < 0 ||
		uid >= static_cast<int>(mCardList.size())) return false;
	Card* mana = mCardList[uid];
	if (mana->mOwner != mTurn || mana->mZone != ZONE_MANA || mana->mIsTapped ||
		std::find(mCastingManaCards.begin(), mCastingManaCards.end(), uid) !=
		mCastingManaCards.end()) return false;

	std::vector<int> fixedCards = mCastingManaCards;
	fixedCards.push_back(uid);
	std::vector<int> availableCards;
	for (size_t i = 0; i < mManazones[mTurn].mCards.size(); ++i)
	{
		Card* card = mManazones[mTurn].mCards[i];
		if (!card->mIsTapped && card->mUniqueId != uid &&
			std::find(fixedCards.begin(), fixedCards.end(), card->mUniqueId) == fixedCards.end())
			availableCards.push_back(card->mUniqueId);
	}
	int remainingSlots = mCastingCost - 1;
	if (static_cast<int>(availableCards.size()) < remainingSlots) return false;
	return canSatisfyCivilizations(mCastingCivilizations, fixedCards,
		availableCards, remainingSlots);
}

void Duel::drawCards(int player, int count)
{
	if (player < 0 || player > 1 || count <= 0)
		return;
	int available = std::min(count, static_cast<int>(mDecks[player].mCards.size()));
	for (int i = 0; i < available; i++)
	{
		Message msg("cardmove");
		msg.addValue("card", mDecks[player].mCards.at(mDecks[player].mCards.size() - i - 1)->mUniqueId);
		msg.addValue("from", mDecks[player].mCards.at(mDecks[player].mCards.size() - i - 1)->mZone);
		msg.addValue("to", ZONE_HAND);
		msg.addValue("draw", 1);
		mMsgMngr.sendMessage(msg);
	}
	if (available < count && mWinner == -1)
		mWinner = getOpponent(player);
}

bool Duel::setDecks(const std::string& p1, const std::string& p2)
{
	if (!loadDeck(p1, 0)) return false;
	if (!loadDeck(p2, 1)) return false;

	/*decks[0].x = ZONE2X;
	decks[1].x = ZONE2X;
	decks[0].y = CENTER - ZONEYOFFSET * 4;
	decks[1].y = CENTER + ZONEYOFFSET * 3;*/
	return true;
}

bool Duel::loadDeck(const std::string& path, int player)
{
	constexpr int MINIMUM_DECK_CARDS = 10;
	mDecks[player].mCards.clear();
	std::vector<int> cardIds;
	std::string resolvedPath;
	if (!loadDeckCardIds(path, cardIds, MINIMUM_DECK_CARDS, &resolvedPath))
		return false;
	for (size_t copy = 0; copy < cardIds.size(); ++copy)
	{
		Card* card = new Card(mNextUniqueId, cardIds[copy], player);
		mCardList.push_back(card);
		mDecks[player].addCard(card);
		++mNextUniqueId;
	}
	mDeckNames[player] = resolvedPath;
	return true;
}

//void Duel::loadDeck(string s, int p)
//{
//	decks[p].cards.empty();
//	fstream file;
//	file.open(s, ios::in | ios::out);
//	string str;
//
//	while (!file.eof())
//	{
//		getline(file, str);
//		if (str == "")
//			continue;
//		cout << "loading card " << str << endl;
//		Card* c = new Card(nextUniqueId, getCardIdFromName(str), p);
//		CardList.push_back(c);
//		decks[p].addCard(c);
//		nextUniqueId++;
//	}
//
//	file.close();
//}

void Duel::startDuel()
{
	mTurn = (int)mRandomGen.Random(2);
	mTurnPhase = TURN_PHASE_MANA;
	mManaUsed = 0;
	mLuaRuleState.clear();
	mShieldBreakersThisTurn[0].clear();
	mShieldBreakersThisTurn[1].clear();
	mShieldsBrokenThisTurn[0] = 0;
	mShieldsBrokenThisTurn[1] = 0;
	mCardsDrawnThisTurn[0] = 0;
	mCardsDrawnThisTurn[1] = 0;
	for (int i = 0; i < 2; i++)
	{
		if (mDecks[i].mCards.size() < 40)
		{
			printf("WARNING: Deck card count less than 40\n");
		}
		mDecks[i].shuffle();
		for (int j = 0; j < 5; j++)
		{
			/*Card* c = decks[i].draw();
			shields[i].addCard(c);*/
			Message msg("cardmove");
			msg.addValue("card", mDecks[i].mCards.at(mDecks[i].mCards.size() - 1 - j)->mUniqueId);
			msg.addValue("from", ZONE_DECK);
			msg.addValue("to", ZONE_SHIELD);
			mMsgMngr.sendMessage(msg);
		}
		for (int j = 0; j < 5; j++)
		{
			/*Card* c = decks[i].draw();
			hands[i].addCard(c);*/
			Message msg("cardmove");
			msg.addValue("card", mDecks[i].mCards.at(mDecks[i].mCards.size() - 6 - j)->mUniqueId);
			msg.addValue("from", ZONE_DECK);
			msg.addValue("to", ZONE_HAND);
			mMsgMngr.sendMessage(msg);
		}
	}
}

void Duel::nextTurn()
{
	Message msg("endturn");
	msg.addValue("player", mTurn);
	mMsgMngr.sendMessage(msg);
}

void Duel::resetAttack()
{
	//cout << "attackreset" << endl;
	mAttackphase = PHASE_NONE;
	mAttacker = -1;
	mDefender = -1;
	mDefenderType = -1;
	mBreakCount = -1;
	mShieldTargets.clear();
}

void Duel::resetCasting()
{
	//cout << "casting reset" << endl;
	mCastingCard = -1;
	mCastingCivilizations = 0;
	mCastingManaCards.clear();
	mCastingCost = -1;
	mCastingEvobait = -1;
	mCastingEvobait2 = -1;
}

void Duel::clearCards()
{
	for (int i = 0; i < 2; i++)
	{
		mDecks[i].mCards.clear();
		mGraveyards[i].mCards.clear();
		mHands[i].mCards.clear();
		mManazones[i].mCards.clear();
		mShields[i].mCards.clear();
		mBattlezones[i].mCards.clear();

		mShields[i].mSlotsUsed = 0;
	}
	for (std::vector<Card*>::iterator card = mCardList.begin(); card != mCardList.end(); ++card)
		delete *card;
	mCardList.clear();
	mNextUniqueId = 0;
	mTurnPhase = TURN_PHASE_MANA;
	mManaUsed = 0;
	mShieldBreakersThisTurn[0].clear();
	mShieldBreakersThisTurn[1].clear();
	mShieldsBrokenThisTurn[0] = 0;
	mShieldsBrokenThisTurn[1] = 0;
	mLuaRuleState.clear();
	mSimulationChoiceFailed = false;
	mZeroPowerCheckPending = false;
	mRaceQueryDepth = 0;
}

int Duel::getLuaRuleState(const std::string& name, int index, int fallback) const
{
	std::unordered_map<std::string, std::unordered_map<int, int> >::const_iterator state = mLuaRuleState.find(name);
	if (state == mLuaRuleState.end())
		return fallback;
	std::unordered_map<int, int>::const_iterator value = state->second.find(index);
	return value == state->second.end() ? fallback : value->second;
}

void Duel::setLuaRuleState(const std::string& name, int index, int value)
{
	mLuaRuleState[name][index] = value;
}

void Duel::clearLuaRuleState(const std::string& name, int index)
{
	std::unordered_map<std::string, std::unordered_map<int, int> >::iterator state = mLuaRuleState.find(name);
	if (state == mLuaRuleState.end())
		return;
	state->second.erase(index);
	if (state->second.empty())
		mLuaRuleState.erase(state);
}

void Duel::rebuildShieldBreakersThisTurn()
{
	mShieldBreakersThisTurn[0].clear();
	mShieldBreakersThisTurn[1].clear();
	mShieldsBrokenThisTurn[0] = 0;
	mShieldsBrokenThisTurn[1] = 0;
	for (std::vector<MsgHistoryItem>::reverse_iterator i = mMessageHistory.rbegin(); i != mMessageHistory.rend(); i++)
	{
		if (i->msg.getType() == "endturn")
			break;
		if (i->msg.getType() == "creaturebreakshield")
		{
			int creature = i->msg.getInt("creature");
			if (creature >= 0 && creature < static_cast<int>(mCardList.size()))
			{
				int owner = mCardList.at(creature)->mOwner;
				mShieldBreakersThisTurn[owner].insert(creature);
				mShieldsBrokenThisTurn[owner]++;
			}
		}
	}
}

bool Duel::hasOtherCreatureBrokenShieldThisTurn(int uid) const
{
	if (uid < 0 || uid >= static_cast<int>(mCardList.size()))
		return false;

	const std::unordered_set<int>& breakers = mShieldBreakersThisTurn[mCardList.at(uid)->mOwner];
	return breakers.size() > 1 || (breakers.size() == 1 && breakers.count(uid) == 0);
}

bool Duel::hasCreatureBrokenShieldThisTurn(int uid) const
{
	if (uid < 0 || uid >= static_cast<int>(mCardList.size()))
		return false;

	const std::unordered_set<int>& breakers = mShieldBreakersThisTurn[mCardList.at(uid)->mOwner];
	return breakers.count(uid) != 0;
}

int Duel::getShieldsBrokenThisTurn(int player) const
{
	return player == 0 || player == 1 ? mShieldsBrokenThisTurn[player] : 0;
}

int Duel::getCardsDrawnThisTurn(int player) const
{
	return player == 0 || player == 1 ? mCardsDrawnThisTurn[player] : 0;
}

ActiveDuelGuard::ActiveDuelGuard(Duel& duel) : mPrevious(ActiveDuel)
{
	ActiveDuel = &duel;
}

ActiveDuelGuard::~ActiveDuelGuard()
{
	ActiveDuel = mPrevious;
}

void Duel::resetChoice()
{
	mChoiceCard = -1;
	mChoicePlayer = -1;
	mIsChoiceActive = false;
	mChoiceValidCards.clear();
	//cout << "choice reset" << endl;
}

Zone* Duel::getZone(int player, int zone)
{
	if (zone == ZONE_BATTLE)
	{
		return &mBattlezones[player];
	}
	else if (zone == ZONE_MANA)
	{
		return &mManazones[player];
	}
	else if (zone == ZONE_HAND)
	{
		return &mHands[player];
	}
	else if (zone == ZONE_DECK)
	{
		return &mDecks[player];
	}
	else if (zone == ZONE_SHIELD)
	{
		return &mShields[player];
	}
	else if (zone == ZONE_GRAVEYARD)
	{
		return &mGraveyards[player];
	}
	else if (zone == ZONE_EVOLVED)
	{
		return &mBattlezones[player];
	}
	printf("WARNING: getZone called with unknown zone type: %d\n", zone);
	return NULL;
}

void Duel::flipCard(int cid)
{
	mCardList.at(cid)->flip();
}

void Duel::unflipCard(int cid)
{
	mCardList.at(cid)->unflip();
}

void Duel::setCardVisibility(int cid, int player, int visibility)
{
	mCardList[cid]->setVisibility(player, visibility);
}

void Duel::destroyCard(Card* c)
{
	//moveCard(c, ZONE_GRAVEYARD);
}

int getOpponent(int turn)
{
	return (turn + 1) % 2;
}
