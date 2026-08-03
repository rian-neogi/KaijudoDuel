#include "Application.h"

#include "AI/HeuristicBot.h"
#include "AppSupport.h"
#include "Game/Card.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

using namespace AppSupport;

namespace
{
	constexpr int GRAVEYARD_PAGE_SIZE = 5;
	const SDL_Rect GRAVEYARD_OVERLAY = { 105, 145, 770, 510 };
	const SDL_Rect GRAVEYARD_CLOSE = { 810, 160, 44, 36 };
	const SDL_Rect GRAVEYARD_PREVIOUS = { 135, 590, 145, 42 };
	const SDL_Rect GRAVEYARD_NEXT = { 700, 590, 145, 42 };
	const SDL_Rect ACTION_LOG_BUTTON = { 1004, 746, 252, 40 };
	const SDL_Rect ACTION_LOG_OVERLAY = { 115, 75, 1050, 650 };
	const SDL_Rect ACTION_LOG_CLOSE = { 1095, 91, 44, 36 };
	constexpr int ACTION_LOG_PAGE_SIZE = 17;
	constexpr Uint32 AI_MOVE_DELAY_MS = 600;
	constexpr Uint32 AI_MANA_TAP_DELAY_MS = 100;

	std::string deckDisplayName(const std::string& path)
	{
		size_t slash = path.find_last_of("/\\");
		std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
		if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
			name.resize(name.size() - 4);
		return name;
	}

	std::string actionIdentity(const Message& message)
	{
		std::string identity;
		for (std::map<std::string, std::string>::const_iterator value = message.map.begin();
			value != message.map.end(); ++value)
		{
			identity += value->first;
			identity += '=';
			identity += value->second;
			identity += ';';
		}
		return identity;
	}
}

void Application::startDuel(int npcIndex, bool ignoreProgressLimit)
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size() || !mNpcs[npcIndex].isDuelist()) return;
	if (!ignoreProgressLimit && !npcVisible(npcIndex)) return;
	if (!ignoreProgressLimit && !mNpcs[npcIndex].canBattle()) return;
	ensurePlayerDataLoaded();
	const std::string aiDeck = ignoreProgressLimit ?
		mNpcs[npcIndex].deckForBattle(0) : mNpcs[npcIndex].battleDeck();
	if (!startDuelWithDecks(mActiveDeckPath, aiDeck, npcIndex))
	{
		mNotice = "Unable to load one of the duel decks.";
		mNoticeUntil = SDL_GetTicks() + 5000;
	}
}

bool Application::startDuelWithDecks(const std::string& playerDeck,
	const std::string& aiDeck, int npcIndex)
{
	stopDuel();
	mActiveNpc = npcIndex;
	mDuel = new Duel();
	ActiveDuel = mDuel;
	if (!mDuel->setDecks(playerDeck, aiDeck))
	{
		stopDuel();
		return false;
	}
	mDuel->startDuel();
	mDuelThread = std::thread(&Duel::loopInput, mDuel);
	mSelectedCard = -1;
	mHoveredCard = -1;
	mHoverCandidateCard = -1;
	mHoverCandidateSince = 0;
	mOnlyActionCandidate.clear();
	mOnlyActionCandidateSince = 0;
	mOnlyActionDispatched = false;
	cancelDrag();
	mCardAnimations.clear();
	mActionScroll = 0;
	mOpenGraveyardPlayer = -1;
	mGraveyardOffset = 0;
	mActionLogOpen = false;
	mActionLogScroll = 0;
	mNextAiMove = SDL_GetTicks() + 700;
	mDuelResult = -1;
	mDuelResultAt = 0;
	mDialogueNpc = -1;
	mScreen = Screen::Duel;
	return true;
}

void Application::stopDuel()
{
	if (mDuel == NULL) return;
	mDuel->stopInputLoop();
	if (mDuelThread.joinable()) mDuelThread.join();
	delete mDuel;
	mDuel = NULL;
	ActiveDuel = NULL;
	mCardHitboxes.clear();
	mActionButtons.clear();
	mCardAnimations.clear();
	mOpenGraveyardPlayer = -1;
	mGraveyardOffset = 0;
	mActionLogOpen = false;
	mActionLogScroll = 0;
	mHoveredCard = -1;
	mHoverCandidateCard = -1;
	mHoverCandidateSince = 0;
	mOnlyActionCandidate.clear();
	mOnlyActionCandidateSince = 0;
	mOnlyActionDispatched = false;
	cancelDrag();
}

void Application::handleDuelEvent(const SDL_Event& event)
{
	if (mActionLogOpen && handleActionLogEvent(event)) return;
	if (mOpenGraveyardPlayer >= 0 && handleGraveyardEvent(event)) return;
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			if (mDuelResult != -1) return;
			if (mDraggingCard >= 0)
			{
				cancelDrag();
				return;
			}
			stopDuel();
			if (mDirectDuelMode)
			{
				mRunning = false;
				return;
			}
			mScreen = Screen::Overworld;
			mNotice = "You left the duel.";
			mNoticeUntil = SDL_GetTicks() + 3000;
			return;
		}
		if (key == SDLK_TAB)
		{
			mSelectedCard = -1;
			mActionScroll = 0;
		}
		if (key == SDLK_l)
		{
			mActionLogOpen = true;
			mActionLogScroll = 0;
			cancelDrag();
			return;
		}
		if (key >= SDLK_1 && key <= SDLK_9)
		{
			int index = (int)(key - SDLK_1);
			if (index >= 0 && index < (int)mActionButtons.size())
				playAction(mActionButtons[index].message);
		}
	}
	else if (event.type == SDL_MOUSEWHEEL)
	{
		mActionScroll = std::max(0, mActionScroll - event.wheel.y);
	}
	else if (event.type == SDL_MOUSEMOTION)
	{
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
		if (mDraggingCard >= 0)
		{
			mDragMouseX = mMouseX;
			mDragMouseY = mMouseY;
		}
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		int x, y;
		logicalMouse(event.button.x, event.button.y, x, y);
		if (contains(ACTION_LOG_BUTTON, x, y))
		{
			mActionLogOpen = true;
			mActionLogScroll = 0;
			mSelectedCard = -1;
			cancelDrag();
			return;
		}
		for (int player = 0; player < 2; ++player)
		{
			if (contains(graveyardPileRect(player), x, y))
			{
				mOpenGraveyardPlayer = player;
				mGraveyardOffset = 0;
				mSelectedCard = -1;
				mHoveredCard = -1;
				mHoverCandidateCard = -1;
				return;
			}
		}
		for (size_t i = 0; i < mActionButtons.size(); ++i)
		{
			if (contains(mActionButtons[i].rect, x, y))
			{
				playAction(mActionButtons[i].message);
				return;
			}
		}
		for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
		{
			if (contains(item->rect, x, y))
			{
				Message directAction;
				if (findClickAction(item->cardId, directAction))
				{
					playAction(directAction);
					return;
				}
				if (event.button.clicks >= 2)
				{
					Message tapAbility;
					if (findDragAction("creatureusetapability", item->cardId, -1, tapAbility))
					{
						cancelDrag();
						playAction(tapAbility);
						return;
					}
				}
				bool draggable = false;
				{
					std::lock_guard<std::mutex> lock(gMutex);
					if (mDuel != NULL && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size())
					{
						Card* card = mDuel->mCardList[item->cardId];
						draggable = card->mOwner == 0 && (card->mZone == ZONE_HAND || card->mZone == ZONE_BATTLE);
					}
				}
				if (draggable) beginDrag(item->cardId, item->rect, x, y);
				else
				{
					mSelectedCard = mSelectedCard == item->cardId ? -1 : item->cardId;
					mActionScroll = 0;
				}
				return;
			}
		}
	}
	else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && mDraggingCard >= 0)
	{
		int x, y;
		logicalMouse(event.button.x, event.button.y, x, y);
		finishDrag(x, y);
	}
}

