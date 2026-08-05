#include "Application.h"

#include "AI/HeuristicBot.h"
#include "AppSupport.h"
#include "CatalogMapStorage.h"
#include "Game/Card.h"
#include "Landmarks.h"
#include "RtpTilesetRenderer.h"
#include "SpriteSheetRenderer.h"
#include "WorldStorage.h"
#include "WorldTileRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <set>

using namespace AppSupport;

namespace
{
	constexpr Uint32 DOOR_OPEN_DURATION = 400;
	const SDL_Rect GRAVEYARD_NEXT = { 700, 590, 145, 42 };
	const SDL_Rect SETTINGS_BUTTON = { 890, 502, 300, 58 };
}

int Application::runSmokeTests()
{
	const bool smokeTest = true;
		if (mScreen != Screen::MainMenu)
		{
			std::cerr << "Main menu did not open at startup." << std::endl;
			return 2;
		}
		renderMainMenu();
		SDL_Event menuNavigation = {};
		menuNavigation.type = SDL_KEYDOWN;
		menuNavigation.key.keysym.sym = SDLK_DOWN;
		handleEvent(menuNavigation);
		menuNavigation.key.keysym.sym = SDLK_UP;
		handleEvent(menuNavigation);
		mScreen = Screen::LoadGame;
		renderLoadGame();
		menuNavigation.key.keysym.sym = SDLK_ESCAPE;
		handleEvent(menuNavigation);
		std::string savedDirectory = mActiveSaveDirectory;
		mActiveSaveDirectory = "PlayerData/Smoke Slot";
		bool savePathsReady = playerDataPath("progress.txt") ==
			"PlayerData/Smoke Slot/progress.txt" &&
			playerDeckDirectory() == "PlayerData/Smoke Slot/Decks";
		mActiveSaveDirectory = savedDirectory;
		if (mScreen != Screen::MainMenu || !savePathsReady)
		{
			std::cerr << "Save menu smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseOverworldMovementSmoke())
		{
			std::cerr << "Overworld movement smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseStorySmoke())
		{
			std::cerr << "Act I story smoke test failed." << std::endl;
			return 2;
		}
		ensurePlayerDataLoaded();
		if (mNpcs.empty() || !startDuelWithDecks(mActiveDeckPath,
			mNpcs[0].deckForBattle(0), 0))
		{
			std::cerr << "Unable to start smoke-test duel." << std::endl;
			return 2;
		}

	Uint32 previous = SDL_GetTicks();
	int smokeFrames = 0;
	int smokeNpc = 0;
	int smokeBlackFeather = -1;
	int smokeBlackFeatherSacrifice = -1;
	bool blackFeatherSmokeStarted = false;
	bool blackFeatherWasSelectable = false;
	int smokeStingerWorm = -1;
	int smokeStingerWormSacrifice = -1;
	bool stingerWormSmokeStarted = false;
	bool stingerWormWasSelectable = false;
	while (mRunning)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
			handleEvent(event);

		Uint32 now = SDL_GetTicks();
		Uint32 delta = std::min<Uint32>(now - previous, 100);
		previous = now;
		update(delta);
		render();
		if (smokeTest)
		{
			if (smokeNpc == 0 && smokeFrames == 5 && !exerciseHoverTimingSmoke())
			{
				std::cerr << "Hover timing smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 10 && !exerciseBinaryChoiceSmoke())
			{
				std::cerr << "Binary choice menu smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 11 && !exerciseActionLabelSmoke())
			{
				std::cerr << "Command menu label smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames >= 100 && smokeFrames < 180 && mDraggingCard < 0)
			{
				for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
				{
					std::lock_guard<std::mutex> lock(gMutex);
					if (item->hoverAnchor && mDuel != NULL && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size() &&
						mDuel->mCardList[item->cardId]->mOwner == 0 && mDuel->mCardList[item->cardId]->mZone == ZONE_HAND)
					{
						mMouseX = item->rect.x + item->rect.w / 2;
						mMouseY = item->rect.y + item->rect.h / 2;
						break;
					}
				}
			}
			if (smokeNpc == 0 && smokeFrames == 20 && !exerciseEvolutionSmoke())
			{
				std::cerr << "Evolution smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 30 && !exerciseHeuristicAttackSafetySmoke())
			{
				std::cerr << "Heuristic attack safety smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 31 && !exerciseHeuristicBlockChoiceSmoke())
			{
				std::cerr << "Heuristic blocker choice smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 32 && !exerciseHeuristicManaConservationSmoke())
			{
				std::cerr << "Heuristic mana conservation smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 33 && !exerciseMultiCivilizationSmoke())
			{
				std::cerr << "Multi-civilization rules smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 34 && !exerciseRaceQuerySmoke())
			{
				std::cerr << "Recursive race-query smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 35 && !exerciseCrypticTotemSmoke())
			{
				std::cerr << "Cryptic Totem shield-trigger smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 60 && mDuel != NULL)
			{
				int selectionCard = -1;
				{
					std::lock_guard<std::mutex> lock(gMutex);
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
					{
						if (moves[i].getType() == "choiceselect" && messageInt(moves[i], "selection") >= 0)
						{
							selectionCard = messageInt(moves[i], "selection");
							break;
						}
					}
				}
				CardHitbox handCard = { { 0, 0, 0, 0 }, -1, true, true, true };
				{
					std::lock_guard<std::mutex> lock(gMutex);
					for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
					{
						if (item->hoverAnchor && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size() &&
							mDuel->mCardList[item->cardId]->mOwner == 0 && mDuel->mCardList[item->cardId]->mZone == ZONE_HAND)
						{
							handCard = *item;
							break;
						}
					}
				}
				if (selectionCard >= 0 && handCard.cardId >= 0)
				{
					beginDrag(handCard.cardId, handCard.rect, handCard.rect.x, handCard.rect.y);
					if (mDraggingCard >= 0)
					{
						std::cerr << "Pending choice allowed an unrelated card drag." << std::endl;
						return 2;
					}
				}
				SDL_Rect selectionButton = { 0, 0, 0, 0 };
				for (size_t i = 0; i < mActionButtons.size(); ++i)
				{
					if (mActionButtons[i].message.getType() == "choiceselect" &&
						messageInt(mActionButtons[i].message, "selection") == selectionCard)
					{
						selectionButton = mActionButtons[i].rect;
						break;
					}
				}
				if (selectionCard < 0 || selectionButton.w == 0)
				{
					std::cerr << "Clickable action smoke test could not find its button." << std::endl;
					return 2;
				}
				SDL_Event click = {};
				click.type = SDL_MOUSEBUTTONDOWN;
				click.button.button = SDL_BUTTON_LEFT;
				// SDL_RenderSetLogicalSize filters real mouse events into logical
				// coordinates before they reach the application.
				click.button.x = selectionButton.x + selectionButton.w / 2;
				click.button.y = selectionButton.y + selectionButton.h / 2;
				handleDuelEvent(click);
				{
					std::lock_guard<std::mutex> lock(gMutex);
					if (mDuel->mIsChoiceActive)
					{
						std::cerr << "Clickable action smoke test did not dispatch its click." << std::endl;
						return 2;
					}
				}
			}
			if (smokeNpc == 0 && smokeFrames >= 65 && smokeFrames < 75 && !blackFeatherSmokeStarted)
			{
				blackFeatherSmokeStarted = beginMandatorySacrificeAiSmoke(
					"Black Feather, Shadow of Rage",
					smokeBlackFeather, smokeBlackFeatherSacrifice);
				if (smokeFrames == 74 && !blackFeatherSmokeStarted && mDuel != NULL)
				{
					std::lock_guard<std::mutex> lock(gMutex);
					std::cerr << "Black Feather setup state: choice=" << mDuel->mIsChoiceActive
						<< " pointer=" << (mDuel->mChoice != NULL)
						<< " suspended=" << mDuel->mLuaCallbackSuspended
						<< " queued=" << mDuel->mMsgMngr.hasMoreMessages() << std::endl;
				}
			}
			if (smokeNpc == 0 && blackFeatherSmokeStarted && !blackFeatherWasSelectable && mDuel != NULL)
			{
				std::lock_guard<std::mutex> lock(gMutex);
				if (mDuel->mIsChoiceActive && mDuel->mChoicePlayer == 1)
				{
					blackFeatherWasSelectable = mDuel->choiceCanBeSelected(smokeBlackFeather) == 1;
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
						if (moves[i].getType() == "choiceselect" && messageInt(moves[i], "selection") < 0)
							blackFeatherWasSelectable = false;
					mNextAiMove = 0;
				}
			}
			if (smokeFrames == 75 && mDuel != NULL)
			{
				Message endTurn;
				bool foundEndTurn = false;
				{
					std::lock_guard<std::mutex> lock(gMutex);
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
					{
						if (moves[i].getType() == "endturn")
						{
							endTurn = moves[i];
							foundEndTurn = true;
							break;
						}
					}
				}
				if (foundEndTurn) playAction(endTurn);
			}
			if (smokeNpc == 0 && smokeFrames == 90 &&
				(!blackFeatherSmokeStarted || !blackFeatherWasSelectable ||
					 !verifyMandatorySacrificeAiSmoke("Black Feather, Shadow of Rage",
						smokeBlackFeather, smokeBlackFeatherSacrifice)))
			{
				std::cerr << "Black Feather AI sacrifice smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 5 && smokeFrames >= 5 && smokeFrames < 20 && !stingerWormSmokeStarted)
			{
				stingerWormSmokeStarted = beginMandatorySacrificeAiSmoke(
					"Stinger Worm", smokeStingerWorm, smokeStingerWormSacrifice);
			}
			if (smokeNpc == 5 && stingerWormSmokeStarted && !stingerWormWasSelectable && mDuel != NULL)
			{
				std::lock_guard<std::mutex> lock(gMutex);
				if (mDuel->mIsChoiceActive && mDuel->mChoicePlayer == 1)
				{
					stingerWormWasSelectable = mDuel->choiceCanBeSelected(smokeStingerWorm) == 1;
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
						if (moves[i].getType() == "choiceselect" && messageInt(moves[i], "selection") < 0)
							stingerWormWasSelectable = false;
					mNextAiMove = 0;
				}
			}
			if (smokeNpc == 5 && smokeFrames == 40 &&
				(!stingerWormSmokeStarted || !stingerWormWasSelectable ||
				 !verifyMandatorySacrificeAiSmoke("Stinger Worm",
					smokeStingerWorm, smokeStingerWormSacrifice)))
			{
				std::cerr << "Stinger Worm AI sacrifice smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 95 && !exerciseGraveyardBrowserSmoke())
			{
				std::cerr << "Graveyard browser smoke test failed." << std::endl;
				return 2;
			}
			SDL_Delay(5);
			// The first duel exercises a complete AI/render cycle and NPC 5 owns the
			// Stinger Worm sequence above. Remaining NPCs need only enough frames to
			// validate deck startup, rendering, and teardown; keeping each at a full
			// second made metadata growth linearly exhaust the CTest timeout.
			const int smokeFrameLimit = smokeNpc == 0 ? 300 : (smokeNpc == 5 ? 60 : 20);
			if (++smokeFrames >= smokeFrameLimit)
			{
				smokeFrames = 0;
				++smokeNpc;
				while (smokeNpc < (int)mNpcs.size() && !mNpcs[smokeNpc].isDuelist()) ++smokeNpc;
				if (smokeNpc < (int)mNpcs.size()) startDuel(smokeNpc, true);
				else
				{
					stopDuel();
					if (!exerciseMenuScreensSmoke())
					{
						std::cerr << "Menu and deck-builder smoke test failed." << std::endl;
						return 2;
					}
					mRunning = false;
				}
			}
		}
	}
	return 0;
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

	int lightBringerId = getCardIdFromName("Chilias, the Oracle");
	int cyberLordId = getCardIdFromName("Tropico");
	int vortexId = getCardIdFromName("Wise Starnoid, Avatar of Hope");
	int innocentHunterId = getCardIdFromName("Innocent Hunter, Blade of All");
	int unrelatedId = getCardIdFromName("Bone Spider");
	if (lightBringerId < 0 || cyberLordId < 0 || vortexId < 0 ||
		innocentHunterId < 0 || unrelatedId < 0) return false;

	Duel* savedActiveDuel = ActiveDuel;
	bool vortexPassed = false;
	{
		Duel vortex;
		vortex.mIsSimulation = true;
		ActiveDuel = &vortex;
		auto addCard = [&vortex](int cardId, int zone) -> int
		{
			int uid = static_cast<int>(vortex.mCardList.size());
			Card* card = new Card(uid, cardId, 0);
			vortex.mCardList.push_back(card);
			vortex.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};

		int lightBringer = addCard(lightBringerId, ZONE_BATTLE);
		int cyberLord = addCard(cyberLordId, ZONE_BATTLE);
		int innocentHunter = addCard(innocentHunterId, ZONE_BATTLE);
		int unrelated = addCard(unrelatedId, ZONE_BATTLE);
		int evolution = addCard(vortexId, ZONE_HAND);
		for (int i = 0; i < 3; ++i)
		{
			addCard(lightBringerId, ZONE_MANA);
			addCard(cyberLordId, ZONE_MANA);
		}
		bool legal = vortex.getEvolutionBaitCount(evolution) == 2 &&
			vortex.getCreatureCanVortexEvolve(evolution, lightBringer, cyberLord) == 1 &&
			vortex.getCreatureCanVortexEvolve(evolution, cyberLord, lightBringer) == 1 &&
			vortex.getCreatureCanVortexEvolve(evolution, innocentHunter, cyberLord) == 1 &&
			vortex.getCreatureCanVortexEvolve(evolution, lightBringer, innocentHunter) == 1 &&
			vortex.getCreatureCanVortexEvolve(evolution, innocentHunter, unrelated) == 0 &&
			vortex.getCreatureCanVortexEvolve(evolution, lightBringer, lightBringer) == 0;

		vortex.mTurn = 0;
		vortex.mTurnPhase = TURN_PHASE_MAIN;
		std::vector<Message> moves = vortex.getPossibleMoves();
		Message evolveVortex;
		bool generated = false;
		for (size_t i = 0; i < moves.size(); ++i)
		{
			if (moves[i].getType() == "cardplay" &&
				messageInt(moves[i], "card") == evolution &&
				((messageInt(moves[i], "evobait") == innocentHunter &&
				  messageInt(moves[i], "evobait2") == cyberLord) ||
				 (messageInt(moves[i], "evobait") == cyberLord &&
				  messageInt(moves[i], "evobait2") == innocentHunter)))
			{
				evolveVortex = moves[i];
				generated = true;
				break;
			}
		}
		if (generated)
		{
			vortex.handleInterfaceInput(evolveVortex);
			for (int safety = 0; vortex.mCastingCard != -1 && safety < 10; ++safety)
			{
				std::vector<Message> paymentMoves = vortex.getPossibleMoves();
				std::vector<Message>::iterator mana = std::find_if(paymentMoves.begin(), paymentMoves.end(),
					[](Message& move) { return move.getType() == "manatap"; });
				if (mana == paymentMoves.end()) break;
				vortex.handleInterfaceInput(*mana);
				vortex.dispatchAllMessages();
			}
		}
		bool stacked = vortex.mBattlezones[0].mCards.size() == 3 &&
			vortex.mCardList[evolution]->mZone == ZONE_BATTLE &&
			vortex.mCardList[evolution]->mEvoStack.size() == 2 &&
			vortex.mCardList[innocentHunter]->mZone == ZONE_EVOLVED &&
			vortex.mCardList[cyberLord]->mZone == ZONE_EVOLVED;

		Message leave("cardmove");
		leave.addValue("card", evolution);
		leave.addValue("from", ZONE_BATTLE);
		leave.addValue("to", ZONE_GRAVEYARD);
		vortex.dispatchMessage(leave);
		vortex.dispatchAllMessages();
		bool movedTogether = vortex.mCardList[evolution]->mZone == ZONE_GRAVEYARD &&
			vortex.mCardList[innocentHunter]->mZone == ZONE_GRAVEYARD &&
			vortex.mCardList[cyberLord]->mZone == ZONE_GRAVEYARD &&
			vortex.mCardList[evolution]->mEvoStack.empty();
		vortexPassed = legal && generated && vortex.mCastingCard == -1 && stacked && movedTogether;
	}
	bool dm12HooksPassed = false;
	int soulPhoenixId = getCardIdFromName("Soul Phoenix, Avatar of Unity");
	int fireBirdId = getCardIdFromName("Peppi Pepper");
	int earthDragonId = getCardIdFromName("Terradragon Arque Delacerna");
	int copperLocustId = getCardIdFromName("Copper Locust");
	int meloppeId = getCardIdFromName("Meloppe");
	if (soulPhoenixId >= 0 && fireBirdId >= 0 && earthDragonId >= 0 &&
		copperLocustId >= 0 && meloppeId >= 0)
	{
		Duel hooks;
		hooks.mIsSimulation = true;
		ActiveDuel = &hooks;
		auto addCard = [&hooks](int cardId, int zone) -> int
		{
			int uid = static_cast<int>(hooks.mCardList.size());
			Card* card = new Card(uid, cardId, 0);
			hooks.mCardList.push_back(card);
			hooks.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};
		int fireBird = addCard(fireBirdId, ZONE_BATTLE);
		int earthDragon = addCard(earthDragonId, ZONE_BATTLE);
		int copperLocust = addCard(copperLocustId, ZONE_BATTLE);
		int meloppe = addCard(meloppeId, ZONE_BATTLE);
		int soulPhoenix = addCard(soulPhoenixId, ZONE_HAND);

		Message evolveSoul("cardmove");
		evolveSoul.addValue("card", soulPhoenix);
		evolveSoul.addValue("from", ZONE_HAND);
		evolveSoul.addValue("to", ZONE_BATTLE);
		evolveSoul.addValue("evobait", fireBird);
		evolveSoul.addValue("evobait2", earthDragon);
		hooks.dispatchMessage(evolveSoul);
		hooks.dispatchAllMessages();
		bool evolutionEventWorked = hooks.mCardList[copperLocust]->mZone == ZONE_GRAVEYARD;

		Message leaveSoul("cardmove");
		leaveSoul.addValue("card", soulPhoenix);
		leaveSoul.addValue("from", ZONE_BATTLE);
		leaveSoul.addValue("to", ZONE_GRAVEYARD);
		hooks.dispatchMessage(leaveSoul);
		hooks.dispatchAllMessages();
		bool separated = hooks.mCardList[soulPhoenix]->mZone == ZONE_GRAVEYARD &&
			hooks.mCardList[fireBird]->mZone == ZONE_BATTLE &&
			hooks.mCardList[earthDragon]->mZone == ZONE_BATTLE &&
			hooks.mCardList[soulPhoenix]->mEvoStack.empty();
		bool shieldChoiceSwapped = hooks.mCardList[meloppe]->mZone == ZONE_BATTLE &&
			hooks.getShieldChooser(1,0) == 0 && hooks.getShieldChooser(0,1) == 1;
		dm12HooksPassed = evolutionEventWorked && separated && shieldChoiceSwapped;
	}
	ActiveDuel = savedActiveDuel;
	return vortexPassed && dm12HooksPassed;
}

bool Application::exerciseRaceQuerySmoke()
{
	int ultimateDragonId = getCardIdFromName("Ultimate Dragon");
	int kanesillId = getCardIdFromName("Kanesill, the Explorer");
	if (ultimateDragonId < 0 || kanesillId < 0) return false;

	std::lock_guard<std::mutex> lock(gMutex);
	Duel* savedActiveDuel = ActiveDuel;
	bool passed = false;
	{
		Duel test;
		test.mIsSimulation = true;
		ActiveDuel = &test;
		auto addBattleCard = [&test](int cardId) -> int
		{
			int uid = static_cast<int>(test.mCardList.size());
			Card* card = new Card(uid, cardId, 0);
			test.mCardList.push_back(card);
			test.mBattlezones[0].addCard(card);
			card->mZone = ZONE_BATTLE;
			return uid;
		};

		addBattleCard(ultimateDragonId);
		int kanesill = addBattleCard(kanesillId);
		passed = test.getCreatureRace(kanesill) == "Gladiator" &&
			test.isCreatureOfRace(kanesill,"Gladiator") == 1 &&
			test.mRaceQueryDepth == 0;
	}
	ActiveDuel = savedActiveDuel;
	return passed;
}

bool Application::exerciseCrypticTotemSmoke()
{
	int crypticTotemId = getCardIdFromName("Cryptic Totem");
	int terrorPitId = getCardIdFromName("Terror Pit");
	if (crypticTotemId < 0 || terrorPitId < 0) return false;

	std::lock_guard<std::mutex> lock(gMutex);
	Duel* savedActiveDuel = ActiveDuel;
	bool passed = false;
	{
		Duel test;
		test.mIsSimulation = true;
		ActiveDuel = &test;
		auto addCard = [&test](int cardId, int owner, int zone) -> int
		{
			int uid = static_cast<int>(test.mCardList.size());
			Card* card = new Card(uid, cardId, owner);
			test.mCardList.push_back(card);
			test.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};

		int crypticTotem = addCard(crypticTotemId, 0, ZONE_BATTLE);
		int opposingTrigger = addCard(terrorPitId, 1, ZONE_SHIELD);
		int friendlyTrigger = addCard(terrorPitId, 0, ZONE_SHIELD);
		test.mCardList[crypticTotem]->tap();
		bool blocksOpponent = test.canUseShieldTrigger(opposingTrigger) == 0;
		bool allowsOwner = test.canUseShieldTrigger(friendlyTrigger) == 1;
		test.mCardList[crypticTotem]->untap();
		bool inactiveWhileUntapped = test.canUseShieldTrigger(opposingTrigger) == 1;
		passed = blocksOpponent && allowsOwner && inactiveWhileUntapped;
	}
	ActiveDuel = savedActiveDuel;
	return passed;
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
	// Keep this rules regression independent of NPC deck-list changes. If the
	// active smoke deck no longer contains the named card, add a test copy to
	// the AI hand and resolve the same real Lua summon callback below.
	if (summonedCard < 0)
	{
		int cardId = getCardIdFromName(cardName);
		if (cardId < 0) return false;
		summonedCard = (int)mDuel->mCardList.size();
		Card* summoned = new Card(summonedCard, cardId, 1);
		mDuel->mCardList.push_back(summoned);
		mDuel->mHands[1].addCard(summoned);
		summoned->mZone = ZONE_HAND;
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

bool Application::exerciseHoverTimingSmoke()
{
	bool valid = true;
	updateHoverState(42, false, 1000);
	valid = valid && mHoveredCard == -1;
	updateHoverState(42, false, 1999);
	valid = valid && mHoveredCard == -1;
	updateHoverState(42, false, 2000);
	valid = valid && mHoveredCard == 42;
	updateHoverState(43, false, 2001);
	valid = valid && mHoveredCard == -1;
	updateHoverState(-1, false, 2002);
	valid = valid && mHoveredCard == -1 && mHoverCandidateCard == -1;
	updateHoverState(7, true, 3000);
	valid = valid && mHoveredCard == 7;
	updateHoverState(-1, false, 3001);
	valid = valid && mHoveredCard == -1;

	std::vector<CardHitbox> savedHitboxes = mCardHitboxes;
	int savedCandidate = mHoverCandidateCard;
	int savedHovered = mHoveredCard;
	mCardHitboxes.clear();
	SDL_Rect overlap = { 100, 100, 80, 120 };
	mCardHitboxes.push_back({ overlap, 11, true, true, true });
	mCardHitboxes.push_back({ overlap, 12, true, true, true });
	mHoverCandidateCard = 11;
	bool immediate = false;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == 11 && immediate;
	mHoverCandidateCard = -1;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == 12;
	CardHitbox clicked;
	mHoveredCard = 11;
	valid = valid && duelClickHitboxAt(140, 150, clicked) && clicked.cardId == 11;
	mHoveredCard = -1;
	valid = valid && duelClickHitboxAt(140, 150, clicked) && clicked.cardId == 12;

	// An enlarged or shrinking card may extend above its original position, but
	// that animated area must never reactivate hover after the pointer leaves the
	// original card. This guards against rapid pop/un-pop oscillation.
	mCardHitboxes.clear();
	SDL_Rect original = { 100, 200, 88, 122 };
	SDL_Rect animated = { 4, 10, 280, 400 };
	mCardHitboxes.push_back({ original, 21, true, true, true });
	mCardHitboxes.push_back({ animated, 21, true, false, true });
	mHoverCandidateCard = 21;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == -1;
	valid = valid && duelHoverCandidateAt(140, 240, immediate) == 21 && immediate;
	mCardHitboxes = savedHitboxes;
	mHoverCandidateCard = savedCandidate;
	mHoveredCard = savedHovered;
	return valid;
}

bool Application::exerciseOverworldMovementSmoke()
{
	if (mNpcs.empty()) return false;
	std::string worldDataError;
	bool worldDataReady = mWorld.validateStructure(worldDataError) &&
		mWorld.map(mWorld.start.mapId) != NULL &&
		mWorld.mapIndex(mWorld.start.mapId) == worldAreaIndex(mWorld.start.mapId) &&
		mWorld.regionAt("overworld", 331, 690) != NULL;
	WorldData emptyWorld;
	std::string emptyWorldError;
	worldDataReady = worldDataReady && !emptyWorld.validateStructure(emptyWorldError) &&
		!emptyWorldError.empty();
	bool npcAppearancesReady = true;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		const bool singleCharacter = mNpcs[i].spriteSheet.find('$') != std::string::npos;
		npcAppearancesReady = npcAppearancesReady &&
			mNpcs[i].spriteSheet.find("Resources/Graphics/Characters/") == 0 &&
			mNpcs[i].spriteIndex >= 0 &&
			mNpcs[i].spriteIndex < (singleCharacter ? 1 : 8);
	}
	SDL_Rect spriteFrame;
	bool spriteSheetsReady = SpriteSheetRenderer::characterSourceRect(
		"Resources/Graphics/Characters/Actor1.png", 5, 0, 1, false, 0,
		384, 256, spriteFrame) && spriteFrame.x == 128 && spriteFrame.y == 128 &&
		spriteFrame.w == 32 && spriteFrame.h == 32;
	spriteSheetsReady = spriteSheetsReady && SpriteSheetRenderer::characterSourceRect(
		"Resources/Graphics/Characters/Actor1.png", 5, -1, 0, true, 0,
		384, 256, spriteFrame) && spriteFrame.x == 96 && spriteFrame.y == 160;
	spriteSheetsReady = spriteSheetsReady && SpriteSheetRenderer::characterSourceRect(
		"Resources/Graphics/Characters/!$Gate1.png", 0, 0, 1, false, 0,
		288, 256, spriteFrame) && spriteFrame.x == 96 && spriteFrame.y == 0 &&
		spriteFrame.w == 96 && spriteFrame.h == 64 &&
		!SpriteSheetRenderer::characterSourceRect(
			"Resources/Graphics/Characters/Actor1.png", 8, 0, 1, false, 0,
			384, 256, spriteFrame);
	SDL_Rect atlasTile;
	spriteSheetsReady = spriteSheetsReady && WorldTileRenderer::atlasSourceRect(
		16, 8, 32, 256, 512, atlasTile) && atlasTile.x == 0 && atlasTile.y == 64 &&
		atlasTile.w == 32 && atlasTile.h == 32;
	spriteSheetsReady = spriteSheetsReady && WorldTileRenderer::atlasSourceRect(
		199, 16, 32, 512, 512, atlasTile) && atlasTile.x == 224 && atlasTile.y == 384 &&
		!WorldTileRenderer::atlasSourceRect(256, 16, 32, 512, 512, atlasTile);
	SDL_Point autotileQuarter;
	unsigned int surrounded = WorldTileRenderer::North | WorldTileRenderer::East |
		WorldTileRenderer::South | WorldTileRenderer::West |
		WorldTileRenderer::NorthWest | WorldTileRenderer::NorthEast |
		WorldTileRenderer::SouthEast | WorldTileRenderer::SouthWest;
	spriteSheetsReady = spriteSheetsReady && WorldTileRenderer::autotileQuarterSource(
		0, surrounded, autotileQuarter) && autotileQuarter.x == 2 && autotileQuarter.y == 4;
	spriteSheetsReady = spriteSheetsReady && WorldTileRenderer::autotileQuarterSource(
		0, surrounded & ~WorldTileRenderer::NorthWest, autotileQuarter) &&
		autotileQuarter.x == 2 && autotileQuarter.y == 0;
	spriteSheetsReady = spriteSheetsReady && WorldTileRenderer::autotileQuarterSource(
		3, 0, autotileQuarter) && autotileQuarter.x == 3 && autotileQuarter.y == 5 &&
		!WorldTileRenderer::autotileQuarterSource(4, surrounded, autotileQuarter);
	spriteSheetsReady = spriteSheetsReady && WorldTileRenderer::hasDecoration(WorldTiles::Tree) &&
		WorldTileRenderer::hasDecoration(WorldTiles::Rocks) &&
		WorldTileRenderer::hasForeground(WorldTiles::Tree) &&
		!WorldTileRenderer::hasForeground(WorldTiles::Rocks);
	spriteSheetsReady = spriteSheetsReady && npcAppearancesReady;
	RtpTilesetRenderer rtpTiles(mRenderer, mAssets);
	std::string tilesetError;
	std::vector<RtpSheetDescriptor> sheets = RtpTilesetRenderer::availableSheets();
	bool fullTilesetReady = sheets.size() == 22 && rtpTiles.validateAllAssets(tilesetError) &&
		RtpTilesetRenderer::descriptor(RtpTilesetFamily::Outside, RtpTileSheet::A3) != NULL &&
		RtpTilesetRenderer::descriptor(RtpTilesetFamily::World, RtpTileSheet::C) == NULL &&
		RtpTilesetRenderer::inferredLayer(RtpTileReference(
			RtpTilesetFamily::Outside, RtpTileSheet::A2, 0)) ==
			RtpRenderLayer::Ground &&
		RtpTilesetRenderer::inferredLayer(RtpTileReference(
			RtpTilesetFamily::Outside, RtpTileSheet::B, 0)) ==
			RtpRenderLayer::Decoration &&
		RtpTilesetRenderer::inferredLayer(RtpTileReference(
			RtpTilesetFamily::Dungeon, RtpTileSheet::C, 31)) ==
			RtpRenderLayer::Foreground;
	SDL_Rect tilesetTarget = { 0, 0, 32, 32 };
	for (size_t sheet = 0; sheet < sheets.size() && fullTilesetReady; ++sheet)
	{
		RtpTileReference layerProbe(sheets[sheet].family, sheets[sheet].sheet, 0);
		RtpRenderLayer layer = RtpTilesetRenderer::inferredLayer(layerProbe);
		RtpTileReference first(sheets[sheet].family, sheets[sheet].sheet, 0, layer);
		RtpTileReference last(sheets[sheet].family, sheets[sheet].sheet,
			sheets[sheet].tileCount - 1, layer);
		fullTilesetReady = rtpTiles.draw(first, surrounded, tilesetTarget, 0) &&
			rtpTiles.draw(last, surrounded, tilesetTarget, 2);
	}
	SDL_Point wallQuarter;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::floorQuarterSource(
		0, surrounded & ~RtpTilesetRenderer::NorthWest, wallQuarter) &&
		wallQuarter.x == 2 && wallQuarter.y == 0;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::floorQuarterSource(
		1, surrounded & ~RtpTilesetRenderer::NorthEast, wallQuarter) &&
		wallQuarter.x == 3 && wallQuarter.y == 0;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::floorQuarterSource(
		2, surrounded & ~RtpTilesetRenderer::SouthWest, wallQuarter) &&
		wallQuarter.x == 2 && wallQuarter.y == 1;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::floorQuarterSource(
		3, surrounded & ~RtpTilesetRenderer::SouthEast, wallQuarter) &&
		wallQuarter.x == 3 && wallQuarter.y == 1;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::wallQuarterSource(
		0, surrounded, wallQuarter) && wallQuarter.x == 2 && wallQuarter.y == 2;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::waterfallQuarterSource(
		3, 0, wallQuarter) && wallQuarter.x == 3 && wallQuarter.y == 1;
	SDL_Rect regularTile;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::regularTileSource(
		RtpTileSheet::B, 83, 512, 512, regularTile) &&
		regularTile.x == 96 && regularTile.y == 320;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::regularTileSource(
		RtpTileSheet::A5, 16, 256, 512, regularTile) &&
		regularTile.x == 0 && regularTile.y == 64;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::paletteTileSource(
		RtpTileSheet::A1, 5, regularTile) && regularTile.x == 448 &&
		regularTile.y == 0 && regularTile.w == 64 && regularTile.h == 96;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::paletteTileSource(
		RtpTileSheet::A2, 31, regularTile) && regularTile.x == 448 &&
		regularTile.y == 288 && regularTile.w == 64 && regularTile.h == 96;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::paletteTileSource(
		RtpTileSheet::A4, 8, regularTile) && regularTile.x == 0 &&
		regularTile.y == 96 && regularTile.w == 64 && regularTile.h == 64;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::paletteTileSource(
		RtpTileSheet::B, 128, regularTile) && regularTile.x == 256 &&
		regularTile.y == 0 && regularTile.w == 32 && regularTile.h == 32;
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::isWallOpening(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::B, 67,
			RtpRenderLayer::Decoration)) &&
		!RtpTilesetRenderer::isWallOpening(RtpTileReference(
			RtpTilesetFamily::Outside, RtpTileSheet::B, 225,
			RtpRenderLayer::Decoration));
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A1, 0)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A2, 0)) ==
		RtpTileCollision::Walkable && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A2, 21)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A3, 8)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A4, 0)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A5, 2)) ==
		RtpTileCollision::Walkable && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Inside, RtpTileSheet::A5, 7)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::B, 67)) ==
		RtpTileCollision::Walkable && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::C, 128)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::World, RtpTileSheet::A2, 0)) ==
		RtpTileCollision::Ignore;
	std::vector<RtpTileReference> layeredTiles;
	layeredTiles.push_back(RtpTileReference(RtpTilesetFamily::Outside,
		RtpTileSheet::A5, 16, RtpRenderLayer::Ground));
	layeredTiles.push_back(RtpTileReference(RtpTilesetFamily::Outside,
		RtpTileSheet::B, 178, RtpRenderLayer::Decoration));
	layeredTiles.push_back(RtpTileReference(RtpTilesetFamily::Outside,
		RtpTileSheet::C, 0, RtpRenderLayer::Foreground));
	fullTilesetReady = fullTilesetReady &&
		rtpTiles.drawLayer(layeredTiles, RtpRenderLayer::Ground,
			surrounded, tilesetTarget) &&
		rtpTiles.drawLayer(layeredTiles, RtpRenderLayer::Decoration,
			surrounded, tilesetTarget) &&
		rtpTiles.drawLayer(layeredTiles, RtpRenderLayer::Foreground,
			surrounded, tilesetTarget);
	const std::string semanticTiles = ".=~HT#WDFCBASMQERXGIPVKJUO1234567890"
		"abcdefghijklmnopqrstuvwxyz";
	for (size_t tile = 0; tile < semanticTiles.size() && fullTilesetReady; ++tile)
		fullTilesetReady = WorldTiles::isValid(WorldTiles::fromGlyph(semanticTiles[tile])) &&
			mWorldTileRenderer->drawPreview(WorldTiles::fromGlyph(semanticTiles[tile]),
				tilesetTarget);
	fullTilesetReady = fullTilesetReady && mWorldTileRenderer->drawSignpost(tilesetTarget) &&
		mWorldTileRenderer->drawChest(tilesetTarget, false) &&
		mWorldTileRenderer->drawChest(tilesetTarget, true) &&
		mWorldTileRenderer->drawShard(tilesetTarget);
	spriteSheetsReady = spriteSheetsReady && fullTilesetReady;
	int savedPlayerX = mPlayerX;
	int savedPlayerY = mPlayerY;
	float savedVisualX = mVisualX;
	float savedVisualY = mVisualY;
	int savedIntentX = mMoveIntentX;
	int savedIntentY = mMoveIntentY;
	mMoveIntentX = 1;
	mMoveIntentY = 0;
	updateOverworld(50);
	bool playerInterpolated = mPlayerX == savedPlayerX + 1 &&
		mVisualX > savedVisualX && mVisualX < (float)mPlayerX;
	mPlayerX = savedPlayerX;
	mPlayerY = savedPlayerY;
	mVisualX = savedVisualX;
	mVisualY = savedVisualY;
	mMoveIntentX = savedIntentX;
	mMoveIntentY = savedIntentY;

	int movementNpc = -1;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (npcVisible((int)i) && mNpcs[i].mapId == currentMapId() && mNpcs[i].canWander())
		{
			movementNpc = (int)i;
			break;
		}
	if (movementNpc < 0) return false;
	Npc& npc = mNpcs[movementNpc];
	int savedNpcX = npc.x;
	int savedNpcY = npc.y;
	float savedNpcVisualX = npc.visualX;
	float savedNpcVisualY = npc.visualY;
	int savedNpcFacingX = npc.facingX;
	int savedNpcFacingY = npc.facingY;
	Uint32 savedNextMoveAt = npc.nextMoveAt;
	npc.nextMoveAt = SDL_GetTicks();
	updateOverworld(16);
	int targetDistance = std::abs(npc.x - npc.homeX) + std::abs(npc.y - npc.homeY);
	updateOverworld(50);
	bool npcInterpolated = targetDistance == 1 && npc.isMoving() &&
		(std::fabs(npc.visualX - savedNpcVisualX) > 0.001f ||
		 std::fabs(npc.visualY - savedNpcVisualY) > 0.001f);
	npc.x = savedNpcX;
	npc.y = savedNpcY;
	npc.visualX = savedNpcVisualX;
	npc.visualY = savedNpcVisualY;
	npc.facingX = savedNpcFacingX;
	npc.facingY = savedNpcFacingY;
	npc.nextMoveAt = savedNextMoveAt;

	int savedArea = mCurrentWorldArea;
	int savedOpeningPortal = mOpeningPortal;
	Uint32 savedPortalAnimationStarted = mPortalAnimationStarted;
	int savedPortalPlayerX = mPlayerX;
	int savedPortalPlayerY = mPlayerY;
	float savedPortalVisualX = mVisualX;
	float savedPortalVisualY = mVisualY;
	int savedDialogue = mDialogueNpc;
	int savedDialogueObject = mDialogueObject;
	std::string savedDialogueText = mDialogueText;
	size_t savedDialogueVisibleBytes = mDialogueVisibleBytes;
	Uint32 savedDialogueCharacterAccumulator = mDialogueCharacterAccumulator;
	DialogueAction savedDialogueAction = mDialogueAction;
	std::string savedNotice = mNotice;
	Uint32 savedNoticeUntil = mNoticeUntil;
	bool enteredIndoor = false;
	bool returnedOutside = false;
	for (size_t i = 0; i < mWorld.portals.size() && !enteredIndoor; ++i)
	{
		int destination = worldAreaIndex(mWorld.portals[i].toMap);
		if (destination < 0 || !mWorld.maps[destination].indoor) continue;
		mCurrentWorldArea = worldAreaIndex(mWorld.portals[i].fromMap);
		mPlayerX = mWorld.portals[i].fromX;
		mPlayerY = mWorld.portals[i].fromY;
		mVisualX = (float)mPlayerX;
		mVisualY = (float)mPlayerY;
		enteredIndoor = beginPortalAt(mPlayerX, mPlayerY);
		mPortalAnimationStarted = SDL_GetTicks() - DOOR_OPEN_DURATION;
		updateOverworld(0);
		enteredIndoor = enteredIndoor && mCurrentWorldArea == destination && mOpeningPortal < 0;
		for (size_t reverse = 0; reverse < mWorld.portals.size() && enteredIndoor; ++reverse)
		{
			if (mWorld.portals[reverse].fromMap != mWorld.portals[i].toMap ||
				mWorld.portals[reverse].toMap != mWorld.portals[i].fromMap) continue;
			mPlayerX = mWorld.portals[reverse].fromX;
			mPlayerY = mWorld.portals[reverse].fromY;
			mVisualX = (float)mPlayerX;
			mVisualY = (float)mPlayerY;
			returnedOutside = beginPortalAt(mPlayerX, mPlayerY);
			mPortalAnimationStarted = SDL_GetTicks() - DOOR_OPEN_DURATION;
			updateOverworld(0);
			returnedOutside = returnedOutside && currentMapId() == mWorld.portals[i].fromMap &&
				mOpeningPortal < 0;
			break;
		}
	}
	mCurrentWorldArea = savedArea;
	mOpeningPortal = savedOpeningPortal;
	mPortalAnimationStarted = savedPortalAnimationStarted;
	mPlayerX = savedPortalPlayerX;
	mPlayerY = savedPortalPlayerY;
	mVisualX = savedPortalVisualX;
	mVisualY = savedPortalVisualY;
	mDialogueNpc = savedDialogue;
	mDialogueObject = savedDialogueObject;
	mDialogueText = savedDialogueText;
	mDialogueVisibleBytes = savedDialogueVisibleBytes;
	mDialogueCharacterAccumulator = savedDialogueCharacterAccumulator;
	mDialogueAction = savedDialogueAction;
	mNotice = savedNotice;
	mNoticeUntil = savedNoticeUntil;
	bool mercerIsIndoors = false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].id == "mercer")
		{
			int area = worldAreaIndex(mNpcs[i].mapId);
			mercerIsIndoors = area >= 0 && mWorld.maps[area].indoor;
		}
	TileBounds largeWorldBounds = visibleTileBounds(500.25f, 500.5f, 1024, 1024);
	TileBounds overworldBounds = visibleTileBounds(500.25f, 500.5f, 1024, 1024,
		OVERWORLD_VIEW_COLUMNS, MAP_VIEW_ROWS);
	TileBounds smallMapBounds = visibleTileBounds(0.f, 0.f, 10, 8);
	bool viewportCullingReady = largeWorldBounds.left == 499 &&
		largeWorldBounds.top == 499 && largeWorldBounds.right == 522 &&
		largeWorldBounds.bottom == 514 &&
		largeWorldBounds.tileCount() <= (MAP_VIEW_COLUMNS + 3) * (MAP_VIEW_ROWS + 3) &&
		largeWorldBounds.contains(500, 500) && !largeWorldBounds.contains(100, 100) &&
		largeWorldBounds.intersects(500, 500, 501, 501) &&
		!largeWorldBounds.intersects(0, 0, 10, 10) &&
		overworldBounds.left == 499 && overworldBounds.top == 499 &&
		overworldBounds.right == 527 && overworldBounds.bottom == 514 &&
		overworldBounds.tileCount() <= (OVERWORLD_VIEW_COLUMNS + 3) *
			(MAP_VIEW_ROWS + 3) &&
		smallMapBounds.left == 0 && smallMapBounds.top == 0 &&
		smallMapBounds.right == 10 && smallMapBounds.bottom == 8 &&
		smallMapBounds.tileCount() == 80;
	int overworldArea = worldAreaIndex("overworld");
	const int overworldOffsetX = 288;
	const int overworldOffsetY = 665;
	bool seamlessWorldReady = overworldArea >= 0 && !mWorld.maps[overworldArea].indoor;
	bool naturalTilesReady = WorldTiles::isValid(WorldTiles::Rocks) &&
		WorldTiles::isValid(WorldTiles::Bush) && WorldTiles::isValid(WorldTiles::Shrub) &&
		WorldTiles::isValid(WorldTiles::CaveEntrance) &&
		WorldTiles::isValid(WorldTiles::TreeStump) &&
		!WorldTiles::isWalkable(WorldTiles::Rocks) &&
		!WorldTiles::isWalkable(WorldTiles::Bush) &&
		WorldTiles::isWalkable(WorldTiles::Shrub) &&
		WorldTiles::isWalkable(WorldTiles::CaveEntrance) &&
		!WorldTiles::isWalkable(WorldTiles::TreeStump);
	int playableOutdoorAreas = 0;
	for (size_t area = 0; area < mWorld.maps.size(); ++area)
	{
		if (mWorld.maps[area].indoor) continue;
		for (size_t region = 0; region < mWorld.regions.size(); ++region)
			if (mWorld.regions[region].mapId == mWorld.maps[area].id)
			{
				++playableOutdoorAreas;
				break;
			}
	}
	seamlessWorldReady = seamlessWorldReady && playableOutdoorAreas == 1 &&
		mWorld.maps[overworldArea].catalogOnly &&
		mWorld.maps[overworldArea].width() == 1024 &&
		mWorld.maps[overworldArea].height() == 1024 &&
		mWorld.maps[overworldArea].tiles.size() == 1024 &&
		mWorld.maps[overworldArea].tiles[0].size() == 1024 &&
		!mWorld.maps[overworldArea].tileLayers.empty() &&
		mWorld.maps[overworldArea].hasTag(248 + overworldOffsetX,
			89 + overworldOffsetY, "blackstone_gate");
	int storageArea = worldAreaIndex("mercers_house");
	std::string storageError;
	const std::string storagePath = "Build/catalog-map-smoke.json";
	WorldMap storageRoundTrip;
	bool catalogStorageReady = storageArea >= 0 &&
		CatalogMapStorage::saveMap(storagePath, mWorld.maps[storageArea], storageError) &&
		CatalogMapStorage::loadMap(storagePath, storageRoundTrip, storageError) &&
		storageRoundTrip.id == mWorld.maps[storageArea].id &&
		storageRoundTrip.width() == mWorld.maps[storageArea].width() &&
		storageRoundTrip.height() == mWorld.maps[storageArea].height() &&
		storageRoundTrip.tileLayers.size() == mWorld.maps[storageArea].tileLayers.size();
	std::remove(storagePath.c_str());
	WorldData nativeWorld;
	std::string nativeWorldError;
	bool nativeWorldReady = WorldStorage::load("World/World.json", nativeWorld,
		nativeWorldError) && nativeWorld.maps.size() == mWorld.maps.size() &&
		nativeWorld.regions.size() == mWorld.regions.size() &&
		nativeWorld.portals.size() == mWorld.portals.size() &&
		nativeWorld.npcPositions == mWorld.npcPositions &&
		nativeWorld.objectPositions == mWorld.objectPositions &&
		nativeWorld.shardPositions == mWorld.shardPositions;
	seamlessWorldReady = seamlessWorldReady && catalogStorageReady && nativeWorldReady;
	const WorldRegion* glasswaterRegion = worldRegionAt("overworld",
		43 + overworldOffsetX, 25 + overworldOffsetY);
	const WorldRegion* rootmazeRegion = worldRegionAt("overworld",
		50 + overworldOffsetX, 78 + overworldOffsetY);
	const WorldRegion* watershedRegion = worldRegionAt("overworld",
		130 + overworldOffsetX, 50 + overworldOffsetY);
	const WorldRegion* emberglenRegion = worldRegionAt("overworld",
		241 + overworldOffsetX, 50 + overworldOffsetY);
	const WorldRegion* oldRoadRegion = worldRegionAt("overworld",
		310 + overworldOffsetX, 44 + overworldOffsetY);
	const WorldRegion* cinderrailRegion = worldRegionAt("overworld",
		367 + overworldOffsetX, 57 + overworldOffsetY);
	const WorldRegion* blackstoneRegion = worldRegionAt("overworld",
		248 + overworldOffsetX, 120 + overworldOffsetY);
	seamlessWorldReady = seamlessWorldReady && glasswaterRegion != NULL &&
		rootmazeRegion != NULL &&
		watershedRegion != NULL &&
		emberglenRegion != NULL && oldRoadRegion != NULL && cinderrailRegion != NULL &&
		blackstoneRegion != NULL &&
		glasswaterRegion->id == "glasswater" &&
		rootmazeRegion->id == "rootmaze" &&
		watershedRegion->id == "watershed_crossroads" &&
		watershedRegion->width == 128 && watershedRegion->height == 72 &&
		emberglenRegion->id == "emberglen" && emberglenRegion->x == 512 &&
		emberglenRegion->y == 700 && oldRoadRegion->id == "old_road" &&
		oldRoadRegion->width == 96 && oldRoadRegion->height == 48 &&
		cinderrailRegion->id == "cinderrail" &&
		blackstoneRegion->id == "blackstone_road" &&
		blackstoneRegion->width == 96 && blackstoneRegion->height == 95;
	int savedGateArea = mCurrentWorldArea;
	int savedGateX = mPlayerX;
	int savedGateY = mPlayerY;
	float savedGateVisualX = mVisualX;
	float savedGateVisualY = mVisualY;
	int savedGateFacingX = mFacingX;
	int savedGateFacingY = mFacingY;
	std::string savedGateNotice = mNotice;
	Uint32 savedGateNoticeUntil = mNoticeUntil;
	std::vector<int> savedConfluenceWins(mNpcs.size());
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		savedConfluenceWins[i] = mNpcs[i].wins;
		if (mNpcs[i].crestId == "confluence") mNpcs[i].wins = 0;
	}
	std::string savedFirstCrest = mNpcs[0].crestId;
	mCurrentWorldArea = overworldArea;
	mPlayerX = 248 + overworldOffsetX;
	mPlayerY = 88 + overworldOffsetY;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	tryMove(0, 1);
	bool blackstoneGateReady = mPlayerY == 88 + overworldOffsetY &&
		mNotice.find("Blackstone gate") != std::string::npos;
	mNpcs[0].crestId = "confluence";
	mNpcs[0].wins = 1;
	tryMove(0, 1);
	blackstoneGateReady = blackstoneGateReady &&
		mPlayerX == 248 + overworldOffsetX && mPlayerY == 89 + overworldOffsetY;
	mNpcs[0].crestId = savedFirstCrest;
	for (size_t i = 0; i < mNpcs.size(); ++i) mNpcs[i].wins = savedConfluenceWins[i];
	mCurrentWorldArea = savedGateArea;
	mPlayerX = savedGateX;
	mPlayerY = savedGateY;
	mVisualX = savedGateVisualX;
	mVisualY = savedGateVisualY;
	mFacingX = savedGateFacingX;
	mFacingY = savedGateFacingY;
	mNotice = savedGateNotice;
	mNoticeUntil = savedGateNoticeUntil;
	int savedBannerArea = mCurrentWorldArea;
	int savedBannerPlayerX = mPlayerX;
	int savedBannerPlayerY = mPlayerY;
	float savedBannerVisualX = mVisualX;
	float savedBannerVisualY = mVisualY;
	int savedBannerMoveIntentX = mMoveIntentX;
	std::string savedLastRegionId = mLastWorldRegionId;
	std::string savedBannerName = mRegionBannerName;
	Uint32 savedBannerStarted = mRegionBannerStarted;
	bool savedBannerConnector = mRegionBannerConnector;
	mCurrentWorldArea = overworldArea;
	mPlayerX = 43 + overworldOffsetX;
	mPlayerY = 25 + overworldOffsetY;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	mMoveIntentX = 1;
	mLastWorldRegionId.clear();
	updateRegionBanner();
	bool regionBannerReady = mLastWorldRegionId == "glasswater" &&
		mRegionBannerName == "Glasswater Port" && !mRegionBannerConnector &&
		mMoveIntentX == 1;
	mPlayerX = 130 + overworldOffsetX;
	mPlayerY = 50 + overworldOffsetY;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	updateRegionBanner();
	regionBannerReady = regionBannerReady &&
		mLastWorldRegionId == "watershed_crossroads" &&
		mRegionBannerName == "Watershed Crossroads" && mRegionBannerConnector &&
		mMoveIntentX == 1;
	renderRegionBanner();
	mCurrentWorldArea = savedBannerArea;
	mPlayerX = savedBannerPlayerX;
	mPlayerY = savedBannerPlayerY;
	mVisualX = savedBannerVisualX;
	mVisualY = savedBannerVisualY;
	mMoveIntentX = savedBannerMoveIntentX;
	mLastWorldRegionId = savedLastRegionId;
	mRegionBannerName = savedBannerName;
	mRegionBannerStarted = savedBannerStarted;
	mRegionBannerConnector = savedBannerConnector;
	seamlessWorldReady = seamlessWorldReady && regionBannerReady;
	for (size_t i = 0; i < mWorld.portals.size(); ++i)
	{
		int from = worldAreaIndex(mWorld.portals[i].fromMap);
		int to = worldAreaIndex(mWorld.portals[i].toMap);
		seamlessWorldReady = seamlessWorldReady && from >= 0 && to >= 0 &&
			(mWorld.maps[from].indoor || mWorld.maps[to].indoor);
	}

	const int dx[] = { 1, 0, -1, 0 };
	const int dy[] = { 0, 1, 0, -1 };
	auto reachableFrom = [this, &dx, &dy](int area, int x, int y)
	{
		mCurrentWorldArea = area;
		std::set<std::pair<int, int> > reachable;
		std::vector<std::pair<int, int> > frontier;
		if (!isWalkable(x, y)) return reachable;
		reachable.insert(std::make_pair(x, y));
		frontier.push_back(std::make_pair(x, y));
		for (size_t next = 0; next < frontier.size(); ++next)
			for (int direction = 0; direction < 4; ++direction)
			{
				int nextX = frontier[next].first + dx[direction];
				int nextY = frontier[next].second + dy[direction];
				if (!isWalkable(nextX, nextY) ||
					!reachable.insert(std::make_pair(nextX, nextY)).second) continue;
				frontier.push_back(std::make_pair(nextX, nextY));
			}
		return reachable;
	};
	std::set<std::pair<int, int> > reachable;
	if (seamlessWorldReady)
	{
		reachable = reachableFrom(overworldArea, mWorld.start.x, mWorld.start.y);
		seamlessWorldReady = reachable.count(std::make_pair(
			overworldOffsetX, 19 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(95 + overworldOffsetX,
				39 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(96 + overworldOffsetX,
				39 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(95 + overworldOffsetX,
				61 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(96 + overworldOffsetX,
				61 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(47 + overworldOffsetX,
				107 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(95 + overworldOffsetX,
				97 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(223 + overworldOffsetX,
				53 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(224 + overworldOffsetX,
				53 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(259 + overworldOffsetX,
				47 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(260 + overworldOffsetX,
				47 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(355 + overworldOffsetX,
				48 + overworldOffsetY)) != 0 &&
			reachable.count(std::make_pair(356 + overworldOffsetX,
				48 + overworldOffsetY)) != 0;
	}
	std::set<std::string> landmarkIds;
	for (size_t landmark = 0; landmark < Landmarks::COUNT; ++landmark)
	{
		const Landmarks::Definition& definition = Landmarks::DEFINITIONS[landmark];
		const WorldRegion* region = NULL;
		for (size_t candidate = 0; candidate < mWorld.regions.size(); ++candidate)
			if (mWorld.regions[candidate].mapId == "overworld" &&
				mWorld.regions[candidate].id == definition.regionId)
				region = &mWorld.regions[candidate];
		bool discoverable = false;
		if (region != NULL && definition.localX >= 0 && definition.localY >= 0 &&
			definition.localX < region->width && definition.localY < region->height &&
			definition.discoveryRadius > 0 && definition.goldReward > 0)
		{
			int centerX = region->x + definition.localX;
			int centerY = region->y + definition.localY;
			for (int y = centerY - definition.discoveryRadius;
				y <= centerY + definition.discoveryRadius && !discoverable; ++y)
				for (int x = centerX - definition.discoveryRadius;
					x <= centerX + definition.discoveryRadius; ++x)
					if (std::abs(x - centerX) + std::abs(y - centerY) <=
						definition.discoveryRadius &&
						reachable.count(std::make_pair(x, y)) != 0)
						discoverable = true;
		}
		seamlessWorldReady = seamlessWorldReady && definition.id[0] != '\0' &&
			definition.name[0] != '\0' && landmarkIds.insert(definition.id).second &&
			discoverable;
	}
	for (std::set<std::string>::const_iterator discovered = mDiscoveredLandmarks.begin();
		discovered != mDiscoveredLandmarks.end(); ++discovered)
		seamlessWorldReady = seamlessWorldReady && Landmarks::find(*discovered) != NULL;
	const char* regionalNpcs[] = { "rook", "kipp", "ansa", "holt",
		"neris", "pell", "iri", "sol", "oren", "fern", "toma", "moss",
	};
	const char* expectedRegions[] = { "old_road", "cinderrail", "cinderrail", "cinderrail",
		"glasswater", "glasswater", "glasswater", "glasswater",
		"rootmaze", "rootmaze", "rootmaze", "rootmaze" };
	for (size_t expected = 0; expected < 12; ++expected)
	{
		bool found = false;
		for (size_t i = 0; i < mNpcs.size(); ++i)
			if (mNpcs[i].id == regionalNpcs[expected])
			{
				const WorldRegion* region = worldRegionAt(mNpcs[i].mapId, mNpcs[i].x, mNpcs[i].y);
				found = mNpcs[i].mapId == "overworld" && region != NULL &&
					region->id == expectedRegions[expected] &&
					reachable.count(std::make_pair(mNpcs[i].x, mNpcs[i].y)) != 0 &&
					(mNpcs[i].id != "neris" || mNpcs[i].crestId == "tidal") &&
					(mNpcs[i].id != "oren" || mNpcs[i].crestId == "verdant");
			}
		seamlessWorldReady = seamlessWorldReady && found;
	}
	int routeDuelists = 0;
	int townNpcs = 0;
	int traders = 0;
	int challengeNpc = -1;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		const Npc& npc = mNpcs[i];
		if (npc.isRouteDuelist())
		{
			++routeDuelists;
			if (challengeNpc < 0) challengeNpc = (int)i;
			seamlessWorldReady = seamlessWorldReady && npc.isDuelist() && !npc.canTrade() &&
				npc.canWander() && npc.sightRange >= 1 && npc.sightRange <= 12;
		}
		else if (npc.isTownNpc())
		{
			++townNpcs;
			if (npc.canTrade()) ++traders;
			seamlessWorldReady = seamlessWorldReady && npc.sightRange == 0;
		}
	}
	seamlessWorldReady = seamlessWorldReady && routeDuelists > 0 && townNpcs > 0 &&
		traders > 0 && challengeNpc >= 0;
	int savedChallengeStage = mStoryStage;
	int savedChallengeArea = mCurrentWorldArea;
	int savedChallengePlayerX = mPlayerX;
	int savedChallengePlayerY = mPlayerY;
	float savedChallengeVisualX = mVisualX;
	float savedChallengeVisualY = mVisualY;
	int savedChallengeNpc = mRouteChallengeNpc;
	int savedMenuNpc = mNpcMenuNpc;
	std::set<std::string> savedSuppressedChallenges = mSuppressedRouteChallenges;
	std::string savedChallengeNotice = mNotice;
	Uint32 savedChallengeNoticeUntil = mNoticeUntil;
	Npc& challenger = mNpcs[challengeNpc];
	int savedTrainerX = challenger.x;
	int savedTrainerY = challenger.y;
	float savedTrainerVisualX = challenger.visualX;
	float savedTrainerVisualY = challenger.visualY;
	int savedTrainerWins = challenger.wins;
	mStoryStage = 4;
	mCurrentWorldArea = overworldArea;
	challenger.wins = 0;
	challenger.visualX = (float)challenger.x;
	challenger.visualY = (float)challenger.y;
	mRouteChallengeNpc = -1;
	mNpcMenuNpc = -1;
	mDialogueNpc = -1;
	mDialogueObject = -1;
	mSuppressedRouteChallenges.erase(challenger.id);
	bool diagonalPositionFound = false;
	for (int dy = -challenger.sightRange; dy <= challenger.sightRange &&
		!diagonalPositionFound; ++dy)
		for (int dx = -challenger.sightRange; dx <= challenger.sightRange &&
			!diagonalPositionFound; ++dx)
		{
			int distance = std::abs(dx) + std::abs(dy);
			if (dx == 0 || dy == 0 || distance < 2 || distance > challenger.sightRange)
				continue;
			mPlayerX = challenger.x + dx;
			mPlayerY = challenger.y + dy;
			mVisualX = (float)mPlayerX;
			mVisualY = (float)mPlayerY;
			diagonalPositionFound = isWalkable(mPlayerX, mPlayerY) &&
				npcAt(mPlayerX, mPlayerY, challengeNpc) < 0 &&
				routeDuelistCanCatchPlayer(challengeNpc);
		}
	bool routeRadiusReady = diagonalPositionFound;
	updateRouteDuelistChallenge();
	routeRadiusReady = routeRadiusReady && mRouteChallengeNpc == challengeNpc;
	for (int step = 0; step < challenger.sightRange * 3 + 2 &&
		mDialogueNpc != challengeNpc; ++step)
	{
		int previousX = challenger.x;
		int previousY = challenger.y;
		updateRouteDuelistChallenge();
		if (challenger.x != previousX || challenger.y != previousY)
			routeRadiusReady = routeRadiusReady &&
				std::abs(challenger.x - previousX) + std::abs(challenger.y - previousY) == 1;
		challenger.updateMovement(1000, 7.2f);
	}
	routeRadiusReady = routeRadiusReady && mDialogueNpc == challengeNpc &&
		mDialogueAction == DialogueAction::ForcedBattle && mRouteChallengeNpc < 0;
	clearDialogue();
	challenger.x = savedTrainerX;
	challenger.y = savedTrainerY;
	challenger.visualX = savedTrainerVisualX;
	challenger.visualY = savedTrainerVisualY;
	challenger.wins = savedTrainerWins;
	mStoryStage = savedChallengeStage;
	mCurrentWorldArea = savedChallengeArea;
	mPlayerX = savedChallengePlayerX;
	mPlayerY = savedChallengePlayerY;
	mVisualX = savedChallengeVisualX;
	mVisualY = savedChallengeVisualY;
	mRouteChallengeNpc = savedChallengeNpc;
	mNpcMenuNpc = savedMenuNpc;
	mSuppressedRouteChallenges = savedSuppressedChallenges;
	mNotice = savedChallengeNotice;
	mNoticeUntil = savedChallengeNoticeUntil;
	seamlessWorldReady = seamlessWorldReady && routeRadiusReady;
	bool deckChestReady = false;
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
	{
		const WorldObject& object = mWorldObjects[i];
		if (object.kind != WorldObjectKind::DeckChest) continue;
		const WorldRegion* region = worldRegionAt(object.mapId, object.x, object.y);
		deckChestReady = object.id == "old_road_wayfarer_chest" && region != NULL &&
			region->id == "old_road" && !object.rewardDeck.empty() &&
			object.rewardDeckName == "Wayfarer's Cache" && !object.openedText.empty();
		if (deckChestReady) break;
	}
	seamlessWorldReady = seamlessWorldReady && deckChestReady;
	bool signpostReady = !mWorldObjects.empty();
	if (signpostReady)
	{
		const WorldObject& signpost = mWorldObjects[0];
		int signArea = worldAreaIndex(signpost.mapId);
		int approachX = signpost.x;
		int approachY = signpost.y;
		int faceX = 0;
		int faceY = 0;
		const int directionX[] = { 0, 1, 0, -1 };
		const int directionY[] = { -1, 0, 1, 0 };
		if (signArea < 0) signpostReady = false;
		if (signpostReady) mCurrentWorldArea = signArea;
		for (int direction = 0; direction < 4 && signpostReady && faceX == 0 && faceY == 0;
			++direction)
		{
			int candidateX = signpost.x - directionX[direction];
			int candidateY = signpost.y - directionY[direction];
			if (!isWalkable(candidateX, candidateY) || npcAt(candidateX, candidateY) >= 0 ||
				worldObjectAt(candidateX, candidateY) >= 0) continue;
			approachX = candidateX;
			approachY = candidateY;
			faceX = directionX[direction];
			faceY = directionY[direction];
		}
		signpostReady = signpostReady && (faceX != 0 || faceY != 0);
		if (signpostReady)
		{
			mPlayerX = approachX;
			mPlayerY = approachY;
			mVisualX = (float)approachX;
			mVisualY = (float)approachY;
			mFacingX = faceX;
			mFacingY = faceY;
			tryMove(faceX, faceY);
			signpostReady = mPlayerX == approachX && mPlayerY == approachY;
			interact();
			signpostReady = signpostReady && mDialogueObject == 0 &&
				mDialogueText == signpost.text;
			mDialogueVisibleBytes = mDialogueText.size();
			advanceDialogue();
			signpostReady = signpostReady && mDialogueObject < 0 && mDialogueNpc < 0;
		}
	}
	int savedStoryStage = mStoryStage;
	mCurrentWorldArea = overworldArea;
	mPlayerX = 259 + overworldOffsetX;
	mPlayerY = 47 + overworldOffsetY;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	mStoryStage = 0;
	tryMove(1, 0);
	bool roadGateReady = mPlayerX == 259 + overworldOffsetX;
	mStoryStage = 4;
	tryMove(1, 0);
	roadGateReady = roadGateReady && mPlayerX == 260 + overworldOffsetX;
	mPlayerX = 224 + overworldOffsetX;
	mPlayerY = 53 + overworldOffsetY;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	mStoryStage = 0;
	tryMove(-1, 0);
	bool watershedGateReady = mPlayerX == 224 + overworldOffsetX;
	mStoryStage = 4;
	tryMove(-1, 0);
	watershedGateReady = watershedGateReady && mPlayerX == 223 + overworldOffsetX;
	mStoryStage = savedStoryStage;
	mCurrentWorldArea = savedArea;
	mPlayerX = savedPortalPlayerX;
	mPlayerY = savedPortalPlayerY;
	mVisualX = savedPortalVisualX;
	mVisualY = savedPortalVisualY;
	if (!(worldDataReady && spriteSheetsReady && playerInterpolated && npcInterpolated &&
		enteredIndoor && returnedOutside && mercerIsIndoors && seamlessWorldReady &&
		signpostReady && roadGateReady && watershedGateReady && viewportCullingReady &&
		naturalTilesReady && blackstoneGateReady))
		std::cerr << "Overworld flags: data=" << worldDataReady << " sprites=" <<
			spriteSheetsReady << " player=" << playerInterpolated << " npc=" <<
			npcInterpolated << " portals=" << enteredIndoor << returnedOutside <<
			" mercer=" << mercerIsIndoors << " seamless=" << seamlessWorldReady <<
			" sign=" << signpostReady << " gates=" << roadGateReady << watershedGateReady <<
			" culling=" << viewportCullingReady << " natural=" << naturalTilesReady <<
			" blackstone=" << blackstoneGateReady << std::endl;
	return worldDataReady && spriteSheetsReady && playerInterpolated && npcInterpolated && enteredIndoor && returnedOutside &&
		mercerIsIndoors && seamlessWorldReady && signpostReady && roadGateReady && watershedGateReady &&
		viewportCullingReady && naturalTilesReady && blackstoneGateReady;
}

bool Application::exerciseStorySmoke()
{
	if (mNpcs.size() < 10) return false;
	bool repeatedDeckReady = false;
	bool repeatedRewardReady = false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		Npc& npc = mNpcs[i];
		if (!npc.isDuelist()) continue;
		if (npc.maxWins <= 0 || npc.decks.empty() || npc.rewards.empty()) return false;
		repeatedDeckReady = repeatedDeckReady || npc.maxWins > (int)npc.decks.size();
		repeatedRewardReady = repeatedRewardReady || npc.maxWins > (int)npc.rewards.size();
		int savedNpcWins = npc.wins;
		for (int battle = 0; battle < npc.maxWins; ++battle)
		{
			npc.wins = battle;
			NpcReward reward = npc.nextReward();
			const size_t deckIndex = std::min((size_t)battle, npc.decks.size() - 1);
			const size_t rewardIndex = std::min((size_t)battle, npc.rewards.size() - 1);
			if (npc.battleDeck() != npc.decks[deckIndex] || npc.battleDeck().empty() ||
				reward.card != npc.rewards[rewardIndex].card ||
				reward.gold != npc.rewards[rewardIndex].gold)
			{
				npc.wins = savedNpcWins;
				return false;
			}
		}
		npc.wins = savedNpcWins;
	}
	if (!repeatedDeckReady || !repeatedRewardReady) return false;
	int savedStage = mStoryStage;
	int savedClues = mStoryClues;
	StoryScene savedScene = mStoryScene;
	int savedPage = mStoryScenePage;
	std::vector<int> savedWins;
	for (size_t i = 0; i < mNpcs.size(); ++i) savedWins.push_back(mNpcs[i].wins);

	mStoryStage = 2;
	int regularWins = 0;
	for (size_t i = 0; i < mNpcs.size() && regularWins < 3; ++i)
	{
		if (!mNpcs[i].isTownNpc() || !mNpcs[i].isDuelist()) continue;
		mNpcs[i].wins = 1;
		++regularWins;
	}
	updateStoryProgress();
	bool bossUnlocked = mStoryStage == 3;
	int bossIndex = -1;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].isBoss()) bossIndex = (int)i;
	bool bossVisible = bossIndex >= 0 && npcVisible(bossIndex);
	if (bossIndex >= 0) mNpcs[bossIndex].wins = 1;
	updateStoryProgress();
	bool actCompleted = mStoryStage == 4;

	for (size_t i = 0; i < mNpcs.size(); ++i) mNpcs[i].wins = savedWins[i];
	mStoryStage = savedStage;
	mStoryClues = savedClues;
	mStoryScene = savedScene;
	mStoryScenePage = savedPage;
	return bossUnlocked && bossVisible && actCompleted;
}

bool Application::exerciseMenuScreensSmoke()
{
	mScreen = Screen::Overworld;
	mPauseMenuOpen = true;
	mPauseMenuSelection = 0;
	renderOverworld();
	if (mCrestTextures.size() != 9) return false;
	for (std::map<std::string, SDL_Texture*>::const_iterator texture = mCrestTextures.begin();
		texture != mCrestTextures.end(); ++texture)
		if (texture->second == NULL) return false;
	int aureliaIndex = -1;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].id == "aurelia") aureliaIndex = (int)i;
	if (aureliaIndex < 0 || mNpcs[aureliaIndex].crestId != "dawn") return false;
	int savedAureliaWins = mNpcs[aureliaIndex].wins;
	mNpcs[aureliaIndex].wins = 0;
	bool crestStartsLocked = !hasCrest("dawn");
	mNpcs[aureliaIndex].wins = 1;
	bool crestUnlocks = hasCrest("dawn");
	mNpcs[aureliaIndex].wins = savedAureliaWins;
	if (!crestStartsLocked || !crestUnlocks) return false;
	SDL_Event navigate = {};
	navigate.type = SDL_KEYDOWN;
	navigate.key.keysym.sym = SDLK_DOWN;
	handleOverworldEvent(navigate);
	navigate.key.keysym.sym = SDLK_UP;
	handleOverworldEvent(navigate);
	navigate.key.keysym.sym = SDLK_s;
	handleOverworldEvent(navigate);
	handleOverworldEvent(navigate);
	navigate.key.keysym.sym = SDLK_RETURN;
	handleOverworldEvent(navigate);
	if (mScreen != Screen::DeckBuilder) return false;
	if (gCardDatabase.size() < 11) return false;
	PlayerDeck deckSizeCheck;
	for (int card = 0; card < 10; ++card) deckSizeCheck.cards[card] = 4;
	bool fortyCardsLegal = deckHasMinimumCards(deckSizeCheck);
	deckSizeCheck.cards[10] = 1;
	bool fortyOneCardsLegal = deckHasMinimumCards(deckSizeCheck);
	deckSizeCheck.cards[0] = 3;
	bool fortyCardsStillLegal = deckHasMinimumCards(deckSizeCheck);
	deckSizeCheck.cards.erase(10);
	bool thirtyNineCardsIllegal = !deckHasMinimumCards(deckSizeCheck);
	if (!fortyCardsLegal || !fortyOneCardsLegal || !fortyCardsStillLegal ||
		!thirtyNineCardsIllegal) return false;
	for (size_t i = 0; i < mPlayerDecks.size(); ++i)
		if (!mPlayerDecks[i].path.empty() &&
			mPlayerDecks[i].path.find(playerDeckDirectory() + "/") != 0) return false;
	renderDeckBuilder();
	mMouseX = 300;
	mMouseY = 200;
	renderDeckBuilder();
	if (mDeckHoveredCard < 0) return false;
	mMouseX = 1000;
	mMouseY = 210;
	renderDeckBuilder();
	if (mDeckHoveredCard < 0) return false;
	SDL_Event back = {};
	back.type = SDL_KEYDOWN;
	back.key.keysym.sym = SDLK_ESCAPE;
	handleDeckBuilderEvent(back);
	if (mScreen != Screen::Overworld) return false;

	mPauseMenuOpen = true;
	SDL_Event openSettings = {};
	openSettings.type = SDL_MOUSEBUTTONDOWN;
	openSettings.button.button = SDL_BUTTON_LEFT;
	openSettings.button.x = SETTINGS_BUTTON.x + SETTINGS_BUTTON.w / 2;
	openSettings.button.y = SETTINGS_BUTTON.y + SETTINGS_BUTTON.h / 2;
	handleOverworldEvent(openSettings);
	if (mScreen != Screen::Settings) return false;
	renderSettings();
	handleSettingsEvent(back);
	if (mScreen != Screen::Overworld) return false;

	int mercerIndex = -1;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].id == "mercer") mercerIndex = (int)i;
	if (!mNpcs[aureliaIndex].isTownNpc() || !mNpcs[aureliaIndex].isDuelist() ||
		mNpcs[aureliaIndex].canTrade() || mercerIndex < 0 ||
		!mNpcs[mercerIndex].isTownNpc() || mNpcs[mercerIndex].isDuelist() ||
		!mNpcs[mercerIndex].canTrade()) return false;
	std::vector<NpcMenuAction> aureliaActions = npcMenuActions(aureliaIndex);
	std::vector<NpcMenuAction> mercerActions = npcMenuActions(mercerIndex);
	if (aureliaActions.size() != 3 || aureliaActions[0] != NpcMenuAction::Talk ||
		aureliaActions[1] != NpcMenuAction::Duel ||
		aureliaActions[2] != NpcMenuAction::Leave || mercerActions.size() != 3 ||
		mercerActions[0] != NpcMenuAction::Talk ||
		mercerActions[1] != NpcMenuAction::Trade ||
		mercerActions[2] != NpcMenuAction::Leave) return false;
	beginDialogue(aureliaIndex, mNpcs[aureliaIndex].dialogueText("greeting"),
		DialogueAction::OpenNpcMenu);
	mDialogueVisibleBytes = mDialogueText.size();
	advanceDialogue();
	if (mDialogueNpc >= 0 || mNpcMenuNpc != aureliaIndex) return false;
	renderNpcMenu();
	activateNpcMenuAction(NpcMenuAction::Leave);
	if (mNpcMenuNpc >= 0) return false;
	mNpcMenuNpc = mercerIndex;
	mNpcMenuSelection = 1;
	activateNpcMenuAction(NpcMenuAction::Trade);
	renderShop();
	if (mScreen != Screen::Shop || mShopCardHitboxes.size() != 10) return false;
	SDL_Event shopNavigation = {};
	shopNavigation.type = SDL_KEYDOWN;
	shopNavigation.key.keysym.sym = SDLK_DOWN;
	handleShopEvent(shopNavigation);
	if (shopInventory().size() > 10 && mShopPage != 1) return false;
	shopNavigation.key.keysym.sym = SDLK_UP;
	handleShopEvent(shopNavigation);
	if (mShopPage != 0) return false;
	handleShopEvent(back);
	if (mScreen != Screen::Overworld) return false;

	Screen savedScreen = mScreen;
	WorldBuilderTab savedBuilderTab = mWorldBuilderTab;
	int savedBuilderArea = mCurrentWorldArea;
	float savedBuilderCameraX = mWorldBuilderCameraX;
	float savedBuilderCameraY = mWorldBuilderCameraY;
	int savedBuilderTileSize = mWorldBuilderTileSize;
	int savedBuilderListScroll = mWorldBuilderListScroll;
	int savedBuilderTileCategory = mWorldBuilderTileCategory;
	int savedBuilderTileSheet = mWorldBuilderTileSheet;
	int savedBuilderCatalogTile = mWorldBuilderCatalogTile;
	int savedBuilderSelectedNpc = mWorldBuilderSelectedNpc;
	int savedBuilderSelectedObject = mWorldBuilderSelectedObject;
	bool savedBuilderDirty = mWorldBuilderDirty;
	int savedMouseX = mMouseX;
	int savedMouseY = mMouseY;
	int builderArea = worldAreaIndex("overworld");
	if (builderArea < 0 || mWorldObjects.empty() || mMercerStock.shards.empty()) return false;
	mScreen = Screen::WorldBuilder;
	mCurrentWorldArea = builderArea;
	mWorldBuilderTab = WorldBuilderTab::Tiles;
	mWorldBuilderListScroll = 0;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	mWorldBuilderTileSize = TILE;
	mWorldBuilderListScroll = 0;
	mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
	mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
	mWorldBuilderPanAccumulator = 0;
	int portraitNpc = 0;
	int portraitArea = worldAreaIndex(mNpcs[portraitNpc].mapId);
	if (portraitArea < 0) return false;
	mWorldBuilderTab = WorldBuilderTab::Npcs;
	SDL_Event npcListClick = {};
	npcListClick.type = SDL_MOUSEBUTTONDOWN;
	npcListClick.button.button = SDL_BUTTON_LEFT;
	npcListClick.button.x = 1100;
	npcListClick.button.y = 159;
	npcListClick.button.clicks = 1;
	handleWorldBuilderEvent(npcListClick);
	bool npcDoubleClickReady = mWorldBuilderSelectedNpc == portraitNpc &&
		mCurrentWorldArea == builderArea;
	npcListClick.button.clicks = 2;
	handleWorldBuilderEvent(npcListClick);
	npcDoubleClickReady = npcDoubleClickReady && mCurrentWorldArea == portraitArea;
	mCurrentWorldArea = builderArea;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	mWorldBuilderListScroll = 0;
	mWorldBuilderTab = WorldBuilderTab::Objects;
	SDL_Event objectListClick = npcListClick;
	objectListClick.button.clicks = 1;
	handleWorldBuilderEvent(objectListClick);
	bool objectDoubleClickReady = mWorldBuilderSelectedObject == 0 &&
		mCurrentWorldArea == builderArea && mWorldBuilderCameraX == 10 &&
		mWorldBuilderCameraY == 10;
	objectListClick.button.clicks = 2;
	handleWorldBuilderEvent(objectListClick);
	int objectArea = worldAreaIndex(mWorldObjects[0].mapId);
	objectDoubleClickReady = objectDoubleClickReady && mCurrentWorldArea == objectArea &&
		(mWorldBuilderCameraX != 10 || mWorldBuilderCameraY != 10);
	mCurrentWorldArea = builderArea;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	mWorldBuilderListScroll = (int)mWorldObjects.size();
	objectListClick.button.clicks = 1;
	handleWorldBuilderEvent(objectListClick);
	bool shardListedAsObject = mWorldBuilderSelectedObject == (int)mWorldObjects.size() &&
		mCurrentWorldArea == builderArea && mWorldBuilderCameraX == 10 &&
		mWorldBuilderCameraY == 10;
	objectListClick.button.clicks = 2;
	handleWorldBuilderEvent(objectListClick);
	int shardArea = worldAreaIndex(mMercerStock.shards[0].mapId);
	shardListedAsObject = shardListedAsObject && mCurrentWorldArea == shardArea;
	mWorldBuilderTab = WorldBuilderTab::Npcs;
	mCurrentWorldArea = portraitArea;
	mWorldBuilderCameraX = 0;
	mWorldBuilderCameraY = 0;
	const std::vector<std::string>& portraitMap = currentMap();
	int portraitMapX = MAP_X + std::max(0,
		MAP_VIEW_WIDTH - (int)portraitMap[0].size() * TILE) / 2;
	int portraitMapY = MAP_Y + std::max(0,
		MAP_VIEW_HEIGHT - (int)portraitMap.size() * TILE) / 2;
	mMouseX = portraitMapX + mNpcs[portraitNpc].x * TILE + TILE / 2;
	mMouseY = portraitMapY + mNpcs[portraitNpc].y * TILE + TILE / 2;
	bool npcBuilderViewReady = worldBuilderHoveredNpc() == portraitNpc;
	renderWorldBuilder();
	npcBuilderViewReady = npcBuilderViewReady && !mWorldBuilderTileScaleActive;
	mCurrentWorldArea = builderArea;
	mWorldBuilderTab = WorldBuilderTab::Tiles;
	mWorldBuilderListScroll = 0;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	mMouseX = -100;
	mMouseY = -100;
	renderWorldBuilder();
	mWorldBuilderTileCategory = 0;
	mWorldBuilderTileSheet = (int)RtpTileSheet::A2;
	mWorldBuilderListScroll = 0;
	mMouseX = 1030;
	mMouseY = 264;
	renderWorldBuilder();
	bool tilePaletteReady = mWorldBuilderHoveredTileName == "Ground (Dirt Cave)";
	std::map<std::tuple<int, int, int>, RtpTileReference> savedTileLayers =
		mWorld.maps[builderArea].tileLayers;
	int catalogTestX = -1;
	int catalogTestY = -1;
	for (int row = 0; row < (int)currentMap().size() && catalogTestX < 0; ++row)
		for (int column = 0; column + 1 < (int)currentMap()[row].size(); ++column)
			if (!worldBuilderRequiresWalkable(column, row) &&
				!worldBuilderRequiresWalkable(column + 1, row))
			{
				catalogTestX = column;
				catalogTestY = row;
				break;
			}
	if (catalogTestX < 0) return false;
	mWorldBuilderTileCategory = 2;
	mWorldBuilderTileSheet = (int)RtpTileSheet::A2;
	mWorldBuilderCatalogTile = 1;
	paintWorldBuilderTile(catalogTestX, catalogTestY);
	const RtpTileReference* paintedLayer = worldTileLayer(mWorld.maps[builderArea],
		catalogTestX, catalogTestY, RtpRenderLayer::Ground);
	bool catalogPaintingReady = paintedLayer != NULL &&
		paintedLayer->family == RtpTilesetFamily::Outside &&
		paintedLayer->sheet == RtpTileSheet::A2 && paintedLayer->index == 1 &&
		worldTileWalkable(mWorld.maps[builderArea], catalogTestX, catalogTestY);
	eraseWorldBuilderTile(catalogTestX, catalogTestY);
	catalogPaintingReady = catalogPaintingReady &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Ground) == NULL;
	mWorldBuilderTileSheet = (int)RtpTileSheet::A3;
	mWorldBuilderCatalogTile = 8;
	paintWorldBuilderTile(catalogTestX, catalogTestY);
	catalogPaintingReady = catalogPaintingReady &&
		!worldTileWalkable(mWorld.maps[builderArea], catalogTestX, catalogTestY);
	mWorldBuilderTileSheet = (int)RtpTileSheet::B;
	mWorldBuilderCatalogTile = 67;
	paintWorldBuilderTile(catalogTestX + 1, catalogTestY);
	catalogPaintingReady = catalogPaintingReady &&
		(worldTileConnections(mWorld.maps[builderArea], catalogTestX,
			catalogTestY, RtpRenderLayer::Ground) & RtpTilesetRenderer::East) != 0 &&
		worldTileWalkable(mWorld.maps[builderArea], catalogTestX + 1, catalogTestY);
	mWorldBuilderTileCategory = 0;
	mWorldBuilderTileSheet = (int)RtpTileSheet::C;
	mWorldBuilderCatalogTile = 31;
	paintWorldBuilderTile(catalogTestX, catalogTestY);
	catalogPaintingReady = catalogPaintingReady &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Foreground) != NULL;
	eraseWorldBuilderTile(catalogTestX, catalogTestY);
	catalogPaintingReady = catalogPaintingReady &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Foreground) == NULL &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Ground) != NULL;
	SDL_Event categoryClick = {};
	categoryClick.type = SDL_MOUSEBUTTONDOWN;
	categoryClick.button.button = SDL_BUTTON_LEFT;
	categoryClick.button.x = 1145;
	categoryClick.button.y = 160;
	handleWorldBuilderEvent(categoryClick);
	tilePaletteReady = tilePaletteReady && mWorldBuilderTileCategory == 1 &&
		mWorldBuilderListScroll == 0;
	categoryClick.button.x = 1030;
	categoryClick.button.y = 264;
	handleWorldBuilderEvent(categoryClick);
	tilePaletteReady = tilePaletteReady &&
		mWorldBuilderTileSheet == (int)RtpTileSheet::A1 &&
		mWorldBuilderCatalogTile == 0;
	mWorldBuilderTileCategory = 0;
	mWorldBuilderTileSheet = (int)RtpTileSheet::B;
	mWorldBuilderCatalogTile = 0;
	mWorldBuilderListScroll = 0;
	SDL_Event pan = {};
	pan.type = SDL_KEYDOWN;
	pan.key.keysym.sym = SDLK_RIGHT;
	handleWorldBuilderEvent(pan);
	updateWorldBuilder(160);
	pan.type = SDL_KEYUP;
	handleWorldBuilderEvent(pan);
	float stoppedCameraX = mWorldBuilderCameraX;
	updateWorldBuilder(160);
	bool arrowHeld = std::fabs(stoppedCameraX - 13.f) < 0.001f &&
		std::fabs(mWorldBuilderCameraX - stoppedCameraX) < 0.001f;
	pan = {};
	pan.type = SDL_KEYDOWN;
	pan.key.keysym.sym = SDLK_s;
	handleWorldBuilderEvent(pan);
	updateWorldBuilder(160);
	pan.type = SDL_KEYUP;
	handleWorldBuilderEvent(pan);
	bool wasdHeld = std::fabs(mWorldBuilderCameraY - 13.f) < 0.001f &&
		mWorldBuilderTab == WorldBuilderTab::Tiles;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	mMouseX = MAP_X + MAP_VIEW_WIDTH / 2;
	mMouseY = MAP_Y + MAP_VIEW_HEIGHT / 2;
	SDL_Event zoom = {};
	zoom.type = SDL_MOUSEWHEEL;
	zoom.wheel.y = 1;
	handleWorldBuilderEvent(zoom);
	bool maximumZoomRendered = mWorldBuilderTileSize == TILE;
	zoom.wheel.y = -1;
	handleWorldBuilderEvent(zoom);
	const int ninetyPercentTile = (TILE * 90 + 50) / 100;
	float expectedCameraX = 10.f + MAP_VIEW_WIDTH / (2.f * TILE) -
		MAP_VIEW_WIDTH / (2.f * ninetyPercentTile);
	float expectedCameraY = 10.f + MAP_VIEW_HEIGHT / (2.f * TILE) -
		MAP_VIEW_HEIGHT / (2.f * ninetyPercentTile);
	bool zoomedOut = mWorldBuilderTileSize == ninetyPercentTile &&
		std::fabs(mWorldBuilderCameraX - expectedCameraX) < 0.001f &&
		std::fabs(mWorldBuilderCameraY - expectedCameraY) < 0.001f;
	for (int level = 9; level > 1; --level)
		zoomWorldBuilder(-1, mMouseX, mMouseY);
	renderWorldBuilder();
	const int tenPercentTile = (TILE * 10 + 50) / 100;
	bool minimumZoomRendered = mWorldBuilderTileSize == tenPercentTile &&
		!mWorldBuilderTileScaleActive;
	zoomWorldBuilder(-1, mMouseX, mMouseY);
	minimumZoomRendered = minimumZoomRendered &&
		mWorldBuilderTileSize == tenPercentTile;
	mCurrentWorldArea = portraitArea;
	mWorldBuilderTab = WorldBuilderTab::Npcs;
	mWorldBuilderCameraX = (float)mNpcs[portraitNpc].x;
	mWorldBuilderCameraY = (float)mNpcs[portraitNpc].y;
	renderWorldBuilder();
	minimumZoomRendered = minimumZoomRendered &&
		!mWorldBuilderTileScaleActive;
	mCurrentWorldArea = builderArea;
	mWorldBuilderTab = WorldBuilderTab::Tiles;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	pan = {};
	pan.type = SDL_KEYDOWN;
	pan.key.keysym.sym = SDLK_RIGHT;
	handleWorldBuilderEvent(pan);
	updateWorldBuilder(160);
	pan.type = SDL_KEYUP;
	handleWorldBuilderEvent(pan);
	float expectedScaledPan = 10.f + 3.f * TILE / tenPercentTile;
	bool scaledPanReady = std::fabs(mWorldBuilderCameraX - expectedScaledPan) < 0.001f;
	for (int level = 1; level < 10; ++level)
		zoomWorldBuilder(1, mMouseX, mMouseY);
	zoomWorldBuilder(1, mMouseX, mMouseY);
	renderWorldBuilder();
	maximumZoomRendered = maximumZoomRendered && mWorldBuilderTileSize == TILE &&
		!mWorldBuilderTileScaleActive;
	mMouseX = 1100;
	mMouseY = 300;
	zoom.wheel.y = -1;
	handleWorldBuilderEvent(zoom);
	bool panelWheelPreservesSheetLayout = mWorldBuilderTileSize == TILE &&
		mWorldBuilderListScroll == 0;
	mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
	mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
	mWorldBuilderPanAccumulator = 0;
	mWorldBuilderCameraX = savedBuilderCameraX;
	mWorldBuilderCameraY = savedBuilderCameraY;
	mWorldBuilderTileSize = savedBuilderTileSize;
	mWorldBuilderListScroll = savedBuilderListScroll;
	mWorldBuilderTileCategory = savedBuilderTileCategory;
	mWorldBuilderTileSheet = savedBuilderTileSheet;
	mWorldBuilderCatalogTile = savedBuilderCatalogTile;
	mWorldBuilderSelectedNpc = savedBuilderSelectedNpc;
	mWorldBuilderSelectedObject = savedBuilderSelectedObject;
	mWorld.maps[builderArea].tileLayers = savedTileLayers;
	mWorldBuilderDirty = savedBuilderDirty;
	mMouseX = savedMouseX;
	mMouseY = savedMouseY;
	mWorldBuilderTab = savedBuilderTab;
	mCurrentWorldArea = savedBuilderArea;
	mScreen = savedScreen;
	if (!npcDoubleClickReady || !objectDoubleClickReady || !shardListedAsObject ||
		!npcBuilderViewReady || !tilePaletteReady || !catalogPaintingReady ||
		!arrowHeld || !wasdHeld ||
		!zoomedOut || !maximumZoomRendered || !scaledPanReady ||
		!minimumZoomRendered || !panelWheelPreservesSheetLayout) return false;

	mPendingRewardCardId = getCardIdFromName("Aqua Hulcus");
	mPendingRewardGold = 100;
	if (mPendingRewardCardId < 0 || mNpcs.empty()) return false;
	beginDialogue(0, mNpcs[0].dialogueText("defeat"), DialogueAction::ShowReward);
	SDL_Event advanceRewardDialogue = {};
	advanceRewardDialogue.type = SDL_MOUSEBUTTONDOWN;
	advanceRewardDialogue.button.button = SDL_BUTTON_LEFT;
	advanceRewardDialogue.button.x = 640;
	advanceRewardDialogue.button.y = 700;
	handleEvent(advanceRewardDialogue);
	bool firstClickOnlyRevealed = mDialogueNpc == 0 &&
		mDialogueVisibleBytes == mDialogueText.size() && mRewardCardId == -1;
	handleEvent(advanceRewardDialogue);
	bool rewardWaitedForDialogue = mDialogueNpc == -1 && mRewardCardId >= 0;
	renderRewardPopup();
	SDL_Event dismissReward = {};
	dismissReward.type = SDL_MOUSEBUTTONDOWN;
	dismissReward.button.button = SDL_BUTTON_LEFT;
	dismissReward.button.x = 640;
	dismissReward.button.y = 693;
	handleEvent(dismissReward);
	return firstClickOnlyRevealed && rewardWaitedForDialogue &&
		mRewardCardId == -1 && mRewardGold == 0 && mScreen == Screen::Overworld;
}
