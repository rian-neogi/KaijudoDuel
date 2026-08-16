#pragma once

#include "Choice.h"

#include <fstream>
#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

enum AttackPhase { PHASE_NONE, PHASE_BLOCK, PHASE_TARGET, PHASE_TRIGGER };
enum TurnPhase { TURN_PHASE_MANA, TURN_PHASE_MAIN, TURN_PHASE_ATTACK };
enum CanAttack { CANATTACK_TAPPED, CANATTACK_UNTAPPED, CANATTACK_NO, CANATTACK_ALWAYS };
enum ReturnValue { RETURN_BUTTON1 = -1, RETURN_BUTTON2 = -2, RETURN_NOVALID = -3, RETURN_NOTHING = -4, RETURN_QUIT = -5 };
enum PlayerType { PLAYER_HUMAN, PLAYER_AI };

struct MsgHistoryItem
{
	Message msg;
	int move;
};

class Duel
{
public:
	typedef std::function<int(const Duel&)> ChoiceResolver;

	Deck mDecks[2];
	Hand mHands[2];
	ManaZone mManazones[2];
	Graveyard mGraveyards[2];
	ShieldZone mShields[2];
	BattleZone mBattlezones[2];

	std::vector<Card*> mCardList;

	std::string mDeckNames[2];

	bool mIsSimulation;
	std::atomic<bool> mInputLoopRunning;
	std::atomic<bool> mLuaCallbackSuspended;
	std::atomic<bool> mAiThinking;
	std::atomic<int> mAiThinkingPlayer;
	std::vector<MsgHistoryItem> mMessageHistory;
	std::vector<Message> mMoveHistory;
	std::vector<int> mMovePlayers;
	int mCurrentMoveCount;

	CRandom mRandomGen;

	int mAttacker;
	int mDefender;
	int mDefenderType;
	int mBreakCount;
	std::vector<int> mShieldTargets;
	std::unordered_set<int> mShieldBreakersThisTurn[2];
	int mShieldsBrokenThisTurn[2];
	int mCardsDrawnThisTurn[2];
	std::unordered_map<std::string, std::unordered_map<int, int> > mLuaRuleState;
	int mAttackphase;

	int mCastingCard;
	int mCastingCivilizations;
	int mCastingCost;
	int mCastingEvobait;
	int mCastingEvobait2;
	std::vector<int> mCastingManaCards;

	Choice* mChoice;
	int mChoiceCard;
	int mChoicePlayer;
	bool mIsChoiceActive;
	std::vector<int> mChoiceValidCards;
	ChoiceResolver mChoiceResolver;
	int mChoiceResolverAnswersRemaining;
	bool mSimulationChoiceFailed;
	bool mZeroPowerCheckPending;
	int mRaceQueryDepth;

	int mWinner;

	int mNextUniqueId;

	MessageManager mMsgMngr;
	Message mCurrentMessage;

	int mTurn;
	int mTurnPhase;
	int mManaUsed;

	int mPlayerType[2];

	Duel();
	~Duel();

	Duel(const Duel&) = delete;
	Duel& operator=(const Duel&) = delete;

	bool isCloneable() const;
	bool copyFrom(const Duel& duel);

	bool setDecks(const std::string& p1, const std::string& p2);
	bool loadDeck(const std::string& path, int player);
	void startDuel();
	void nextTurn();

	//Game Messages
	int handleMessage(Message& msg);
	bool dispatchAllMessages();
	void dispatchMessage(Message& msg);
	//void parseMessages(unsigned int deltatime);

	//Interface Messages
	int handleInterfaceInput(Message& msg);
	void loopInput();
	void stopInputLoop();
	int waitForChoice();
	int resolveChoice();
	void setChoiceResolver(const ChoiceResolver& resolver, int answersRemaining = -1);
	void clearChoiceResolver();
	bool hasSimulationChoiceFailure() const;
	void clearSimulationChoiceFailure();

	//For AI
	int getPlayerToMove();
	std::vector<Message> getPossibleMoves();
	void undoLastMove();
	void undoMessage(Message& msg);