bool Application::handleGraveyardEvent(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
		return true;
	}
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		if (event.key.keysym.sym == SDLK_ESCAPE)
		{
			mOpenGraveyardPlayer = -1;
			mHoveredCard = -1;
			mHoverCandidateCard = -1;
			return true;
		}
		if (event.key.keysym.sym != SDLK_LEFT && event.key.keysym.sym != SDLK_RIGHT)
			return true;
		int count = 0;
		{
			std::lock_guard<std::mutex> lock(gMutex);
			if (mDuel != NULL) count = (int)mDuel->mGraveyards[mOpenGraveyardPlayer].mCards.size();
		}
		int direction = event.key.keysym.sym == SDLK_LEFT ? -GRAVEYARD_PAGE_SIZE : GRAVEYARD_PAGE_SIZE;
		mGraveyardOffset = std::max(0, std::min(std::max(0, count - 1), mGraveyardOffset + direction));
		return true;
	}
	if (event.type == SDL_MOUSEWHEEL)
	{
		int count = 0;
		{
			std::lock_guard<std::mutex> lock(gMutex);
			if (mDuel != NULL) count = (int)mDuel->mGraveyards[mOpenGraveyardPlayer].mCards.size();
		}
		mGraveyardOffset = std::max(0, std::min(std::max(0, count - 1), mGraveyardOffset - event.wheel.y));
		return true;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT)
		return true;

	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(GRAVEYARD_CLOSE, x, y))
	{
		mOpenGraveyardPlayer = -1;
		mHoveredCard = -1;
		mHoverCandidateCard = -1;
		return true;
	}
	int count = 0;
	{
		std::lock_guard<std::mutex> lock(gMutex);
		if (mDuel != NULL) count = (int)mDuel->mGraveyards[mOpenGraveyardPlayer].mCards.size();
	}
	if (contains(GRAVEYARD_PREVIOUS, x, y))
	{
		mGraveyardOffset = std::max(0, mGraveyardOffset - GRAVEYARD_PAGE_SIZE);
		return true;
	}
	if (contains(GRAVEYARD_NEXT, x, y))
	{
		mGraveyardOffset = std::min(std::max(0, count - 1), mGraveyardOffset + GRAVEYARD_PAGE_SIZE);
		return true;
	}

	for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
	{
		if (!contains(item->rect, x, y)) continue;
		bool isOpenGraveyardCard = false;
		{
			std::lock_guard<std::mutex> lock(gMutex);
			if (mDuel != NULL && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size())
			{
				Card* card = mDuel->mCardList[item->cardId];
				isOpenGraveyardCard = card->mOwner == mOpenGraveyardPlayer && card->mZone == ZONE_GRAVEYARD;
			}
		}
		if (!isOpenGraveyardCard) continue;
		Message choice;
		if (findClickAction(item->cardId, choice))
		{
			mOpenGraveyardPlayer = -1;
			playAction(choice);
		}
		else
		{
			mSelectedCard = mSelectedCard == item->cardId ? -1 : item->cardId;
			mActionScroll = 0;
		}
		return true;
	}
	return true;
}

bool Application::handleActionLogEvent(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
		return true;
	}
	if (event.type == SDL_MOUSEWHEEL)
	{
		mActionLogScroll = std::max(0, mActionLogScroll + event.wheel.y * 3);
		return true;
	}
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_l)
		{
			mActionLogOpen = false;
			mActionLogScroll = 0;
		}
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		int x, y;
		logicalMouse(event.button.x, event.button.y, x, y);
		if (contains(ACTION_LOG_CLOSE, x, y) || contains(ACTION_LOG_BUTTON, x, y))
		{
			mActionLogOpen = false;
			mActionLogScroll = 0;
		}
	}
	return true;
}

void Application::updateDuel(Uint32 deltaTime)
{
	if (mDuel == NULL) return;
	updateCardAnimations(deltaTime);
	Uint32 now = SDL_GetTicks();
	int winner = -1;
	Message automaticAction;
	bool chooseAutomaticAction = false;
	{
		std::lock_guard<std::mutex> lock(gMutex);
		winner = mDuel->mWinner;
		if (winner == -1 && mDuel->getPlayerToMove() == 1 && now >= mNextAiMove && !mDuel->mMsgMngr.hasMoreMessages())
		{
			std::vector<Message> moves = mDuel->getPossibleMoves();
			Uint32 nextMoveDelay = AI_MOVE_DELAY_MS;
			if (!moves.empty())
			{
				const std::string personality = mActiveNpc >= 0 ?
					mNpcs[mActiveNpc].aiPersonality : "balanced";
				HeuristicBot rival(1, personality);
				Message move = rival.chooseMove(*mDuel, moves);
				if (move.getType() == "manatap") nextMoveDelay = AI_MANA_TAP_DELAY_MS;
				mDuel->handleInterfaceInput(move);
			}
			mNextAiMove = now + nextMoveDelay;
		}

		bool canChoose = winner == -1 && mAutoChooseOnlyAction &&
			mDuel->getPlayerToMove() == 0 && !mDuel->mMsgMngr.hasMoreMessages() &&
			!mActionLogOpen && mOpenGraveyardPlayer < 0 && mDraggingCard < 0;
		if (canChoose)
		{
			std::vector<Message> actions = mDuel->getPossibleMoves();
			if (actions.size() == 1)
			{
				std::string identity = actionIdentity(actions[0]);
				if (identity != mOnlyActionCandidate)
				{
					mOnlyActionCandidate = identity;
					mOnlyActionCandidateSince = now;
					mOnlyActionDispatched = false;
				}
				else if (!mOnlyActionDispatched && now - mOnlyActionCandidateSince >= 350)
				{
					automaticAction = actions[0];
					mOnlyActionDispatched = true;
					chooseAutomaticAction = true;
				}
			}
			else
			{
				mOnlyActionCandidate.clear();
				mOnlyActionDispatched = false;
			}
		}
		else
		{
			mOnlyActionCandidate.clear();
			mOnlyActionDispatched = false;
		}
	}
	if (chooseAutomaticAction) playAction(automaticAction);

	if (winner != -1 && mDuelResult == -1)
	{
		mDuelResult = winner;
		mDuelResultAt = now;
	}
	if (mDuelResult != -1 && now - mDuelResultAt > 1800)
	{
		bool won = mDuelResult == 0;
		std::string rival = mActiveNpc >= 0 ? mNpcs[mActiveNpc].name : "your rival";
		if (won && mActiveNpc >= 0) awardNpcVictory(mActiveNpc);
		else if (!mDirectDuelMode)
		{
			mNotice = mActiveNpc >= 0 ? mNpcs[mActiveNpc].dialogueText("victory") : "";
			if (!mNotice.empty()) mNotice += "  ";
			mNotice += "Defeat. You can challenge " + rival + " again.";
			mNoticeUntil = now + 5000;
		}
		stopDuel();
		if (mDirectDuelMode)
			mRunning = false;
		else
			mScreen = Screen::Overworld;
	}
}

void Application::playAction(const Message& message)
{
	if (mDuel == NULL || mDuelResult != -1) return;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->getPlayerToMove() == 0)
	{
		Message action = message;
		mDuel->handleInterfaceInput(action);
	}
	mSelectedCard = -1;
	mActionScroll = 0;
}

bool Application::exerciseMultiCivilizationSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int dualCardId = getCardIdFromName("Deklowaz, the Terminator");
	int darknessCardId = getCardIdFromName("Bone Spider");
	int fireCardId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (dualCardId < 0 || darknessCardId < 0 || fireCardId < 0) return false;

	Duel* savedActiveDuel = ActiveDuel;
	bool valid = true;
	{
		Duel test;
		test.mIsSimulation = true;
		ActiveDuel = &test;
		auto addCard = [&test](int cardId, int zone) -> int
		{
			int uid = (int)test.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			test.mCardList.push_back(card);
			test.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};

		for (int i = 0; i < 6; ++i) addCard(darknessCardId, ZONE_MANA);
		int fireMana = addCard(fireCardId, ZONE_MANA);
		int dualCard = addCard(dualCardId, ZONE_HAND);
		valid = valid && test.cardHasCivilization(dualCard, CIV_DARKNESS) &&
			test.cardHasCivilization(dualCard, CIV_FIRE) &&
			!test.cardHasCivilization(dualCard, CIV_LIGHT) && test.canPayForCard(0, dualCard);

		test.mCardList[fireMana]->tap();
		valid = valid && !test.canPayForCard(0, dualCard);
		test.mCardList[fireMana]->untap();

		test.mTurn = 0;
		test.mTurnPhase = TURN_PHASE_MAIN;
		Message play("cardplay");
		play.addValue("card", dualCard);
		play.addValue("evobait", -1);
		test.handleInterfaceInput(play);
		int payments = 0;
		for (int safety = 0; test.mCastingCard != -1 && safety < 10; ++safety)
		{
			std::vector<Message> moves = test.getPossibleMoves();
			std::vector<Message>::iterator mana = std::find_if(moves.begin(), moves.end(),
				[](Message& move) { return move.getType() == "manatap"; });
			if (mana == moves.end())
			{
				valid = false;
				break;
			}
			test.handleInterfaceInput(*mana);
			test.dispatchAllMessages();
			++payments;
		}
		valid = valid && payments == 6 && test.mCastingCard == -1 &&
			test.mCardList[dualCard]->mZone == ZONE_BATTLE &&
			test.mCardList[fireMana]->mIsTapped;

		int tappedDual = addCard(dualCardId, ZONE_HAND);
		Message charge("cardmove");
		charge.addValue("card", tappedDual);
		charge.addValue("from", ZONE_HAND);
		charge.addValue("to", ZONE_MANA);
		test.dispatchMessage(charge);
		test.dispatchAllMessages();
		valid = valid && test.mCardList[tappedDual]->mZone == ZONE_MANA &&
			test.mCardList[tappedDual]->mIsTapped;
	}
	ActiveDuel = savedActiveDuel;
	return valid;
}

