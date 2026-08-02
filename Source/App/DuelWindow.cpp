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
}

void Application::startDuel(int npcIndex)
{
	stopDuel();
	ensurePlayerDataLoaded();
	mActiveNpc = npcIndex;
	mDuel = new Duel();
	ActiveDuel = mDuel;
	mDuel->setDecks(mActiveDeckPath, mNpcs[npcIndex].deck);
	mDuel->startDuel();
	mDuelThread = std::thread(&Duel::loopInput, mDuel);
	mSelectedCard = -1;
	mHoveredCard = -1;
	mHoverCandidateCard = -1;
	mHoverCandidateSince = 0;
	cancelDrag();
	mCardAnimations.clear();
	mActionScroll = 0;
	mOpenGraveyardPlayer = -1;
	mGraveyardOffset = 0;
	mNextAiMove = SDL_GetTicks() + 700;
	mDuelResult = -1;
	mDuelResultAt = 0;
	mDialogueNpc = -1;
	mScreen = Screen::Duel;
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
	mHoveredCard = -1;
	mHoverCandidateCard = -1;
	mHoverCandidateSince = 0;
	cancelDrag();
}

void Application::handleDuelEvent(const SDL_Event& event)
{
	if (mOpenGraveyardPlayer >= 0 && handleGraveyardEvent(event)) return;
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			if (mDraggingCard >= 0)
			{
				cancelDrag();
				return;
			}
			stopDuel();
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

void Application::updateDuel(Uint32 deltaTime)
{
	if (mDuel == NULL) return;
	updateCardAnimations(deltaTime);
	Uint32 now = SDL_GetTicks();
	int winner = -1;
	{
		std::lock_guard<std::mutex> lock(gMutex);
		winner = mDuel->mWinner;
		if (winner == -1 && mDuel->getPlayerToMove() == 1 && now >= mNextAiMove && !mDuel->mMsgMngr.hasMoreMessages())
		{
			std::vector<Message> moves = mDuel->getPossibleMoves();
			if (!moves.empty())
			{
				HeuristicBot rival(1);
				Message move = rival.chooseMove(*mDuel, moves);
				mDuel->handleInterfaceInput(move);
			}
			mNextAiMove = now + 600;
		}
	}

	if (winner != -1 && mDuelResult == -1)
	{
		mDuelResult = winner;
		mDuelResultAt = now;
	}
	if (mDuelResult != -1 && now - mDuelResultAt > 1800)
	{
		bool won = mDuelResult == 0;
		if (won && mActiveNpc >= 0) mNpcs[mActiveNpc].defeated = true;
		std::string rival = mActiveNpc >= 0 ? mNpcs[mActiveNpc].name : "your rival";
		mNotice = won ? "Victory over " + rival + "!" : "Defeat. You can challenge " + rival + " again.";
		mNoticeUntil = now + 5000;
		stopDuel();
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

bool Application::beginBlackFeatherAiSmoke(int& blackFeather, int& sacrifice)
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (mDuel->mIsChoiceActive || mDuel->mChoice != NULL ||
		mDuel->mLuaCallbackSuspended || mDuel->mMsgMngr.hasMoreMessages())
		return false;

	blackFeather = -1;
	sacrifice = -1;
	for (size_t i = 0; i < mDuel->mCardList.size(); ++i)
	{
		Card* card = mDuel->mCardList[i];
		if (card->mOwner != 1) continue;
		if (card->mName == "Black Feather, Shadow of Rage" && blackFeather < 0)
			blackFeather = card->mUniqueId;
		else if (card->mType == TYPE_CREATURE && sacrifice < 0)
			sacrifice = card->mUniqueId;
	}
	if (blackFeather < 0 || sacrifice < 0) return false;

	Card* sacrificeCard = mDuel->mCardList[sacrifice];
	mDuel->getZone(1, sacrificeCard->mZone)->removeCard(sacrificeCard);
	mDuel->mBattlezones[1].addCard(sacrificeCard);
	sacrificeCard->mZone = ZONE_BATTLE;

	Card* blackFeatherCard = mDuel->mCardList[blackFeather];
	Message summon("cardmove");
	summon.addValue("card", blackFeather);
	summon.addValue("from", blackFeatherCard->mZone);
	summon.addValue("to", ZONE_BATTLE);
	summon.addValue("evobait", -1);
	mDuel->mMsgMngr.sendMessage(summon);
	// Hold the AI briefly so the smoke test can assert that Black Feather
	// itself is one of the mandatory legal targets before resolving it.
	mNextAiMove = SDL_GetTicks() + 60000;
	return true;
}

bool Application::verifyBlackFeatherAiSmoke(int blackFeather, int sacrifice)
{
	if (mDuel == NULL) return false;
	std::lock_guard<std::mutex> lock(gMutex);
	if (blackFeather < 0 || sacrifice < 0 ||
		blackFeather >= (int)mDuel->mCardList.size() || sacrifice >= (int)mDuel->mCardList.size())
		return false;
	int blackFeatherZone = mDuel->mCardList[blackFeather]->mZone;
	int sacrificeZone = mDuel->mCardList[sacrifice]->mZone;
	bool exactlyOneWasDestroyed =
		(blackFeatherZone == ZONE_GRAVEYARD && sacrificeZone == ZONE_BATTLE) ||
		(blackFeatherZone == ZONE_BATTLE && sacrificeZone == ZONE_GRAVEYARD);
	bool passed = !mDuel->mIsChoiceActive && !mDuel->mLuaCallbackSuspended && exactlyOneWasDestroyed;
	if (!passed)
	{
		std::cerr << "Black Feather state: choice=" << mDuel->mIsChoiceActive
			<< " suspended=" << mDuel->mLuaCallbackSuspended
			<< " black-zone=" << mDuel->mCardList[blackFeather]->mZone
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
		else
		{
			const SDL_Rect opponentDrop = { 150, 0, 760, 200 };
			const SDL_Rect attackPrompt = { 330, 315, 320, 80 };
			if (contains(opponentDrop, mouseX, mouseY) || contains(attackPrompt, mouseX, mouseY))
				found = findDragAction("creatureattack", cardId, -1, action);
		}
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
	const char* keys[] = { "card", "attacker", "defender", "blocker", "trigger", "shield", "selection", "evobait" };
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
		if (uid >= 0 && uid < (int)mDuel->mCardList.size()) return mDuel->mCardList[uid]->mName;
		return "card";
	};
	if (type == "endturn") return "End turn";
	if (type == "cardmana") return "Charge " + cardName(messageInt(message, "card"));
	if (type == "cardplay") return "Cast " + cardName(messageInt(message, "card"));
	if (type == "manatap") return "Tap mana: " + cardName(messageInt(message, "card"));
	if (type == "creatureattack")
	{
		int defenderType = messageInt(message, "defendertype");
		return "Attack " + std::string(defenderType == DEFENDER_PLAYER ? "rival" : cardName(messageInt(message, "defender")));
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
		return selection < 0 ? "Choose option" : "Choose " + cardName(selection);
	}
	return type;
}

SDL_Rect Application::graveyardPileRect(int player) const
{
	return { 914, player == 1 ? 86 : 551, 54, 76 };
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
	drawText(mActiveNpc >= 0 ? mNpcs[mActiveNpc].name : "RIVAL", 18, 14, color(244, 205, 99), 20);
	drawText("Deck " + std::to_string(mDuel->mDecks[1].mCards.size()), 18, 39, color(229, 235, 245), 13);
	drawHand(mDuel->mHands[1].mCards, true);

	drawText("RIVAL MANA", 78, 62, color(180, 198, 224), 13);
	drawText("RIVAL SHIELDS", 520, 62, color(180, 198, 224), 13);
	drawZone(mDuel->mManazones[1].mCards, 70, 82, 390, 54, 76, true, true);
	drawZone(mDuel->mShields[1].mCards, 510, 82, 390, 54, 76, false, true);
	renderGraveyardPile(1);
	drawZone(mDuel->mBattlezones[1].mCards, 65, 205, 850, 82, 114, true, true);

	fillRect({ 372, 332, 235, 42 }, 20, 27, 43, 220);
	outlineRect({ 372, 332, 235, 42 }, 122, 92, 49, 255, 2);
	drawText("DRAG HERE TO ATTACK", 397, 343, color(229, 200, 130), 14);

	drawZone(mDuel->mBattlezones[0].mCards, 65, 390, 850, 82, 114, true, true);
	drawText("YOUR SHIELDS", 78, 532, color(180, 198, 224), 13);
	drawText("YOUR MANA", 520, 532, color(180, 198, 224), 13);
	drawZone(mDuel->mShields[0].mCards, 70, 551, 390, 54, 76, false, true);
	drawZone(mDuel->mManazones[0].mCards, 510, 551, 390, 54, 76, true, true);
	renderGraveyardPile(0);
	drawHand(mDuel->mHands[0].mCards, false);
	drawText("YOU", 18, 748, color(244, 205, 99), 22);
	drawText("Deck " + std::to_string(mDuel->mDecks[0].mCards.size()), 18, 775, color(229, 235, 245), 13);

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
		int maxScroll = std::max(0, (int)actions.size() - 9);
		mActionScroll = std::min(mActionScroll, maxScroll);
		for (int i = mActionScroll; i < (int)actions.size() && i < mActionScroll + 9; ++i)
		{
			int visibleIndex = i - mActionScroll;
			SDL_Rect button = { 1004, 160 + visibleIndex * 66, 252, 55 };
			fillRect(button, 35, 51, 76, 245);
			outlineRect(button, 116, 151, 201, 255, 2);
			std::string label = actionLabel(actions[i]);
			drawText(std::to_string(visibleIndex + 1) + ". " + label, button.x + 10, button.y + 9, color(235, 239, 247), 15, button.w - 20);
			mActionButtons.push_back({ button, actions[i], label });
		}
		if ((int)actions.size() > 9) drawText("Mouse wheel: more actions", 1009, 760, color(158, 177, 205), 13);
	}

	if (mOpenGraveyardPlayer >= 0) renderGraveyardOverlay();
	renderDragOverlay();
	renderHoverPreview();

	if (mDuelResult != -1)
	{
		fillRect({ 260, 280, 700, 190 }, 10, 15, 25, 238);
		outlineRect({ 260, 280, 700, 190 }, 214, 166, 67, 255, 4);
		drawText(mDuelResult == 0 ? "VICTORY" : "DEFEAT", 475, 319,
			mDuelResult == 0 ? color(101, 231, 133) : color(238, 101, 83), 48);
		drawText("Returning to Emberglen...", 460, 394, color(226, 232, 243), 20);
	}
}
