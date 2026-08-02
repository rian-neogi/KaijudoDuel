#include "Application.h"

#include "AppSupport.h"
#include "Game/Card.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>

using namespace AppSupport;

void Application::startDuel(int npcIndex)
{
	stopDuel();
	mActiveNpc = npcIndex;
	mDuel = new Duel();
	ActiveDuel = mDuel;
	mDuel->setDecks("Decks/My Decks/7 - L Tappy Tappy.txt", mNpcs[npcIndex].deck);
	mDuel->startDuel();
	mDuelThread = std::thread(&Duel::loopInput, mDuel);
	mSelectedCard = -1;
	mHoveredCard = -1;
	cancelDrag();
	mCardAnimations.clear();
	mActionScroll = 0;
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
	mHoveredCard = -1;
	cancelDrag();
}

void Application::handleDuelEvent(const SDL_Event& event)
{
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
			if (!moves.empty()) mDuel->handleInterfaceInput(moves[std::rand() % moves.size()]);
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
	mDraggingCard = cardId;
	mDragFromZone = card->mZone;
	mDragOrigin = origin;
	mDragMouseX = mouseX;
	mDragMouseY = mouseY;
	mSelectedCard = cardId;
	mHoveredCard = -1;
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
		return selection < 0 ? "Choose option" : "Choose " + cardName(selection);
	}
	return type;
}

void Application::renderDuel()
{
	if (mBoardTexture != NULL)
		SDL_RenderCopy(mRenderer, mBoardTexture, NULL, NULL);
	else
		fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 75, 45, 31);
	fillRect({ 980, 0, 300, 800 }, 13, 19, 31, 238);
	outlineRect({ 980, 0, 300, 800 }, 181, 137, 56, 255, 3);

	int hovered = -1;
	if (mDraggingCard < 0)
	{
		for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
		{
			if (item->faceUp && contains(item->rect, mMouseX, mMouseY))
			{
				hovered = item->cardId;
				break;
			}
		}
	}
	mHoveredCard = hovered;
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
	drawZone(mDuel->mBattlezones[1].mCards, 65, 205, 850, 82, 114, true, true);

	fillRect({ 372, 332, 235, 42 }, 20, 27, 43, 220);
	outlineRect({ 372, 332, 235, 42 }, 122, 92, 49, 255, 2);
	drawText("DRAG HERE TO ATTACK", 397, 343, color(229, 200, 130), 14);

	drawZone(mDuel->mBattlezones[0].mCards, 65, 390, 850, 82, 114, true, true);
	drawText("YOUR SHIELDS", 78, 532, color(180, 198, 224), 13);
	drawText("YOUR MANA", 520, 532, color(180, 198, 224), 13);
	drawZone(mDuel->mShields[0].mCards, 70, 551, 390, 54, 76, false, true);
	drawZone(mDuel->mManazones[0].mCards, 510, 551, 390, 54, 76, true, true);
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