bool Application::exerciseEvolutionSmoke()
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	int savedAttackPhase = mDuel->mAttackphase;
	mDuel->mAttackphase = PHASE_TARGET;
	std::vector<Message> shieldMoves = mDuel->getPossibleMoves();
	mDuel->mAttackphase = savedAttackPhase;
	if (shieldMoves.empty()) return false;
	for (size_t i = 0; i < shieldMoves.size(); ++i)
		if (shieldMoves[i].getType() != "targetshield") return false;

	auto findCard = [this](const std::string& name, const std::set<int>& excluded) -> int
	{
		for (size_t i = 0; i < mDuel->mCardList.size(); ++i)
			if (mDuel->mCardList[i]->mName == name && excluded.find((int)i) == excluded.end()) return (int)i;
		return -1;
	};
	auto moveCard = [this](int cardId, int zone, int evolutionBase)
	{
		Message move("cardmove");
		move.addValue("card", cardId);
		move.addValue("from", mDuel->mCardList[cardId]->mZone);
		move.addValue("to", zone);
		if (evolutionBase >= 0) move.addValue("evobait", evolutionBase);
		mDuel->dispatchMessage(move);
	};

	std::set<int> used;
	int guardian = findCard("La Ura Giga, Sky Guardian", used);
	if (guardian >= 0) used.insert(guardian);
	int larba = findCard("Larba Geer, the Immaculate", used);
	if (larba >= 0) used.insert(larba);
	int initiate = findCard("Miele, Vizier of Lightning", used);
	if (initiate >= 0) used.insert(initiate);
	int craze = findCard("Craze Valkyrie, the Drastic", used);
	if (craze >= 0) used.insert(craze);
	int opponent = findCard("Bone Spider", used);
	if (opponent >= 0) used.insert(opponent);
	if (guardian < 0 || larba < 0 || initiate < 0 || craze < 0 || opponent < 0) return false;

	moveCard(guardian, ZONE_BATTLE, -1);
	moveCard(larba, ZONE_BATTLE, guardian);
	moveCard(initiate, ZONE_BATTLE, -1);
	moveCard(opponent, ZONE_BATTLE, -1);
	Message evolve("cardmove");
	evolve.addValue("card", craze);
	evolve.addValue("from", mDuel->mCardList[craze]->mZone);
	evolve.addValue("to", ZONE_BATTLE);
	evolve.addValue("evobait", initiate);
	mDuel->mMsgMngr.sendMessage(evolve);

	if (mDuel->mCardList[guardian]->mZone != ZONE_EVOLVED ||
		mDuel->mCardList[larba]->mZone != ZONE_BATTLE ||
		mDuel->mCardList[larba]->mEvoStack.empty()) return false;

	(void)mDuel->getCreaturePower(larba);
	return true;
}

bool Application::exerciseBinaryChoiceSmoke()
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->mIsChoiceActive || mDuel->mChoice != NULL) return false;

	mDuel->addChoice("Draw a card?", 2, 0, 0, LUA_REFNIL, LUA_REFNIL);
	mDuel->mLuaCallbackSuspended = true;
	std::vector<Message> actions = visibleActions();
	bool hasYes = false;
	bool hasNo = false;
	for (size_t i = 0; i < actions.size(); ++i)
	{
		if (actions[i].getType() != "choiceselect") continue;
		int selection = messageInt(actions[i], "selection");
		if (selection == RETURN_BUTTON1 && actionLabel(actions[i]) == "Yes") hasYes = true;
		if (selection == RETURN_BUTTON2 && actionLabel(actions[i]) == "No") hasNo = true;
	}

	Choice* temporaryChoice = mDuel->mChoice;
	mDuel->mChoice = NULL;
	mDuel->resetChoice();
	mDuel->mLuaCallbackSuspended = false;
	delete temporaryChoice;

	// A required card choice has no negative/button result. Rejecting this is
	// what prevents an AI or stale UI action from skipping a mandatory cost.
	mDuel->addChoice("Required creature", 0, 0, 0, LUA_REFNIL, LUA_REFNIL);
	mDuel->mChoiceValidCards.push_back(0);
	Message illegalSkip("choiceselect");
	illegalSkip.addValue("selection", RETURN_BUTTON1);
	mDuel->handleInterfaceInput(illegalSkip);
	bool requiredChoiceStayedActive = mDuel->mIsChoiceActive && mDuel->mChoice != NULL;
	temporaryChoice = mDuel->mChoice;
	mDuel->mChoice = NULL;
	mDuel->resetChoice();
	delete temporaryChoice;
	return hasYes && hasNo && actions.size() == 2 && requiredChoiceStayedActive;
}

bool Application::exerciseActionLabelSmoke()
{
	if (mDuel == NULL || mDuel->mCardList.size() < 2 || mDuel->mShields[0].mCards.empty() ||
		mDuel->mDecks[0].mCards.empty()) return false;
	std::lock_guard<std::mutex> lock(gMutex);

	Card* shield = mDuel->mShields[0].mCards.front();
	bool savedShieldVisibility = shield->mIsVisible[0];
	Message shieldChoice("choiceselect");
	shieldChoice.addValue("selection", shield->mUniqueId);
	shield->mIsVisible[0] = false;
	std::string hiddenShieldLabel = actionLabel(shieldChoice);
	shield->mIsVisible[0] = true;
	std::string visibleShieldLabel = actionLabel(shieldChoice);
	shield->mIsVisible[0] = savedShieldVisibility;
	Card* deckCard = mDuel->mDecks[0].mCards.front();
	bool savedDeckVisibility = deckCard->mIsVisible[0];
	Message deckChoice("choiceselect");
	deckChoice.addValue("selection", deckCard->mUniqueId);
	deckCard->mIsVisible[0] = false;
	std::string deckChoiceLabel = actionLabel(deckChoice);
	deckCard->mIsVisible[0] = savedDeckVisibility;

	Card* attacker = mDuel->mCardList[0];
	Card* defender = mDuel->mCardList[1];
	bool savedAttackerVisibility = attacker->mIsVisible[0];
	bool savedDefenderVisibility = defender->mIsVisible[0];
	attacker->mIsVisible[0] = true;
	defender->mIsVisible[0] = true;

	Message creatureAttack("creatureattack");
	creatureAttack.addValue("attacker", attacker->mUniqueId);
	creatureAttack.addValue("defender", defender->mUniqueId);
	creatureAttack.addValue("defendertype", DEFENDER_CREATURE);
	std::string creatureAttackLabel = actionLabel(creatureAttack);

	Message playerAttack("creatureattack");
	playerAttack.addValue("attacker", attacker->mUniqueId);
	playerAttack.addValue("defender", 0);
	playerAttack.addValue("defendertype", DEFENDER_PLAYER);
	std::string playerAttackLabel = actionLabel(playerAttack);

	attacker->mIsVisible[0] = savedAttackerVisibility;
	defender->mIsVisible[0] = savedDefenderVisibility;

	return hiddenShieldLabel == "Choose hidden card" &&
		visibleShieldLabel == "Choose " + shield->mName &&
		deckChoiceLabel == "Choose " + deckCard->mName &&
		creatureAttackLabel == "Attack " + defender->mName + " with " + attacker->mName &&
		playerAttackLabel == "Attack rival with " + attacker->mName;
}

