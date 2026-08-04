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
	constexpr Uint32 DOOR_OPEN_DURATION = 400;
	const SDL_Rect GRAVEYARD_NEXT = { 700, 590, 145, 42 };
	const SDL_Rect SETTINGS_BUTTON = { 890, 430, 300, 58 };
}

int Application::runSmokeTests()
{
	const bool smokeTest = true;
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
	mCardHitboxes.clear();
	SDL_Rect overlap = { 100, 100, 80, 120 };
	mCardHitboxes.push_back({ overlap, 11, true, true, true });
	mCardHitboxes.push_back({ overlap, 12, true, true, true });
	mHoverCandidateCard = 11;
	bool immediate = false;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == 11 && immediate;
	mHoverCandidateCard = -1;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == 12;

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
	return valid;
}

bool Application::exerciseOverworldMovementSmoke()
{
	if (mNpcs.empty()) return false;
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
	npc.nextMoveAt = savedNextMoveAt;

	int savedArea = mCurrentWorldArea;
	int savedOpeningPortal = mOpeningPortal;
	Uint32 savedPortalAnimationStarted = mPortalAnimationStarted;
	int savedPortalPlayerX = mPlayerX;
	int savedPortalPlayerY = mPlayerY;
	float savedPortalVisualX = mVisualX;
	float savedPortalVisualY = mVisualY;
	int savedDialogue = mDialogueNpc;
	std::string savedDialogueText = mDialogueText;
	size_t savedDialogueVisibleBytes = mDialogueVisibleBytes;
	Uint32 savedDialogueCharacterAccumulator = mDialogueCharacterAccumulator;
	DialogueAction savedDialogueAction = mDialogueAction;
	std::string savedNotice = mNotice;
	Uint32 savedNoticeUntil = mNoticeUntil;
	bool enteredIndoor = false;
	bool returnedOutside = false;
	for (size_t i = 0; i < mWorldPortals.size() && !enteredIndoor; ++i)
	{
		int destination = worldAreaIndex(mWorldPortals[i].toMap);
		if (destination < 0 || !mWorldAreas[destination].indoor) continue;
		mCurrentWorldArea = worldAreaIndex(mWorldPortals[i].fromMap);
		mPlayerX = mWorldPortals[i].fromX;
		mPlayerY = mWorldPortals[i].fromY;
		mVisualX = (float)mPlayerX;
		mVisualY = (float)mPlayerY;
		enteredIndoor = beginPortalAt(mPlayerX, mPlayerY);
		mPortalAnimationStarted = SDL_GetTicks() - DOOR_OPEN_DURATION;
		updateOverworld(0);
		enteredIndoor = enteredIndoor && mCurrentWorldArea == destination && mOpeningPortal < 0;
		for (size_t reverse = 0; reverse < mWorldPortals.size() && enteredIndoor; ++reverse)
		{
			if (mWorldPortals[reverse].fromMap != mWorldPortals[i].toMap ||
				mWorldPortals[reverse].toMap != mWorldPortals[i].fromMap) continue;
			mPlayerX = mWorldPortals[reverse].fromX;
			mPlayerY = mWorldPortals[reverse].fromY;
			mVisualX = (float)mPlayerX;
			mVisualY = (float)mPlayerY;
			returnedOutside = beginPortalAt(mPlayerX, mPlayerY);
			mPortalAnimationStarted = SDL_GetTicks() - DOOR_OPEN_DURATION;
			updateOverworld(0);
			returnedOutside = returnedOutside && currentMapId() == mWorldPortals[i].fromMap &&
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
			mercerIsIndoors = area >= 0 && mWorldAreas[area].indoor;
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
	bool seamlessWorldReady = overworldArea >= 0 && !mWorldAreas[overworldArea].indoor;
	int outdoorAreas = 0;
	for (size_t area = 0; area < mWorldAreas.size(); ++area)
		if (!mWorldAreas[area].indoor) ++outdoorAreas;
	seamlessWorldReady = seamlessWorldReady && outdoorAreas == 1 &&
		mWorldAreas[overworldArea].tiles.size() == 108 &&
		mWorldAreas[overworldArea].tiles[0].size() == 296;
	std::set<char> worldTiles;
	if (seamlessWorldReady)
		for (size_t row = 0; row < mWorldAreas[overworldArea].tiles.size(); ++row)
			for (size_t column = 0; column < mWorldAreas[overworldArea].tiles[row].size(); ++column)
			{
				char glyph = mWorldAreas[overworldArea].tiles[row][column];
				worldTiles.insert(glyph);
				seamlessWorldReady = seamlessWorldReady &&
					WorldTiles::isValid(WorldTiles::fromGlyph(glyph));
			}
	const std::string requiredWorldTiles = "0123456789abcdefghijklmnopqRXGIPVJKWQMOU";
	for (size_t i = 0; i < requiredWorldTiles.size(); ++i)
		seamlessWorldReady = seamlessWorldReady &&
			worldTiles.count(requiredWorldTiles[i]) != 0;

	const WorldRegion* glasswaterRegion = worldRegionAt("overworld", 43, 25);
	const WorldRegion* rootmazeRegion = worldRegionAt("overworld", 50, 78);
	const WorldRegion* watershedRegion = worldRegionAt("overworld", 130, 50);
	const WorldRegion* emberglenRegion = worldRegionAt("overworld", 177, 50);
	const WorldRegion* oldRoadRegion = worldRegionAt("overworld", 220, 44);
	const WorldRegion* cinderrailRegion = worldRegionAt("overworld", 255, 57);
	seamlessWorldReady = seamlessWorldReady && glasswaterRegion != NULL &&
		rootmazeRegion != NULL &&
		watershedRegion != NULL &&
		emberglenRegion != NULL && oldRoadRegion != NULL && cinderrailRegion != NULL &&
		glasswaterRegion->id == "glasswater" &&
		rootmazeRegion->id == "rootmaze" &&
		watershedRegion->id == "watershed_crossroads" &&
		emberglenRegion->id == "emberglen" && oldRoadRegion->id == "old_road" &&
		cinderrailRegion->id == "cinderrail";
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
	{
		int from = worldAreaIndex(mWorldPortals[i].fromMap);
		int to = worldAreaIndex(mWorldPortals[i].toMap);
		seamlessWorldReady = seamlessWorldReady && from >= 0 && to >= 0 &&
			(mWorldAreas[from].indoor || mWorldAreas[to].indoor);
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
		reachable = reachableFrom(overworldArea, mWorldStartX, mWorldStartY);
		seamlessWorldReady = reachable.count(std::make_pair(0, 19)) != 0 &&
			reachable.count(std::make_pair(95, 39)) != 0 &&
			reachable.count(std::make_pair(96, 39)) != 0 &&
			reachable.count(std::make_pair(95, 61)) != 0 &&
			reachable.count(std::make_pair(96, 61)) != 0 &&
			reachable.count(std::make_pair(47, 107)) != 0 &&
			reachable.count(std::make_pair(95, 97)) != 0 &&
			reachable.count(std::make_pair(159, 53)) != 0 &&
			reachable.count(std::make_pair(160, 53)) != 0 &&
			reachable.count(std::make_pair(195, 47)) != 0 &&
			reachable.count(std::make_pair(196, 47)) != 0 &&
			reachable.count(std::make_pair(243, 48)) != 0 &&
			reachable.count(std::make_pair(244, 48)) != 0;
	}
	const char* regionalNpcs[] = { "rook", "kipp", "ansa", "holt",
		"neris", "pell", "iri", "sol", "oren", "fern", "toma", "moss" };
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
	int savedStoryStage = mStoryStage;
	mCurrentWorldArea = overworldArea;
	mPlayerX = 195;
	mPlayerY = 47;
	mVisualX = 195.f;
	mVisualY = 47.f;
	mStoryStage = 0;
	tryMove(1, 0);
	bool roadGateReady = mPlayerX == 195;
	mStoryStage = 4;
	tryMove(1, 0);
	roadGateReady = roadGateReady && mPlayerX == 196;
	mPlayerX = 160;
	mPlayerY = 53;
	mVisualX = 160.f;
	mVisualY = 53.f;
	mStoryStage = 0;
	tryMove(-1, 0);
	bool watershedGateReady = mPlayerX == 160;
	mStoryStage = 4;
	tryMove(-1, 0);
	watershedGateReady = watershedGateReady && mPlayerX == 159;
	mStoryStage = savedStoryStage;
	mCurrentWorldArea = savedArea;
	mPlayerX = savedPortalPlayerX;
	mPlayerY = savedPortalPlayerY;
	mVisualX = savedPortalVisualX;
	mVisualY = savedPortalVisualY;
	return playerInterpolated && npcInterpolated && enteredIndoor && returnedOutside &&
		mercerIsIndoors && seamlessWorldReady && roadGateReady && watershedGateReady &&
		viewportCullingReady;
}

bool Application::exerciseStorySmoke()
{
	if (mNpcs.size() < 10) return false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		Npc& npc = mNpcs[i];
		if (!npc.isDuelist()) continue;
		if (npc.maxWins <= 0 || npc.maxWins != (int)npc.decks.size() ||
			npc.maxWins != (int)npc.rewards.size()) return false;
		int savedNpcWins = npc.wins;
		for (int battle = 0; battle < npc.maxWins; ++battle)
		{
			npc.wins = battle;
			NpcReward reward = npc.nextReward();
			if (npc.battleDeck() != npc.decks[battle] || npc.battleDeck().empty() ||
				reward.card != npc.rewards[battle].card || reward.gold != npc.rewards[battle].gold)
			{
				npc.wins = savedNpcWins;
				return false;
			}
		}
		npc.wins = savedNpcWins;
	}
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
		if (mNpcs[i].kind != NpcKind::Duelist) continue;
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
			mPlayerDecks[i].path.find("PlayerData/Decks/") != 0) return false;
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

	enterShop();
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
	int savedBuilderCameraX = mWorldBuilderCameraX;
	int savedBuilderCameraY = mWorldBuilderCameraY;
	int builderArea = worldAreaIndex("overworld");
	if (builderArea < 0) return false;
	mScreen = Screen::WorldBuilder;
	mCurrentWorldArea = builderArea;
	mWorldBuilderTab = WorldBuilderTab::Tiles;
	mWorldBuilderCameraX = 10;
	mWorldBuilderCameraY = 10;
	mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
	mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
	mWorldBuilderPanAccumulator = 0;
	renderWorldBuilder();
	SDL_Event pan = {};
	pan.type = SDL_KEYDOWN;
	pan.key.keysym.sym = SDLK_RIGHT;
	handleWorldBuilderEvent(pan);
	updateWorldBuilder(160);
	pan.type = SDL_KEYUP;
	handleWorldBuilderEvent(pan);
	int stoppedCameraX = mWorldBuilderCameraX;
	updateWorldBuilder(160);
	bool arrowHeld = stoppedCameraX == 13 && mWorldBuilderCameraX == stoppedCameraX;
	pan = {};
	pan.type = SDL_KEYDOWN;
	pan.key.keysym.sym = SDLK_s;
	handleWorldBuilderEvent(pan);
	updateWorldBuilder(160);
	pan.type = SDL_KEYUP;
	handleWorldBuilderEvent(pan);
	bool wasdHeld = mWorldBuilderCameraY == 13 &&
		mWorldBuilderTab == WorldBuilderTab::Tiles;
	mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
	mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
	mWorldBuilderPanAccumulator = 0;
	mWorldBuilderCameraX = savedBuilderCameraX;
	mWorldBuilderCameraY = savedBuilderCameraY;
	mWorldBuilderTab = savedBuilderTab;
	mCurrentWorldArea = savedBuilderArea;
	mScreen = savedScreen;
	if (!arrowHeld || !wasdHeld) return false;

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