	//Other
	void addChoice(std::string info, int skip, int card, int player, int validref, int actionref,
		int aiPreferredSelection = RETURN_NOTHING);
	void checkChoiceValid();
	int choiceCanBeSelected(int sid) const;
	bool selectChoice(int sid, bool sendMessage);
	void cancelChoice();

	//void undoSelection();
	void resetAttack();
	void resetCasting();
	void resetChoice();
	void clearCards();
	void rebuildShieldBreakersThisTurn();
	void scheduleZeroPowerCheck();
	void probeBattleZonePower();
	int getLuaRuleState(const std::string& name, int index, int fallback) const;
	void setLuaRuleState(const std::string& name, int index, int value);
	void clearLuaRuleState(const std::string& name, int index);

	void setMyPlayer(int p);

	bool isThereUntappedManaOfCiv(int player, int civ);
	bool canPayForCard(int player, int uid);
	bool canTapManaForCasting(int uid);
	bool canSatisfyCivilizations(int required, const std::vector<int>& fixedCards,
		const std::vector<int>& availableCards, int futureSlots) const;

	Zone* getZone(int player, int zone);
	void destroyCard(Card* c);
	void battle(int att, int def);
	Card* getCard(int player, int zone, int id);

	void flipCard(int cid);
	void unflipCard(int cid);
	void setCardVisibility(int cid, int player, int visibility);

	void drawCards(int player, int count);

	int getCreaturePower(int uid);
	int getCreatureBreaker(int uid);
	int getCreatureIsBlocker(int uid);
	int getCreatureCanBlock(int attckr, int blckr);
	int getCreatureCanBlockRepeatedly(int uid);
	//int getCreatureCanAttack(int uid);
	int getCreatureCanAttackPlayers(int uid);
	//int getCreatureCanAttackCreatures(int uid);
	//int getCreatureCanAttackTarget(int attckr, int dfndr);
	int getCreatureCanAttackCreature(int attckr, int dfndr);
	int getCreatureCanBeChosen(int uid, int chooser, int source);
	int getCreatureMustAttack(int uid);
	int getCreatureForcedBlocker(int uid);
	bool canCreatureAttackNow(int uid);
	bool canEndTurn();
	//int getCreatureCanBeAttacked(int attckr, int dfndr);
	//int getCreatureCanBeBlocked(int uid);
	//int getCreatureCanAttackUntappedCreatures(int uid);
	int getCardCost(int uid);
	int getCardCivilization(int uid);
	int getCardCivilizations(int uid) const;
	bool cardHasCivilization(int uid, int civ) const;
	int getIsShieldTrigger(int uid);
	int canUseShieldTrigger(int uid);
	int getIsEvolution(int uid);
	int getIsSpeedAttacker(int uid);
	int getCardCanCast(int uid);
	int getCardAiCanCast(int uid);
	int getCardAiPreferredChoice(int uid);
	int isCreatureOfRace(int uid, std::string race); //finds if the word race exists in the creature's race
	std::string getCreatureRace(int uid); //returns the full race string of the creature
	int getCreatureCanEvolve(int evo, int bait);
	int getCreatureCanVortexEvolve(int evo, int bait, int bait2);
	int getEvolutionBaitCount(int evo);
	int getShieldChooser(int chooser, int shieldOwner);
	int getCreatureHasTapAbility(int uid);
	bool hasCreatureBrokenShieldThisTurn(int uid) const;
	bool hasOtherCreatureBrokenShieldThisTurn(int uid) const;
	int getShieldsBrokenThisTurn(int player) const;
	int getCardsDrawnThisTurn(int player) const;
};

extern Duel* ActiveDuel;

class ActiveDuelGuard
{
public:
	explicit ActiveDuelGuard(Duel& duel);
	~ActiveDuelGuard();

	ActiveDuelGuard(const ActiveDuelGuard&) = delete;
	ActiveDuelGuard& operator=(const ActiveDuelGuard&) = delete;

private:
	Duel* mPrevious;
};

extern std::mutex gMutex;