bool Application::exerciseHeuristicAttackSafetySmoke()
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->mBattlezones[1].mCards.empty() || mDuel->mBattlezones[0].mCards.empty()) return false;

	Card* attacker = mDuel->mBattlezones[1].mCards.front();
	Card* defender = mDuel->mBattlezones[0].mCards.front();
	int savedAttackerPower = attacker->mPower;
	int savedDefenderPower = defender->mPower;
	bool savedBlocker = defender->mIsBlocker;
	bool savedTapped = defender->mIsTapped;
	attacker->mPower = 1000;
	defender->mPower = 100000;
	defender->mIsTapped = false;

	Message attackCreature("creatureattack");
	attackCreature.addValue("attacker", attacker->mUniqueId);
	attackCreature.addValue("defender", defender->mUniqueId);
	attackCreature.addValue("defendertype", DEFENDER_CREATURE);
	HeuristicBot rival(1);
	double strongerCreatureScore = rival.scoreMove(*mDuel, attackCreature);

	defender->mIsBlocker = true;
	Message attackPlayer("creatureattack");
	attackPlayer.addValue("attacker", attacker->mUniqueId);
	attackPlayer.addValue("defender", 0);
	attackPlayer.addValue("defendertype", DEFENDER_PLAYER);
	double strongerBlockerScore = rival.scoreMove(*mDuel, attackPlayer);
	Message endTurn("endturn");
	std::vector<Message> choices;
	choices.push_back(attackCreature);
	choices.push_back(attackPlayer);
	choices.push_back(endTurn);
	Message selected = rival.chooseMove(*mDuel, choices);
	choices.pop_back();
	Message forcedSelection = rival.chooseMove(*mDuel, choices);

	attacker->mPower = savedAttackerPower;
	defender->mPower = savedDefenderPower;
	defender->mIsBlocker = savedBlocker;
	defender->mIsTapped = savedTapped;
	return std::isinf(strongerCreatureScore) && strongerCreatureScore < 0.0 &&
		std::isinf(strongerBlockerScore) && strongerBlockerScore < 0.0 &&
		selected.getType() == "endturn" && forcedSelection.getType() == "creatureattack";
}

bool Application::exerciseHeuristicBlockChoiceSmoke()
{
	if (mDuel == NULL || mDuel->mCardList.size() < 3) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	Card* attacker = mDuel->mCardList[0];
	Card* lowerWinningBlocker = mDuel->mCardList[1];
	Card* higherWinningBlocker = mDuel->mCardList[2];
	int savedAttacker = mDuel->mAttacker;
	int savedAttackerPower = attacker->mPower;
	int savedLowerPower = lowerWinningBlocker->mPower;
	int savedHigherPower = higherWinningBlocker->mPower;

	mDuel->mAttacker = attacker->mUniqueId;
	attacker->mPower = 1000;
	lowerWinningBlocker->mPower = 10000;
	higherWinningBlocker->mPower = 100000;
	Message higherBlock("creatureblock");
	higherBlock.addValue("blocker", higherWinningBlocker->mUniqueId);
	Message lowerBlock("creatureblock");
	lowerBlock.addValue("blocker", lowerWinningBlocker->mUniqueId);
	Message skip("blockskip");
	std::vector<Message> choices;
	choices.push_back(higherBlock);
	choices.push_back(lowerBlock);
	choices.push_back(skip);
	HeuristicBot rival(1);
	Message selected = rival.chooseMove(*mDuel, choices);

	mDuel->mAttacker = savedAttacker;
	attacker->mPower = savedAttackerPower;
	lowerWinningBlocker->mPower = savedLowerPower;
	higherWinningBlocker->mPower = savedHigherPower;
	return selected.getType() == "creatureblock" &&
		messageInt(selected, "blocker") == lowerWinningBlocker->mUniqueId;
}

bool Application::exerciseHeuristicManaConservationSmoke()
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	std::vector<Card*> ownedCards;
	int maximumDeckCost = 0;
	for (size_t i = 0; i < mDuel->mCardList.size(); ++i)
	{
		Card* card = mDuel->mCardList[i];
		if (card->mOwner != 1) continue;
		ownedCards.push_back(card);
		maximumDeckCost = std::max(maximumDeckCost, card->mManaCost);
	}
	if (ownedCards.size() < 7 || maximumDeckCost <= 0 ||
		maximumDeckCost > (int)ownedCards.size()) return false;

	std::vector<Card*> savedMana = mDuel->mManazones[1].mCards;
	std::vector<Card*> savedHand = mDuel->mHands[1].mCards;
	int savedTurn = mDuel->mTurn;
	int savedAttackPhase = mDuel->mAttackphase;
	int savedCastingCard = mDuel->mCastingCard;
	bool savedChoiceActive = mDuel->mIsChoiceActive;

	Card* candidate = ownedCards.front();
	Message charge("cardmana");
	charge.addValue("card", candidate->mUniqueId);
	Message endTurn("endturn");
	HeuristicBot rival(1);

	mDuel->mManazones[1].mCards.assign(ownedCards.begin(), ownedCards.begin() +
		std::min(3, std::max(0, maximumDeckCost - 1)));
	mDuel->mHands[1].mCards.assign(ownedCards.begin(), ownedCards.begin() + 7);
	double fullHandScore = rival.scoreMove(*mDuel, charge);
	mDuel->mHands[1].mCards.assign(ownedCards.begin(), ownedCards.begin() + 2);
	double scarceHandScore = rival.scoreMove(*mDuel, charge);

	mDuel->mManazones[1].mCards.assign(ownedCards.begin(),
		ownedCards.begin() + maximumDeckCost);
	double cappedManaScore = rival.scoreMove(*mDuel, charge);
	mDuel->mTurn = 1;
	mDuel->mAttackphase = PHASE_NONE;
	mDuel->mCastingCard = -1;
	mDuel->mIsChoiceActive = false;
	std::vector<Message> moves;
	moves.push_back(charge);
	moves.push_back(endTurn);
	Message selected = rival.chooseMove(*mDuel, moves);

	mDuel->mManazones[1].mCards = savedMana;
	mDuel->mHands[1].mCards = savedHand;
	mDuel->mTurn = savedTurn;
	mDuel->mAttackphase = savedAttackPhase;
	mDuel->mCastingCard = savedCastingCard;
	mDuel->mIsChoiceActive = savedChoiceActive;
	return fullHandScore > scarceHandScore + 30.0 && cappedManaScore < 5.0 &&
		selected.getType() == "endturn";
}

bool Application::beginMandatorySacrificeAiSmoke(
	const std::string& cardName, int& summonedCard, int& sacrifice)
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->mIsChoiceActive || mDuel->mChoice != NULL ||
		mDuel->mLuaCallbackSuspended || mDuel->mMsgMngr.hasMoreMessages())
		return false;

	summonedCard = -1;
	sacrifice = -1;
	for (size_t i = 0; i < mDuel->mCardList.size(); ++i)
	{
		Card* card = mDuel->mCardList[i];
		if (card->mOwner != 1) continue;
		if (card->mName == cardName && summonedCard < 0)
			summonedCard = card->mUniqueId;
		else if (card->mType == TYPE_CREATURE && sacrifice < 0)
			sacrifice = card->mUniqueId;
	}
	if (summonedCard < 0 || sacrifice < 0) return false;

	Card* sacrificeCard = mDuel->mCardList[sacrifice];
	mDuel->getZone(1, sacrificeCard->mZone)->removeCard(sacrificeCard);
	mDuel->mBattlezones[1].addCard(sacrificeCard);
	sacrificeCard->mZone = ZONE_BATTLE;

	Card* summoned = mDuel->mCardList[summonedCard];
	Message summon("cardmove");
	summon.addValue("card", summonedCard);
	summon.addValue("from", summoned->mZone);
	summon.addValue("to", ZONE_BATTLE);
	summon.addValue("evobait", -1);
	mDuel->mMsgMngr.sendMessage(summon);
	// Hold the AI briefly so the smoke test can assert that the summoned card
	// itself is one of the mandatory legal targets before resolving it.
	mNextAiMove = SDL_GetTicks() + 60000;
	return true;
}

bool Application::verifyMandatorySacrificeAiSmoke(
	const std::string& cardName, int summonedCard, int sacrifice)
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (summonedCard < 0 || sacrifice < 0 ||
		summonedCard >= (int)mDuel->mCardList.size() || sacrifice >= (int)mDuel->mCardList.size())
		return false;
	int summonedZone = mDuel->mCardList[summonedCard]->mZone;
	int sacrificeZone = mDuel->mCardList[sacrifice]->mZone;
	bool exactlyOneWasDestroyed =
		(summonedZone == ZONE_GRAVEYARD && sacrificeZone == ZONE_BATTLE) ||
		(summonedZone == ZONE_BATTLE && sacrificeZone == ZONE_GRAVEYARD);
	bool passed = !mDuel->mIsChoiceActive && !mDuel->mLuaCallbackSuspended && exactlyOneWasDestroyed;
	if (!passed)
	{
		std::cerr << cardName << " state: choice=" << mDuel->mIsChoiceActive
			<< " suspended=" << mDuel->mLuaCallbackSuspended
			<< " summoned-zone=" << mDuel->mCardList[summonedCard]->mZone
			<< " sacrifice-zone=" << mDuel->mCardList[sacrifice]->mZone
			<< " queued=" << mDuel->mMsgMngr.hasMoreMessages() << std::endl;
	}
	return passed;
}

bool Application::exerciseGraveyardBrowserSmoke()
{
	if (mDuel == NULL) return false;
	{
		std::lock_guard<std::mutex> lock(gMutex);
		if (mDuel->mGraveyards[1].mCards.empty()) return false;
	}

	SDL_Rect pile = graveyardPileRect(1);
	SDL_Event open = {};
	open.type = SDL_MOUSEBUTTONDOWN;
	open.button.button = SDL_BUTTON_LEFT;
	open.button.x = pile.x + pile.w / 2;
	open.button.y = pile.y + pile.h / 2;
	handleDuelEvent(open);
	if (mOpenGraveyardPlayer != 1) return false;

	{
		std::lock_guard<std::mutex> lock(gMutex);
		renderGraveyardOverlay();
	}
	CardHitbox graveyardCard = { { 0, 0, 0, 0 }, -1, true, true, false };
	for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
	{
		std::lock_guard<std::mutex> lock(gMutex);
		if (item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size() &&
			mDuel->mCardList[item->cardId]->mOwner == 1 &&
			mDuel->mCardList[item->cardId]->mZone == ZONE_GRAVEYARD)
		{
			graveyardCard = *item;
			break;
		}
	}
	if (graveyardCard.cardId < 0) return false;

	SDL_Event select = {};
	select.type = SDL_MOUSEBUTTONDOWN;
	select.button.button = SDL_BUTTON_LEFT;
	select.button.x = graveyardCard.rect.x + graveyardCard.rect.w / 2;
	select.button.y = graveyardCard.rect.y + graveyardCard.rect.h / 2;
	handleDuelEvent(select);
	bool selected = mSelectedCard == graveyardCard.cardId;
	SDL_Event close = {};
	close.type = SDL_KEYDOWN;
	close.key.keysym.sym = SDLK_ESCAPE;
	handleDuelEvent(close);
	mSelectedCard = -1;
	return selected && mOpenGraveyardPlayer == -1;
}

void Application::playCard(const Message& message)
{
	if (mDuel == NULL || mDuelResult != -1) return;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->getPlayerToMove() != 0) return;

	Message action = message;
	mDuel->handleInterfaceInput(action);
	std::set<int> selectedMana;
	for (int safety = 0; mDuel->mCastingCard != -1 && safety < 20; ++safety)
	{
		std::vector<Message> moves = mDuel->getPossibleMoves();
		bool paid = false;
		for (size_t i = 0; i < moves.size(); ++i)
		{
			if (moves[i].getType() != "manatap") continue;
			int mana = messageInt(moves[i], "card");
			if (selectedMana.find(mana) != selectedMana.end()) continue;
			selectedMana.insert(mana);
			Message payment = moves[i];
			mDuel->handleInterfaceInput(payment);
			paid = true;
			break;
		}
		if (!paid) break;
	}
	mSelectedCard = -1;
	mActionScroll = 0;
}

void Application::beginDrag(int cardId, const SDL_Rect& origin, int mouseX, int mouseY)
{
	if (mDuel == NULL) return;
	std::lock_guard<std::mutex> lock(gMutex);
	if (cardId < 0 || cardId >= (int)mDuel->mCardList.size()) return;
	Card* card = mDuel->mCardList[cardId];
	if (card->mOwner != 0 || (card->mZone != ZONE_HAND && card->mZone != ZONE_BATTLE)) return;
	bool legalDrag = false;
	std::vector<Message> moves = mDuel->getPossibleMoves();
	for (size_t i = 0; i < moves.size(); ++i)
	{
		const std::string type = moves[i].getType();
		if (card->mZone == ZONE_HAND &&
			(type == "cardplay" || type == "cardmana") && messageInt(moves[i], "card") == cardId)
			legalDrag = true;
		else if (card->mZone == ZONE_BATTLE && type == "creatureattack" &&
			messageInt(moves[i], "attacker") == cardId)
			legalDrag = true;
		if (legalDrag) break;
	}
	if (!legalDrag) return;
	mDraggingCard = cardId;
	mDragFromZone = card->mZone;
	mDragOrigin = origin;
	mDragMouseX = mouseX;
	mDragMouseY = mouseY;
	mSelectedCard = cardId;
	mHoveredCard = -1;
	mHoverCandidateCard = -1;
	mHoverCandidateSince = 0;
}

void Application::cancelDrag()
{
	mDraggingCard = -1;
	mDragFromZone = -1;
	mDragOrigin = { 0, 0, 0, 0 };
}

bool Application::findDragAction(const std::string& type, int cardId, int targetId, Message& result)
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	std::vector<Message> moves = mDuel->getPossibleMoves();
	for (size_t i = 0; i < moves.size(); ++i)
	{
		if (moves[i].getType() != type) continue;
		if (type == "cardplay" || type == "cardmana")
		{
			if (messageInt(moves[i], "card") != cardId) continue;
			if (type == "cardplay" && targetId >= 0 && messageInt(moves[i], "evobait") != targetId) continue;
			if (type == "cardplay" && targetId < 0 && messageInt(moves[i], "evobait") >= 0) continue;
		}
		else if (type == "creatureattack")
		{
			if (messageInt(moves[i], "attacker") != cardId) continue;
			if (targetId >= 0)
			{
				if (messageInt(moves[i], "defendertype") != DEFENDER_CREATURE ||
					messageInt(moves[i], "defender") != targetId) continue;
			}
			else if (messageInt(moves[i], "defendertype") != DEFENDER_PLAYER) continue;
		}
		else if (type == "creatureusetapability" && messageInt(moves[i], "creature") != cardId)
		{
			continue;
		}
		result = moves[i];
		return true;
	}
	return false;
}

bool Application::findClickAction(int cardId, Message& result)
{
	if (mDuel == NULL || mDuelResult != -1) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->getPlayerToMove() != 0) return false;

	std::vector<Message> moves = mDuel->getPossibleMoves();
	for (size_t i = 0; i < moves.size(); ++i)
	{
		const std::string type = moves[i].getType();
		const char* key = NULL;
		if (type == "choiceselect") key = "selection";
		else if (type == "manatap") key = "card";
		else if (type == "targetshield") key = "shield";
		else if (type == "triggeruse") key = "trigger";
		else if (type == "creatureblock") key = "blocker";
		if (key != NULL && messageInt(moves[i], key) == cardId)
		{
			result = moves[i];
			return true;
		}
	}
	return false;
}

void Application::finishDrag(int mouseX, int mouseY)
{
	int cardId = mDraggingCard;
	int sourceZone = mDragFromZone;
	Message action;
	bool found = false;
	bool isCardPlay = false;

	if (sourceZone == ZONE_HAND)
	{
		const SDL_Rect manaDrop = { 500, 535, 455, 125 };
		const SDL_Rect battleDrop = { 55, 330, 890, 190 };
		if (contains(manaDrop, mouseX, mouseY))
			found = findDragAction("cardmana", cardId, -1, action);
		else if (contains(battleDrop, mouseX, mouseY))
		{
			int evolutionTarget = -1;
			for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
			{
				if (!contains(item->rect, mouseX, mouseY)) continue;
				std::lock_guard<std::mutex> lock(gMutex);
				if (mDuel != NULL && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size())
				{
					Card* target = mDuel->mCardList[item->cardId];
					if (target->mOwner == 0 && target->mZone == ZONE_BATTLE) evolutionTarget = item->cardId;
				}
				break;
			}
			found = findDragAction("cardplay", cardId, evolutionTarget, action);
			if (!found && evolutionTarget >= 0) found = findDragAction("cardplay", cardId, -1, action);
			isCardPlay = found;
		}
	}
	else if (sourceZone == ZONE_BATTLE)
	{
		int defender = -1;
		for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
		{
			if (!contains(item->rect, mouseX, mouseY)) continue;
			std::lock_guard<std::mutex> lock(gMutex);
			if (mDuel != NULL && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size())
			{
				Card* target = mDuel->mCardList[item->cardId];
				if (target->mOwner == 1 && target->mZone == ZONE_BATTLE) defender = item->cardId;
			}
			break;
		}
		if (defender >= 0)
			found = findDragAction("creatureattack", cardId, defender, action);
		else if (contains({ 150, 0, 760, 200 }, mouseX, mouseY))
			found = findDragAction("creatureattack", cardId, -1, action);
	}

	cancelDrag();
	if (found)
	{
		if (isCardPlay) playCard(action);
		else playAction(action);
	}
}

bool Application::messageReferencesCard(const Message& message, int cardId) const
{
	const char* keys[] = { "card", "creature", "attacker", "defender", "blocker", "trigger", "shield", "selection", "evobait" };
	for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
	{
		std::map<std::string, std::string>::const_iterator found = message.map.find(keys[i]);
		if (found != message.map.end() && std::atoi(found->second.c_str()) == cardId) return true;
	}
	return false;
}

std::vector<Message> Application::visibleActions()
{
	std::vector<Message> all = mDuel->getPossibleMoves();
	if (mDuel->mIsChoiceActive) return all;
	if (mSelectedCard < 0) return all;
	std::vector<Message> filtered;
	for (size_t i = 0; i < all.size(); ++i)
		if (messageReferencesCard(all[i], mSelectedCard)) filtered.push_back(all[i]);
	return filtered.empty() ? all : filtered;
}

std::string Application::actionLabel(const Message& message) const
{
	std::string type = message.map.find("msgtype") == message.map.end() ? "action" : message.map.find("msgtype")->second;
	auto cardName = [this](int uid) -> std::string
	{
		if (mDuel != NULL && uid >= 0 && uid < (int)mDuel->mCardList.size())
		{
			Card* card = mDuel->mCardList[uid];
			return card->mIsVisible[0] ? card->mName : "hidden card";
		}
		return "card";
	};
	if (type == "endturn") return "End turn";
	if (type == "cardmana") return "Charge " + cardName(messageInt(message, "card"));
	if (type == "cardplay") return "Cast " + cardName(messageInt(message, "card"));
	if (type == "manatap") return "Tap mana: " + cardName(messageInt(message, "card"));
	if (type == "creatureusetapability") return "Use tap ability: " + cardName(messageInt(message, "creature"));
	if (type == "creatureattack")
	{
		int defenderType = messageInt(message, "defendertype");
		std::string defender = defenderType == DEFENDER_PLAYER ? "rival" : cardName(messageInt(message, "defender"));
		return "Attack " + defender + " with " + cardName(messageInt(message, "attacker"));
	}
	if (type == "creatureblock") return "Block with " + cardName(messageInt(message, "blocker"));
	if (type == "blockskip") return "Do not block";
	if (type == "targetshield") return "Break selected shield";
	if (type == "triggeruse") return "Use trigger: " + cardName(messageInt(message, "trigger"));
	if (type == "triggerskip") return "Skip shield triggers";
	if (type == "choiceselect")
	{
		int selection = messageInt(message, "selection");
		if (selection == RETURN_BUTTON1)
			return mDuel->mChoice != NULL && mDuel->mChoice->mButtonCount >= 2 ? "Yes" : "Continue";
		if (selection == RETURN_BUTTON2) return "No";
		if (selection < 0) return "Choose option";
		if (mDuel != NULL && selection < static_cast<int>(mDuel->mCardList.size()) &&
			mDuel->mCardList[selection]->mZone == ZONE_DECK)
			return "Choose " + mDuel->mCardList[selection]->mName;
		return "Choose " + cardName(selection);
	}
	return type;
}

std::string Application::actionLogLabel(const Message& message, int player) const
{
	std::map<std::string, std::string>::const_iterator typeValue = message.map.find("msgtype");
	const std::string type = typeValue == message.map.end() ? "" : typeValue->second;
	auto cardName = [this](int uid) -> std::string
	{
		if (mDuel == NULL || uid < 0 || uid >= (int)mDuel->mCardList.size()) return "card";
		Card* card = mDuel->mCardList[uid];
		return card->mOwner == 0 || card->mIsVisible[0] ? card->mName : "hidden card";
	};
	if (type == "cardmana") return "charged " + cardName(messageInt(message, "card")) + " as mana";
	if (type == "cardplay")
	{
		int cardId = messageInt(message, "card");
		bool creature = mDuel != NULL && cardId >= 0 && cardId < (int)mDuel->mCardList.size() &&
			mDuel->mCardList[cardId]->mType == TYPE_CREATURE;
		return std::string(creature ? "summoned " : "cast ") + cardName(cardId);
	}
	if (type == "creatureusetapability")
		return "used the tap ability of " + cardName(messageInt(message, "creature"));
	if (type == "creatureattack")
	{
		std::string target = messageInt(message, "defendertype") == DEFENDER_PLAYER ?
			(player == 0 ? "the rival" : "you") : cardName(messageInt(message, "defender"));
		return "attacked " + target + " with " + cardName(messageInt(message, "attacker"));
	}
	if (type == "creatureblock") return "blocked with " + cardName(messageInt(message, "blocker"));
	if (type == "blockskip") return "declined to block";
	if (type == "targetshield") return "selected a shield to break";
	if (type == "triggeruse") return "used shield trigger " + cardName(messageInt(message, "trigger"));
	if (type == "triggerskip") return "declined remaining shield triggers";
	if (type == "choiceselect")
	{
		int selection = messageInt(message, "selection");
		return selection >= 0 ? "chose " + cardName(selection) : "made a choice";
	}
	if (type == "endturn") return "ended the turn";
	return "";
}

void Application::renderAttackIndicator()
{
	if (mDuel == NULL || mDuel->mAttackphase == PHASE_NONE ||
		mDuel->mAttacker < 0 || mDuel->mAttacker >= (int)mDuel->mCardList.size()) return;
	auto cardRect = [this](int uid, SDL_Rect& result) -> bool
	{
		for (std::vector<CardHitbox>::const_reverse_iterator hitbox = mCardHitboxes.rbegin();
			hitbox != mCardHitboxes.rend(); ++hitbox)
		{
			if (hitbox->cardId != uid || !hitbox->hoverAnchor) continue;
			result = hitbox->rect;
			return true;
		}
		return false;
	};
	SDL_Rect attacker;
	if (!cardRect(mDuel->mAttacker, attacker)) return;
	SDL_Rect target;
	if (mDuel->mDefenderType == DEFENDER_CREATURE)
	{
		if (!cardRect(mDuel->mDefender, target)) return;
	}
	else if (mDuel->mDefenderType == DEFENDER_PLAYER)
	{
		target = mDuel->mDefender == 0 ? SDL_Rect{ 12, 738, 944, 55 } : SDL_Rect{ 12, 4, 944, 55 };
	}
	else return;

	int startX = attacker.x + attacker.w / 2;
	int startY = attacker.y + attacker.h / 2;
	int endX = target.x + target.w / 2;
	int endY = target.y + target.h / 2;
	float dx = (float)(endX - startX);
	float dy = (float)(endY - startY);
	float length = std::sqrt(dx * dx + dy * dy);
	if (length > 0.1f)
	{
		float ux = dx / length;
		float uy = dy / length;
		setColor(244, 76, 63, 235);
		for (int offset = -2; offset <= 2; ++offset)
			SDL_RenderDrawLine(mRenderer, startX + offset, startY, endX + offset, endY);
		int baseX = (int)std::round(endX - ux * 20.f);
		int baseY = (int)std::round(endY - uy * 20.f);
		int sideX = (int)std::round(-uy * 9.f);
		int sideY = (int)std::round(ux * 9.f);
		SDL_RenderDrawLine(mRenderer, endX, endY, baseX + sideX, baseY + sideY);
		SDL_RenderDrawLine(mRenderer, endX, endY, baseX - sideX, baseY - sideY);
	}
	outlineRect(attacker, 244, 76, 63, 255, 5);
	outlineRect(target, 255, 211, 83, 255, 5);
	SDL_Rect attackerBadge = { attacker.x, std::max(2, attacker.y - 23), 82, 21 };
	fillRect(attackerBadge, 110, 25, 24, 245);
	drawText("ATTACKER", attackerBadge.x + 7, attackerBadge.y + 3, color(255, 226, 218), 11);
	SDL_Rect targetBadge = { target.x, target.y > 80 ? target.y - 23 : target.y + target.h + 2, 72, 21 };
	fillRect(targetBadge, 92, 70, 20, 245);
	drawText("TARGET", targetBadge.x + 10, targetBadge.y + 3, color(255, 239, 181), 11);
}

void Application::renderActionLogOverlay()
{
	std::vector<std::string> lines;
	if (mDuel != NULL)
	{
		size_t count = std::min(mDuel->mMoveHistory.size(), mDuel->mMovePlayers.size());
		for (size_t i = 0; i < count; ++i)
		{
			std::string detail = actionLogLabel(mDuel->mMoveHistory[i], mDuel->mMovePlayers[i]);
			if (detail.empty()) continue;
			std::string actor = mDuel->mMovePlayers[i] == 0 ? "YOU" : "RIVAL";
			lines.push_back(actor + ": " + detail);
		}
	}
	int maxScroll = std::max(0, (int)lines.size() - ACTION_LOG_PAGE_SIZE);
	mActionLogScroll = std::max(0, std::min(maxScroll, mActionLogScroll));
	int first = std::max(0, (int)lines.size() - ACTION_LOG_PAGE_SIZE - mActionLogScroll);
	int last = std::min((int)lines.size(), first + ACTION_LOG_PAGE_SIZE);

	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 3, 6, 11, 190);
	fillRect(ACTION_LOG_OVERLAY, 17, 24, 38, 252);
	outlineRect(ACTION_LOG_OVERLAY, 190, 145, 62, 255, 4);
	drawText("DUEL ACTION LOG", 145, 101, color(245, 211, 126), 27);
	drawText("Newest actions are at the bottom. Use the mouse wheel to review earlier turns.",
		145, 130, color(181, 197, 221), 13, 850);
	fillRect(ACTION_LOG_CLOSE, 74, 43, 43, 245);
	outlineRect(ACTION_LOG_CLOSE, 220, 116, 99, 255, 2);
	drawText("X", ACTION_LOG_CLOSE.x + 14, ACTION_LOG_CLOSE.y + 7, color(250, 230, 225), 17);
	if (lines.empty())
		drawText("No actions have been played yet.", 390, 365, color(178, 190, 210), 18);
	for (int i = first; i < last; ++i)
	{
		int row = i - first;
		SDL_Rect background = { 143, 165 + row * 29, 994, 25 };
		if (row % 2 == 0) fillRect(background, 28, 38, 57, 205);
		drawText(std::to_string(i + 1) + ". " + lines[i], background.x + 8,
			background.y + 4, color(230, 235, 244), 13, background.w - 16);
	}
	if (!lines.empty())
		drawText(std::to_string(first + 1) + "-" + std::to_string(last) + " of " +
			std::to_string(lines.size()), 525, 680, color(183, 199, 222), 13);
}

SDL_Rect Application::graveyardPileRect(int player) const
{
	return { 914, player == 1 ? 86 : 551, 54, 76 };
}

SDL_Rect Application::deckPileRect(int player) const
{
	return { 8, player == 1 ? 82 : 551, 54, 76 };
}

void Application::renderDeckPile(int player)
{
	SDL_Rect pile = deckPileRect(player);
	const std::vector<Card*>& cards = mDuel->mDecks[player].mCards;
	drawText(player == 1 ? "RIVAL DECK" : "YOUR DECK", 5, player == 1 ? 62 : 632,
		color(180, 198, 224), 10, 60);
	for (size_t i = 0; i < cards.size(); ++i)
	{
		AnimatedCard& animation = mCardAnimations[cards[i]->mUniqueId];
		animation.targetX = (float)pile.x;
		animation.targetY = (float)pile.y;
		animation.targetWidth = (float)pile.w;
		animation.targetHeight = (float)pile.h;
		animation.targetAngle = 0.f;
		if (!animation.initialized)
		{
			animation.x = animation.targetX;
			animation.y = animation.targetY;
			animation.width = animation.targetWidth;
			animation.height = animation.targetHeight;
			animation.angle = 0.f;
			animation.initialized = true;
		}
	}
	if (cards.empty())
	{
		fillRect(pile, 16, 23, 35, 190);
		outlineRect(pile, 111, 123, 143, 230, 2);
		drawText("EMPTY", pile.x + 7, pile.y + 30, color(171, 181, 198), 10);
		return;
	}
	if (cards.size() >= 3)
	{
		SDL_Rect backLayer = { pile.x + 5, pile.y + 5, pile.w, pile.h };
		drawCardBack(backLayer);
	}
	if (cards.size() >= 2)
	{
		SDL_Rect middleLayer = { pile.x + 2, pile.y + 2, pile.w, pile.h };
		drawCardBack(middleLayer);
	}
	drawCardBack(pile);
	SDL_Rect count = { pile.x + pile.w - 22, pile.y + pile.h - 20, 22, 20 };
	fillRect(count, 12, 18, 29, 235);
	drawText(std::to_string(cards.size()), count.x + 4, count.y + 2,
		color(244, 225, 171), 11);
}

void Application::renderGraveyardPile(int player)
{
	SDL_Rect pile = graveyardPileRect(player);
	const std::vector<Card*>& cards = mDuel->mGraveyards[player].mCards;
	drawText(player == 1 ? "RIVAL GY" : "YOUR GY", 908, pile.y - 20,
		color(180, 198, 224), 11, 64);
	if (cards.empty())
	{
		fillRect(pile, 20, 27, 40, 175);
		outlineRect(pile, 111, 123, 143, 230, 2);
		drawText("0", pile.x + 22, pile.y + 29, color(171, 181, 198), 13);
		return;
	}

	if (cards.size() >= 3)
	{
		fillRect({ pile.x + 5, pile.y + 5, pile.w, pile.h }, 12, 17, 27, 210);
		outlineRect({ pile.x + 5, pile.y + 5, pile.w, pile.h }, 91, 102, 121, 230, 2);
	}
	if (cards.size() >= 2)
	{
		fillRect({ pile.x + 2, pile.y + 2, pile.w, pile.h }, 18, 24, 36, 230);
		outlineRect({ pile.x + 2, pile.y + 2, pile.w, pile.h }, 121, 132, 151, 240, 2);
	}
	drawCard(cards.back(), pile, true, cards.back()->mUniqueId == mSelectedCard, true);
	SDL_Rect count = { pile.x + pile.w - 22, pile.y + pile.h - 20, 22, 20 };
	fillRect(count, 12, 18, 29, 235);
	drawText(std::to_string(cards.size()), count.x + 4, count.y + 2, color(244, 225, 171), 11);
}

void Application::renderGraveyardOverlay()
{
	if (mOpenGraveyardPlayer < 0 || mOpenGraveyardPlayer > 1) return;
	const std::vector<Card*>& cards = mDuel->mGraveyards[mOpenGraveyardPlayer].mCards;
	mGraveyardOffset = std::max(0, std::min(std::max(0, (int)cards.size() - 1), mGraveyardOffset));

	// The graveyard browser is modal: only its visible cards should receive
	// clicks while it is open.
	mCardHitboxes.clear();
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 5, 8, 14, 185);
	fillRect(GRAVEYARD_OVERLAY, 18, 25, 39, 250);
	outlineRect(GRAVEYARD_OVERLAY, 188, 145, 64, 255, 4);
	drawText(mOpenGraveyardPlayer == 0 ? "YOUR GRAVEYARD" : "RIVAL GRAVEYARD",
		135, 170, color(245, 211, 126), 25);
	drawText("Newest cards are shown first. Click a card to select it.",
		135, 208, color(188, 202, 224), 14);
	fillRect(GRAVEYARD_CLOSE, 74, 43, 43, 245);
	outlineRect(GRAVEYARD_CLOSE, 220, 116, 99, 255, 2);
	drawText("X", GRAVEYARD_CLOSE.x + 14, GRAVEYARD_CLOSE.y + 7, color(250, 230, 225), 17);

	int shown = std::min(GRAVEYARD_PAGE_SIZE, std::max(0, (int)cards.size() - mGraveyardOffset));
	for (int i = 0; i < shown; ++i)
	{
		int cardIndex = (int)cards.size() - 1 - (mGraveyardOffset + i);
		Card* card = cards[cardIndex];
		SDL_Rect rect = { 135 + i * 143, 260, 116, 162 };
		drawCard(card, rect, true, card->mUniqueId == mSelectedCard, true);
		drawText(card->mName, rect.x, 435, color(226, 232, 242), 12, rect.w);
	}
	if (cards.empty())
		drawText("This graveyard is empty.", 355, 355, color(178, 190, 210), 18);

	fillRect(GRAVEYARD_PREVIOUS, 31, 45, 68, 245);
	outlineRect(GRAVEYARD_PREVIOUS, 112, 146, 196, 255, 2);
	drawText("< NEWER", GRAVEYARD_PREVIOUS.x + 25, GRAVEYARD_PREVIOUS.y + 11,
		color(232, 237, 246), 14);
	fillRect(GRAVEYARD_NEXT, 31, 45, 68, 245);
	outlineRect(GRAVEYARD_NEXT, 112, 146, 196, 255, 2);
	drawText("OLDER >", GRAVEYARD_NEXT.x + 31, GRAVEYARD_NEXT.y + 11,
		color(232, 237, 246), 14);
	if (!cards.empty())
	{
		int first = mGraveyardOffset + 1;
		int last = mGraveyardOffset + shown;
		drawText(std::to_string(first) + "-" + std::to_string(last) + " of " + std::to_string(cards.size()),
		445, 602, color(190, 202, 221), 13);
	}
}

void Application::renderDuel()
{
	if (mBoardTexture != NULL)
		SDL_RenderCopy(mRenderer, mBoardTexture, NULL, NULL);
	else
		fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 75, 45, 31);
	fillRect({ 980, 0, 300, 800 }, 13, 19, 31, 238);
	outlineRect({ 980, 0, 300, 800 }, 181, 137, 56, 255, 3);

	int hoverCandidate = -1;
	bool immediateHover = false;
	if (mDraggingCard < 0)
		hoverCandidate = duelHoverCandidateAt(mMouseX, mMouseY, immediateHover);
	updateHoverState(hoverCandidate, immediateHover, SDL_GetTicks());
	mCardHitboxes.clear();
	mActionButtons.clear();
	if (mDuel == NULL) return;

	std::lock_guard<std::mutex> lock(gMutex);
	std::string rivalName = mActiveNpc >= 0 ? mNpcs[mActiveNpc].name :
		"AI: " + deckDisplayName(mDuel->mDeckNames[1]);
	drawText(rivalName, 18, 14, color(244, 205, 99), 20, 900);
	drawText("Hand " + std::to_string(mDuel->mHands[1].mCards.size()), 18, 39, color(229, 235, 245), 13);
	renderDeckPile(1);
	drawHand(mDuel->mHands[1].mCards, true);

	drawZone(mDuel->mManazones[1].mCards, 70, 82, 390, 54, 76, true, true);
	drawZone(mDuel->mShields[1].mCards, 510, 82, 390, 54, 76, false, true);
	renderGraveyardPile(1);
	drawZone(mDuel->mBattlezones[1].mCards, 65, 205, 850, 82, 114, true, true);

	drawZone(mDuel->mBattlezones[0].mCards, 65, 390, 850, 82, 114, true, true);
	drawZone(mDuel->mShields[0].mCards, 70, 551, 390, 54, 76, false, true);
	drawZone(mDuel->mManazones[0].mCards, 510, 551, 390, 54, 76, true, true);
	renderGraveyardPile(0);
	renderDeckPile(0);
	drawHand(mDuel->mHands[0].mCards, false);
	std::string playerName = mDirectDuelMode ?
		"YOU: " + deckDisplayName(mDuel->mDeckNames[0]) : "YOU";
	drawText(playerName, 18, 748, color(244, 205, 99), 22, 900);
	drawText("Hand " + std::to_string(mDuel->mHands[0].mCards.size()), 18, 775, color(229, 235, 245), 13);
	renderAttackIndicator();

	drawText(mDuel->mTurn == 0 ? "YOUR TURN" : "RIVAL THINKING", 1010, 26,
		mDuel->mTurn == 0 ? color(99, 225, 128) : color(239, 137, 70), 23);
	if (mSelectedCard >= 0 && mSelectedCard < (int)mDuel->mCardList.size())
	{
		drawText("SELECTED", 1010, 67, color(128, 172, 238), 14);
		drawText(mDuel->mCardList[mSelectedCard]->mName, 1010, 87, color(237, 239, 245), 17, 238);
	}
	else
		drawText("Select a card to filter actions", 1010, 72, color(165, 180, 204), 15, 238);

	std::string phase;
	if (mDuel->mIsChoiceActive) phase = mDuel->mChoice == NULL ? "Choose an option" : mDuel->mChoice->mInfotext;
	else if (mDuel->mAttackphase == PHASE_BLOCK) phase = "Choose a blocker";
	else if (mDuel->mAttackphase == PHASE_TARGET) phase = "Choose shields";
	else if (mDuel->mAttackphase == PHASE_TRIGGER) phase = "Choose shield triggers";
	else if (mDuel->mCastingCard != -1) phase = "Choose mana to tap";
	if (!phase.empty()) drawText(phase, 1010, 119, color(245, 199, 91), 15, 238);

	if (mDuel->getPlayerToMove() == 0 && mDuelResult == -1)
	{
		std::vector<Message> actions = visibleActions();
		int maxScroll = std::max(0, (int)actions.size() - 8);
		mActionScroll = std::min(mActionScroll, maxScroll);
		for (int i = mActionScroll; i < (int)actions.size() && i < mActionScroll + 8; ++i)
		{
			int visibleIndex = i - mActionScroll;
			SDL_Rect button = { 1004, 160 + visibleIndex * 66, 252, 55 };
			fillRect(button, 35, 51, 76, 245);
			outlineRect(button, 116, 151, 201, 255, 2);
			std::string label = actionLabel(actions[i]);
			drawText(std::to_string(visibleIndex + 1) + ". " + label, button.x + 10, button.y + 9, color(235, 239, 247), 15, button.w - 20);
			mActionButtons.push_back({ button, actions[i], label });
		}
		if ((int)actions.size() > 8) drawText("Mouse wheel: more actions", 1009, 708, color(158, 177, 205), 13);
	}
	fillRect(ACTION_LOG_BUTTON, 49, 39, 64, 248);
	outlineRect(ACTION_LOG_BUTTON, 184, 139, 211, 255, 2);
	drawText("ACTION LOG  [L]", ACTION_LOG_BUTTON.x + 55, ACTION_LOG_BUTTON.y + 10,
		color(242, 221, 250), 15);

	if (mOpenGraveyardPlayer >= 0) renderGraveyardOverlay();
	renderDragOverlay();
	renderHoverPreview();
	if (mActionLogOpen) renderActionLogOverlay();

	if (mDuelResult != -1)
	{
		fillRect({ 260, 280, 700, 190 }, 10, 15, 25, 238);
		outlineRect({ 260, 280, 700, 190 }, 214, 166, 67, 255, 4);
		drawText(mDuelResult == 0 ? "VICTORY" : "DEFEAT", 475, 319,
			mDuelResult == 0 ? color(101, 231, 133) : color(238, 101, 83), 48);
		drawText(mDirectDuelMode ? "Direct duel complete..." : "Returning to Emberglen...",
			mDirectDuelMode ? 475 : 460, 394, color(226, 232, 243), 20);
	}
}
