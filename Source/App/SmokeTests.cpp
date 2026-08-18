#include "Application.h"

#include "AI/AiParams.h"
#include "AI/AiScoring.h"
#include "AI/AiDriver.h"
#include "AI/DecisionPlan.h"
#include "AI/HeuristicBot.h"
#include "AI/Mcts.h"
#include "AppSupport.h"
#include "CatalogMapStorage.h"
#include "Game/Card.h"
#include "Game/Deck.h"
#include "Landmarks.h"
#include "LuaInclude.h"
#include "RtpTilesetRenderer.h"
#include "SpriteSheetRenderer.h"
#include "WorldStorage.h"
#include "WorldTileRenderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <set>
#include <sys/stat.h>

using namespace AppSupport;

namespace
{
	constexpr Uint32 DOOR_OPEN_DURATION = 400;
	const SDL_Rect GRAVEYARD_NEXT = { 700, 590, 145, 42 };
	const SDL_Rect SETTINGS_BUTTON = { 890, 502, 300, 58 };

	bool collectDeckFiles(const std::string& directory, std::vector<std::string>& files)
	{
		DIR* folder = opendir(directory.c_str());
		if (folder == NULL)
		{
			std::cerr << "Unable to open bundled deck directory '" << directory << "'." << std::endl;
			return false;
		}
		bool valid = true;
		for (dirent* entry = readdir(folder); entry != NULL; entry = readdir(folder))
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..") continue;
			std::string path = directory + "/" + name;
			struct stat status;
			if (lstat(path.c_str(), &status) != 0)
			{
				std::cerr << "Unable to inspect bundled deck path '" << path << "'." << std::endl;
				valid = false;
			}
			else if (S_ISDIR(status.st_mode))
			{
				if (!collectDeckFiles(path, files)) valid = false;
			}
			else if (S_ISREG(status.st_mode))
			{
				files.push_back(path);
			}
		}
		closedir(folder);
		return valid;
	}

	bool validateShopStockCardList(lua_State* state, int shopTable, const char* field,
		const std::string& shopId, std::string& error)
	{
		shopTable = lua_absindex(state, shopTable);
		lua_getfield(state, shopTable, field);
		if (!lua_istable(state, -1))
		{
			error = "shop '" + shopId + "' is missing " + field;
			lua_pop(state, 1);
			return false;
		}
		const size_t count = lua_rawlen(state, -1);
		if (std::string(field) == "initial_stock" && count == 0)
		{
			error = "shop '" + shopId + "' has empty initial stock";
			lua_pop(state, 1);
			return false;
		}
		for (size_t index = 1; index <= count; ++index)
		{
			lua_rawgeti(state, -1, (lua_Integer)index);
			if (!lua_isstring(state, -1))
			{
				error = "shop '" + shopId + "' has a non-string card in " + field;
				lua_pop(state, 2);
				return false;
			}
			const std::string cardName = lua_tostring(state, -1);
			if (getCardIdFromName(cardName) < 0)
			{
				error = "shop '" + shopId + "' has unknown card '" + cardName + "'";
				lua_pop(state, 2);
				return false;
			}
			lua_pop(state, 1);
		}
		lua_pop(state, 1);
		return true;
	}
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
		if (!exerciseShopStockSmoke())
		{
			std::cerr << "Shop stock card-name smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseDuelCloneSmoke())
		{
			std::cerr << "Duel clone smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseModifierDestroySmoke())
		{
			std::cerr << "Modifier destruction smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseSimulationChoiceSmoke())
		{
			std::cerr << "Simulation choice smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseLiveAndBreatheSmoke())
		{
			std::cerr << "Live and Breathe smoke test failed." << std::endl;
			return 2;
		}
		if (!exercisePoppleTapAbilitySmoke())
		{
			std::cerr << "Popple tap-ability smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseDecisionPlanSmoke())
		{
			std::cerr << "Decision-plan smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseMotorcycleMutantSmoke())
		{
			std::cerr << "Motorcycle Mutant smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseAvalancheGiantSmoke())
		{
			std::cerr << "Avalanche Giant smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseMctsSmoke())
		{
			std::cerr << "MCTS smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseLiveDecisionPlanSmoke())
		{
			std::cerr << "Live decision-plan smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseAiDriverSmoke())
		{
			std::cerr << "AI driver smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseBackgroundMctsSmoke())
		{
			std::cerr << "Background MCTS smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseNpcRewardsSmoke())
		{
			std::cerr << "NPC reward-tier smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseAtmosphereSmoke())
		{
			std::cerr << "Day/night and weather smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseWorldObjectsSmoke())
		{
			std::cerr << "World-object template smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseHollowCardsSmoke())
		{
			std::cerr << "Hollow-card rules smoke test failed." << std::endl;
			return 2;
		}
		if (!exerciseBundledDecksSmoke())
		{
			std::cerr << "Bundled deck smoke test failed." << std::endl;
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
		if (!exerciseUntapAfterBlockSmoke())
		{
			std::cerr << "Untap-after-block smoke test failed." << std::endl;
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
			if (smokeNpc == 0 && smokeFrames == 36 && !exerciseKnockoutScoringSmoke())
			{
				std::cerr << "Knockout scoring smoke test failed." << std::endl;
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

bool Application::exerciseShopStockSmoke()
{
	lua_State* state = luaL_newstate();
	if (state == NULL) return false;
	luaL_openlibs(state);
	std::string error;
	bool valid = luaL_loadfile(state, "Lua/ShopStock.lua") == LUA_OK;
	if (valid) valid = lua_pcall(state, 0, 1, 0) == LUA_OK;
	if (!valid)
	{
		const char* message = lua_tostring(state, -1);
		error = message == NULL ? "unknown Lua error" : message;
	}
	if (valid && !lua_istable(state, -1))
	{
		error = "ShopStock.lua must return a table";
		valid = false;
	}
	if (valid)
	{
		const int root = lua_gettop(state);
		lua_getfield(state, root, "shops");
		if (!lua_istable(state, -1))
		{
			error = "ShopStock.lua is missing shops";
			valid = false;
		}
		else if (lua_rawlen(state, -1) == 0)
		{
			error = "ShopStock.lua has no shops";
			valid = false;
		}
		else
		{
			const size_t shopCount = lua_rawlen(state, -1);
			for (size_t index = 1; index <= shopCount && valid; ++index)
			{
				lua_rawgeti(state, -1, (lua_Integer)index);
				const int shopTable = lua_gettop(state);
				if (!lua_istable(state, shopTable))
				{
					error = "ShopStock.lua has a non-table shop entry";
					valid = false;
				}
				else
				{
					lua_getfield(state, shopTable, "id");
					const std::string shopId = lua_isstring(state, -1) ?
						lua_tostring(state, -1) : "";
					lua_pop(state, 1);
					if (shopId.empty())
					{
						error = "ShopStock.lua has a shop without an id";
						valid = false;
					}
					else if (!validateShopStockCardList(state, shopTable, "initial_stock",
						shopId, error) ||
						!validateShopStockCardList(state, shopTable, "act_iii_bonus",
						shopId, error))
						valid = false;
				}
				lua_pop(state, 1);
			}
		}
		lua_pop(state, 1);
	}
	if (!valid) std::cerr << error << std::endl;
	lua_close(state);
	return valid;
}

bool Application::exerciseDuelCloneSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int creatureId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (creatureId < 0 || LuaCards == NULL || lua_gettop(LuaCards) != 0)
		return false;

	Duel* savedActiveDuel = ActiveDuel;
	bool valid = true;
	{
		Duel source;
		Duel clone;
		source.mIsSimulation = false;
		clone.mIsSimulation = true;
		clone.mInputLoopRunning = false;
		ActiveDuel = &source;
		for (int uid = 0; uid < 5; ++uid)
		{
			Card* card = new Card(uid, creatureId, uid % 2);
			source.mCardList.push_back(card);
		}

		Card* base = source.mCardList[0];
		Card* evolution = source.mCardList[1];
		Card* hand = source.mCardList[2];
		Card* shield = source.mCardList[3];
		Card* deck = source.mCardList[4];
		base->mZone = ZONE_EVOLVED;
		evolution->mZone = ZONE_BATTLE;
		evolution->mEvoStack.push_back(base);
		evolution->mPower = 12345;
		evolution->mManaCost = 7;
		evolution->mBreaker = 3;
		evolution->mIsBlocker = 1;
		evolution->mIsShieldTrigger = 1;
		evolution->mIsTapped = true;
		evolution->mIsFlipped = true;
		evolution->mSummoningSickness = 0;
		evolution->mIsVisible[0] = false;
		evolution->mIsVisible[1] = true;
		source.mBattlezones[1].mCards.push_back(evolution);
		hand->mZone = ZONE_HAND;
		source.mHands[0].mCards.push_back(hand);
		shield->mZone = ZONE_SHIELD;
		source.mShields[1].mCards.push_back(shield);
		source.mShields[1].mSlotsUsed = 5;
		deck->mZone = ZONE_DECK;
		source.mDecks[0].mCards.push_back(deck);
		source.mHands[0].mMyPlayer = 1;

		int stackTop = lua_gettop(LuaCards);
		if (luaL_loadstring(LuaCards, "return function(card, modifier) end") != LUA_OK ||
			lua_pcall(LuaCards, 0, 1, 0) != LUA_OK || !lua_isfunction(LuaCards, -1))
		{
			lua_settop(LuaCards, stackTop);
			ActiveDuel = savedActiveDuel;
			return false;
		}
		int modifierRef = luaL_ref(LuaCards, LUA_REGISTRYINDEX);
		Modifier* modifier = new Modifier(modifierRef);
		modifier->setLuaRuleState("counter", 9);
		evolution->mModifiers.push_back(modifier);

		source.mDeckNames[0] = "clone-player";
		source.mDeckNames[1] = "clone-rival";
		Message historyMessage("cardtap");
		historyMessage.addValue("card", 1);
		MsgHistoryItem historyItem;
		historyItem.msg = historyMessage;
		historyItem.move = 4;
		source.mMessageHistory.push_back(historyItem);
		source.mMoveHistory.push_back(historyMessage);
		source.mMovePlayers.push_back(1);
		source.mCurrentMoveCount = 4;
		source.mRandomGen.SetRandomSeed(982451653);
		source.mRandomGen.Random(1000);
		source.mAttacker = 1;
		source.mDefender = 3;
		source.mDefenderType = DEFENDER_PLAYER;
		source.mBreakCount = 2;
		source.mShieldTargets.push_back(3);
		source.mShieldBreakersThisTurn[1].insert(1);
		source.mShieldsBrokenThisTurn[1] = 3;
		source.mCardsDrawnThisTurn[0] = 2;
		source.mCardsDrawnThisTurn[1] = 1;
		source.setLuaRuleState("clone-test", 1, 17);
		source.mAttackphase = PHASE_TARGET;
		source.mCastingCard = 2;
		source.mCastingCivilizations = 3;
		source.mCastingCost = 4;
		source.mCastingEvobait = 0;
		source.mCastingEvobait2 = 3;
		source.mCastingManaCards.push_back(4);
		source.mWinner = -1;
		source.mNextUniqueId = 5;
		source.mCurrentMessage = historyMessage;
		source.mTurn = 1;
		source.mTurnPhase = TURN_PHASE_ATTACK;
		source.mManaUsed = 1;
		source.mPlayerType[0] = PLAYER_AI;
		source.mPlayerType[1] = PLAYER_HUMAN;

		valid = clone.copyFrom(source);
		if (valid)
		{
			for (size_t index = 0; index < source.mCardList.size(); ++index)
			{
				Card* original = source.mCardList[index];
				Card* copied = clone.mCardList[index];
				valid = valid && original != copied && original->mUniqueId == copied->mUniqueId &&
					original->mCardId == copied->mCardId && original->mName == copied->mName &&
					original->mRace == copied->mRace && original->mCivilization == copied->mCivilization &&
					original->mCivilizations == copied->mCivilizations && original->mType == copied->mType &&
					original->mManaCost == copied->mManaCost && original->mPower == copied->mPower &&
					original->mBreaker == copied->mBreaker && original->mOwner == copied->mOwner &&
					original->mZone == copied->mZone && original->mIsBlocker == copied->mIsBlocker &&
					original->mIsShieldTrigger == copied->mIsShieldTrigger &&
					original->mIsTapped == copied->mIsTapped && original->mIsFlipped == copied->mIsFlipped &&
					original->mSummoningSickness == copied->mSummoningSickness &&
					original->mIsVisible[0] == copied->mIsVisible[0] &&
					original->mIsVisible[1] == copied->mIsVisible[1];
			}
			valid = valid && clone.mBattlezones[1].mCards.size() == 1 &&
				clone.mBattlezones[1].mCards[0] == clone.mCardList[1] &&
				clone.mHands[0].mCards.size() == 1 && clone.mHands[0].mCards[0] == clone.mCardList[2] &&
				clone.mShields[1].mCards.size() == 1 && clone.mShields[1].mCards[0] == clone.mCardList[3] &&
				clone.mDecks[0].mCards.size() == 1 && clone.mDecks[0].mCards[0] == clone.mCardList[4] &&
				clone.mHands[0].mMyPlayer == 1 && clone.mShields[1].mSlotsUsed == 5 &&
				clone.mDecks[0].mRandomGen == &clone.mRandomGen &&
				clone.mDecks[1].mRandomGen == &clone.mRandomGen &&
				clone.mCardList[1]->mEvoStack.size() == 1 &&
				clone.mCardList[1]->mEvoStack[0] == clone.mCardList[0];

			Modifier* clonedModifier = clone.mCardList[1]->mModifiers.empty() ? NULL :
				clone.mCardList[1]->mModifiers[0];
			valid = valid && clonedModifier != NULL && clonedModifier != modifier &&
				clonedModifier->mFuncRef != modifier->mFuncRef &&
				clonedModifier->getLuaRuleState("counter", -1) == 9;
			if (clonedModifier != NULL)
			{
				lua_rawgeti(LuaCards, LUA_REGISTRYINDEX, clonedModifier->mFuncRef);
				valid = valid && lua_isfunction(LuaCards, -1);
				lua_pop(LuaCards, 1);
			}

			valid = valid && clone.mDeckNames[0] == source.mDeckNames[0] &&
				clone.mDeckNames[1] == source.mDeckNames[1] && clone.mIsSimulation &&
				!clone.mInputLoopRunning.load() &&
				clone.mMessageHistory.size() == 1 && clone.mMessageHistory[0].move == 4 &&
				clone.mMessageHistory[0].msg.map == historyMessage.map &&
				clone.mMoveHistory.size() == 1 && clone.mMoveHistory[0].map == historyMessage.map &&
				clone.mMovePlayers == source.mMovePlayers && clone.mCurrentMoveCount == 4 &&
				clone.mAttacker == 1 && clone.mDefender == 3 && clone.mDefenderType == DEFENDER_PLAYER &&
				clone.mBreakCount == 2 && clone.mShieldTargets == source.mShieldTargets &&
				clone.mShieldBreakersThisTurn[0].empty() && clone.mShieldBreakersThisTurn[1].count(1) == 1 &&
				clone.mShieldsBrokenThisTurn[0] == 0 && clone.mShieldsBrokenThisTurn[1] == 3 &&
				clone.mCardsDrawnThisTurn[0] == 2 && clone.mCardsDrawnThisTurn[1] == 1 &&
				clone.getLuaRuleState("clone-test", 1, -1) == 17 && clone.mAttackphase == PHASE_TARGET &&
				clone.mCastingCard == 2 && clone.mCastingCivilizations == 3 && clone.mCastingCost == 4 &&
				clone.mCastingEvobait == 0 && clone.mCastingEvobait2 == 3 &&
				clone.mCastingManaCards == source.mCastingManaCards && clone.mWinner == -1 &&
				clone.mNextUniqueId == 5 && clone.mCurrentMessage.map == historyMessage.map &&
				clone.mTurn == 1 && clone.mTurnPhase == TURN_PHASE_ATTACK && clone.mManaUsed == 1 &&
				clone.mPlayerType[0] == PLAYER_AI && clone.mPlayerType[1] == PLAYER_HUMAN &&
				!clone.mLuaCallbackSuspended.load() && !clone.mIsChoiceActive && clone.mChoice == NULL &&
				clone.mChoiceValidCards.empty() && clone.mMsgMngr.messages.empty() &&
				source.mRandomGen.Random(1000000) == clone.mRandomGen.Random(1000000);

			clone.mCardList[1]->mPower++;
			clone.mCardList[1]->mEvoStack.clear();
			clone.mCardList[1]->mModifiers[0]->setLuaRuleState("counter", 99);
			clone.mBattlezones[1].mCards.clear();
			clone.mShieldsBrokenThisTurn[1] = 99;
			clone.setLuaRuleState("clone-test", 1, 88);
			valid = valid && source.mCardList[1]->mPower == 12345 &&
				source.mCardList[1]->mEvoStack.size() == 1 &&
				modifier->getLuaRuleState("counter", -1) == 9 &&
				source.mBattlezones[1].mCards.size() == 1 &&
				source.mShieldsBrokenThisTurn[1] == 3 &&
				source.getLuaRuleState("clone-test", 1, -1) == 17;

			Card* cloneFirstCard = clone.mCardList[0];
			Message queued("queued-test");
			source.mMsgMngr.sendMessage(queued);
			valid = valid && !clone.copyFrom(source) && clone.mCardList[0] == cloneFirstCard;
			source.mMsgMngr.dispatch();
			source.mLuaCallbackSuspended = true;
			valid = valid && !clone.copyFrom(source) && clone.mCardList[0] == cloneFirstCard;
			source.mLuaCallbackSuspended = false;
			valid = valid && clone.copyFrom(source) && clone.mCardList[1]->mPower == 12345 &&
				clone.mCardList[1]->mEvoStack.size() == 1 &&
				clone.mCardList[1]->mModifiers[0]->getLuaRuleState("counter", -1) == 9;
		}
		valid = valid && lua_gettop(LuaCards) == stackTop;
		ActiveDuel = savedActiveDuel;
	}
	return valid;
}

bool Application::exerciseModifierDestroySmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int creatureId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (creatureId < 0 || LuaCards == NULL || lua_gettop(LuaCards) != 0) return false;

	Duel duel;
	duel.mIsSimulation = true;
	duel.mInputLoopRunning = false;
	Card* card = new Card(0, creatureId, 0);
	duel.mCardList.push_back(card);
	duel.mBattlezones[0].addCard(card);
	card->mZone = ZONE_BATTLE;
	duel.mNextUniqueId = 1;

	int stackTop = lua_gettop(LuaCards);
	auto addModifier = [card, stackTop](bool expires) -> Modifier*
	{
		const char* script = expires ?
			"return function(cid, mid) "
			"if getMessageType() == 'post modifier-expiry-test' then "
			"destroyModifier(cid, mid) end end" :
			"return function(cid, mid) end";
		if (luaL_loadstring(LuaCards, script) != LUA_OK ||
			lua_pcall(LuaCards, 0, 1, 0) != LUA_OK || !lua_isfunction(LuaCards, -1))
		{
			lua_settop(LuaCards, stackTop);
			return static_cast<Modifier*>(NULL);
		}
		Modifier* modifier = new Modifier(luaL_ref(LuaCards, LUA_REGISTRYINDEX));
		card->mModifiers.push_back(modifier);
		return modifier;
	};

	Modifier* firstExpired = addModifier(true);
	Modifier* retained = addModifier(false);
	Modifier* lastExpired = addModifier(true);
	if (firstExpired == NULL || retained == NULL || lastExpired == NULL)
	{
		lua_settop(LuaCards, stackTop);
		return false;
	}

	Message expire("modifier-expiry-test");
	duel.mMsgMngr.sendMessage(expire);
	{
		ActiveDuelGuard activeGuard(duel);
		duel.dispatchAllMessages();
	}
	bool valid = card->mModifiers.size() == 1 && card->mModifiers[0] == retained &&
		duel.mMsgMngr.messages.empty() && lua_gettop(LuaCards) == stackTop;
	return valid;
}

bool Application::exerciseSimulationChoiceSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int creatureId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (creatureId < 0 || LuaCards == NULL || lua_gettop(LuaCards) != 0)
		return false;

	Duel* savedActiveDuel = ActiveDuel;
	bool valid = true;
	{
		Duel simulation;
		Duel nested;
		simulation.mIsSimulation = true;
		for (int uid = 0; uid < 3; ++uid)
		{
			Card* card = new Card(uid, creatureId, 0);
			simulation.mCardList.push_back(card);
			simulation.mHands[0].addCard(card);
			card->mZone = ZONE_HAND;
		}
		simulation.mNextUniqueId = 3;
		Message outerChoiceMessage("post cardmove");
		outerChoiceMessage.addValue("card", 0);
		outerChoiceMessage.addValue("from", ZONE_HAND);
		outerChoiceMessage.addValue("to", ZONE_BATTLE);
		simulation.mCurrentMessage = outerChoiceMessage;

		std::vector<int> answers;
		answers.push_back(1);
		answers.push_back(2);
		size_t nextAnswer = 0;
		simulation.setChoiceResolver(
			[&](const Duel& position) -> int
			{
				valid = valid && ActiveDuel == &simulation && !position.mLuaCallbackSuspended.load() &&
					position.mChoice != NULL && position.mChoicePlayer == 0 &&
					position.mChoiceValidCards.size() == 1 && nextAnswer < answers.size() &&
					position.mChoiceValidCards[0] == answers[nextAnswer];
				// Mimic the live wait loop, which dispatches choiceselect while the
				// containing Lua callback is suspended.
				simulation.mCurrentMessage = Message("post choiceselect");
				return nextAnswer < answers.size() ? answers[nextAnswer++] : RETURN_QUIT;
			});

		int stackTop = lua_gettop(LuaCards);
		{
			ActiveDuelGuard simulationGuard(simulation);
			valid = valid && ActiveDuel == &simulation;
			{
				ActiveDuelGuard nestedGuard(nested);
				valid = valid && ActiveDuel == &nested;
			}
			valid = valid && ActiveDuel == &simulation;

			const char* twoChoiceScript =
				"local first = createChoice('first target', 0, 0, 0, "
				"function(cid, sid) return sid == 1 and 1 or 0 end) "
				"local second = createChoice('second target', 0, 0, 0, "
				"function(cid, sid) return first == 1 and sid == 2 and 1 or 0 end) "
				"return first, second";
			int status = luaL_loadstring(LuaCards, twoChoiceScript);
			if (status == LUA_OK)
				status = lua_pcall(LuaCards, 0, 2, 0);
			if (status != LUA_OK)
			{
				const char* error = lua_tostring(LuaCards, -1);
				std::cerr << "Simulation choice Lua error: " <<
					(error == NULL ? "unknown error" : error) << std::endl;
				valid = false;
			}
			else
			{
				valid = valid && lua_tointeger(LuaCards, -2) == 1 &&
					lua_tointeger(LuaCards, -1) == 2;
			}
			lua_settop(LuaCards, stackTop);
		}
		valid = valid && ActiveDuel == savedActiveDuel && nextAnswer == answers.size() &&
			!simulation.hasSimulationChoiceFailure() && !simulation.mIsChoiceActive &&
			simulation.mChoice == NULL && simulation.mChoiceValidCards.empty() &&
			!simulation.mLuaCallbackSuspended.load() && simulation.mMsgMngr.messages.empty() &&
			simulation.mCurrentMessage.map == outerChoiceMessage.map;

		Duel restored;
		restored.mIsSimulation = true;
		int restoredResolverCalls = 0;
		restored.setChoiceResolver(
			[&](const Duel&) -> int
			{
				++restoredResolverCalls;
				return RETURN_BUTTON1;
			});
		valid = valid && restored.copyFrom(simulation);
		{
			ActiveDuelGuard restoredGuard(restored);
			int status = luaL_loadstring(LuaCards,
				"return createChoiceNoCheck('restored resolver', 1, 0, 0, function() return 0 end)");
			if (status == LUA_OK)
				status = lua_pcall(LuaCards, 0, 1, 0);
			valid = valid && status == LUA_OK && lua_tointeger(LuaCards, -1) == RETURN_BUTTON1;
			lua_settop(LuaCards, stackTop);
		}
		valid = valid && restoredResolverCalls == 1 && !restored.hasSimulationChoiceFailure() &&
			!restored.mIsChoiceActive && restored.mChoice == NULL;

		simulation.setChoiceResolver([](const Duel&) { return 2; });
		{
			ActiveDuelGuard simulationGuard(simulation);
			const char* invalidChoiceScript =
				"return createChoice('invalid target', 0, 0, 0, "
				"function(cid, sid) return sid == 1 and 1 or 0 end)";
			int status = luaL_loadstring(LuaCards, invalidChoiceScript);
			if (status == LUA_OK)
				status = lua_pcall(LuaCards, 0, 1, 0);
			valid = valid && status == LUA_OK && lua_tointeger(LuaCards, -1) == RETURN_QUIT;
			lua_settop(LuaCards, stackTop);
		}
		valid = valid && simulation.hasSimulationChoiceFailure() &&
			!simulation.mIsChoiceActive && simulation.mChoice == NULL;

		simulation.clearSimulationChoiceFailure();
		simulation.clearChoiceResolver();
		{
			ActiveDuelGuard simulationGuard(simulation);
			int status = luaL_loadstring(LuaCards,
				"return createChoiceNoCheck('missing answer', 1, 0, 0, function() return 0 end)");
			if (status == LUA_OK)
				status = lua_pcall(LuaCards, 0, 1, 0);
			valid = valid && status == LUA_OK && lua_tointeger(LuaCards, -1) == RETURN_QUIT;
			lua_settop(LuaCards, stackTop);
		}
		valid = valid && simulation.hasSimulationChoiceFailure() &&
			!simulation.mIsChoiceActive && simulation.mChoice == NULL &&
			!simulation.mLuaCallbackSuspended.load() && lua_gettop(LuaCards) == stackTop;
	}
	ActiveDuel = savedActiveDuel;
	return valid;
}

bool Application::exerciseLiveAndBreatheSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int spellId = getCardIdFromName("Live and Breathe");
	int creatureId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (spellId < 0 || creatureId < 0) return false;

	Duel duel;
	duel.mIsSimulation = true;
	duel.mInputLoopRunning = false;
	auto addCard = [&duel](int cardId, int zone) -> int
	{
		int uid = (int)duel.mCardList.size();
		Card* card = new Card(uid, cardId, 0);
		duel.mCardList.push_back(card);
		duel.getZone(0, zone)->addCard(card);
		card->mZone = zone;
		duel.mNextUniqueId = uid + 1;
		return uid;
	};

	int spell = addCard(spellId, ZONE_HAND);
	int summoned = addCard(creatureId, ZONE_HAND);
	for (int copy = 0; copy < 3; ++copy) addCard(creatureId, ZONE_DECK);

	auto chooseFirst = [](const Duel& position) -> int
	{
		return position.mChoiceValidCards.empty() ? RETURN_QUIT :
			position.mChoiceValidCards.front();
	};
	duel.setChoiceResolver(chooseFirst, 1);
	{
		ActiveDuelGuard activeGuard(duel);
		Message cast("cardmove");
		cast.addValue("card", spell);
		cast.addValue("from", ZONE_HAND);
		cast.addValue("to", ZONE_BATTLE);
		duel.mMsgMngr.sendMessage(cast);
		duel.dispatchAllMessages();

		Message summon("cardplay");
		summon.addValue("card", summoned);
		summon.addValue("evobait", -1);
		summon.addValue("evobait2", -1);
		duel.mMsgMngr.sendMessage(summon);
		duel.dispatchAllMessages();
	}
	bool valid = !duel.mSimulationChoiceFailed && !duel.mIsChoiceActive &&
		duel.mBattlezones[0].mCards.size() == 2 &&
		duel.mDecks[0].mCards.size() == 2;

	int placed = duel.mDecks[0].mCards.back()->mUniqueId;
	{
		ActiveDuelGuard activeGuard(duel);
		Message putIntoBattle("cardmove");
		putIntoBattle.addValue("card", placed);
		putIntoBattle.addValue("from", ZONE_DECK);
		putIntoBattle.addValue("to", ZONE_BATTLE);
		duel.mMsgMngr.sendMessage(putIntoBattle);
		duel.dispatchAllMessages();
	}
	return valid && !duel.mSimulationChoiceFailed && !duel.mIsChoiceActive &&
		duel.mBattlezones[0].mCards.size() == 3 &&
		duel.mDecks[0].mCards.size() == 1;
}

bool Application::exercisePoppleTapAbilitySmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int poppleId = getCardIdFromName("Popple, Flowerpetal Dancer");
	int deckCardId = getCardIdFromName("Bone Spider");
	if (poppleId < 0 || deckCardId < 0) return false;

	Duel duel;
	duel.mIsSimulation = true;
	duel.mInputLoopRunning = false;
	duel.mTurn = 0;
	duel.mTurnPhase = TURN_PHASE_MAIN;
	auto addCard = [&duel](int cardId, int zone) -> int
	{
		int uid = (int)duel.mCardList.size();
		Card* card = new Card(uid, cardId, 0);
		duel.mCardList.push_back(card);
		duel.getZone(0, zone)->addCard(card);
		card->mZone = zone;
		duel.mNextUniqueId = uid + 1;
		return uid;
	};

	int popple = addCard(poppleId, ZONE_BATTLE);
	duel.mCardList[popple]->mSummoningSickness = 0;
	addCard(deckCardId, ZONE_DECK);
	addCard(deckCardId, ZONE_DECK);
	{
		ActiveDuelGuard activeGuard(duel);
		Message tapAbility("creatureusetapability");
		tapAbility.addValue("creature", popple);
		duel.handleInterfaceInput(tapAbility);
		duel.dispatchAllMessages();
	}
	return duel.mCardList[popple]->mIsTapped &&
		duel.mDecks[0].mCards.size() == 1 &&
		duel.mManazones[0].mCards.size() == 1;
}

bool Application::exerciseDecisionPlanSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int hammerId = getCardIdFromName("Crimson Hammer");
	int lunarChargerId = getCardIdFromName("Lunar Charger");
	int fireCreatureId = getCardIdFromName("Deadly Fighter Braid Claw");
	int natureCreatureId = getCardIdFromName("Bronze-Arm Tribe");
	int slashChargerId = getCardIdFromName("Slash Charger");
	int futureSlashId = getCardIdFromName("Future Slash");
	int brutalChargeId = getCardIdFromName("Brutal Charge");
	int pokolulId = getCardIdFromName("Pokolul");
	int aquaSurferId = getCardIdFromName("Aqua Surfer");
	int spasticMissileId = getCardIdFromName("Spastic Missile");
	int emeralId = getCardIdFromName("Emeral");
	int solarRayId = getCardIdFromName("Solar Ray");
	int bolshackDragonId = getCardIdFromName("Bolshack Dragon");
	int dimensionGateId = getCardIdFromName("Dimension Gate");
	int manaNexusId = getCardIdFromName("Mana Nexus");
	int rondobilId = getCardIdFromName("Rondobil, the Explorer");
	int tornadoFlameId = getCardIdFromName("Tornado Flame");
	int terrorPitId = getCardIdFromName("Terror Pit");
	int deathSmokeId = getCardIdFromName("Death Smoke");
	int ghostTouchId = getCardIdFromName("Ghost Touch");
	int holyAweId = getCardIdFromName("Holy Awe");
	int crisisBoulderId = getCardIdFromName("Crisis Boulder");
	if (hammerId < 0 || lunarChargerId < 0 || fireCreatureId < 0 || natureCreatureId < 0 ||
		slashChargerId < 0 || futureSlashId < 0 || brutalChargeId < 0 || pokolulId < 0 ||
		aquaSurferId < 0 || spasticMissileId < 0 || emeralId < 0 || solarRayId < 0 ||
		bolshackDragonId < 0 || dimensionGateId < 0 || manaNexusId < 0 || rondobilId < 0 ||
		tornadoFlameId < 0 || terrorPitId < 0 || deathSmokeId < 0 || ghostTouchId < 0 ||
		holyAweId < 0 || crisisBoulderId < 0)
		return false;

	bool valid = true;
	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int hammer = addCard(hammerId, 0, ZONE_HAND);
		int fireMana1 = addCard(fireCreatureId, 0, ZONE_MANA);
		int fireMana2 = addCard(fireCreatureId, 0, ZONE_MANA);
		int natureMana = addCard(natureCreatureId, 0, ZONE_MANA);
		int target1 = addCard(fireCreatureId, 1, ZONE_BATTLE);
		int target2 = addCard(natureCreatureId, 1, ZONE_BATTLE);

		std::vector<DecisionPlan> allPlans = enumerateDecisionPlans(root);
		std::vector<DecisionPlan> hammerPlans;
		for (std::vector<DecisionPlan>::const_iterator plan = allPlans.begin();
			plan != allPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator type = plan->action.map.find("msgtype");
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (type != plan->action.map.end() && type->second == "cardplay" &&
				card != plan->action.map.end() && std::atoi(card->second.c_str()) == hammer)
				hammerPlans.push_back(*plan);
		}

		std::set<std::string> combinations;
		for (std::vector<DecisionPlan>::const_iterator plan = hammerPlans.begin();
			plan != hammerPlans.end(); ++plan)
		{
			valid = valid && plan->player == 0 && plan->manaCards.size() == 2 &&
				plan->manaCards[0] < plan->manaCards[1] && plan->choices.size() == 1 &&
				plan->choices[0].player == 0 &&
				(plan->choices[0].selection == target1 || plan->choices[0].selection == target2) &&
				(std::find(plan->manaCards.begin(), plan->manaCards.end(), fireMana1) !=
					plan->manaCards.end() ||
				 std::find(plan->manaCards.begin(), plan->manaCards.end(), fireMana2) !=
					plan->manaCards.end());
			if (plan->manaCards.size() == 2 && plan->choices.size() == 1)
			{
				combinations.insert(std::to_string(plan->manaCards[0]) + "," +
					std::to_string(plan->manaCards[1]) + ":" +
					std::to_string(plan->choices[0].selection));
			}

			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete;
			if (result.mCardList.size() == root.mCardList.size())
			{
				int selectedTarget = plan->choices[0].selection;
				int otherTarget = selectedTarget == target1 ? target2 : target1;
				valid = valid && result.mCardList[hammer]->mZone == ZONE_GRAVEYARD &&
					result.mCardList[selectedTarget]->mZone == ZONE_GRAVEYARD &&
					result.mCardList[otherTarget]->mZone == ZONE_BATTLE;
				for (int mana = fireMana1; mana <= natureMana; ++mana)
				{
					bool paid = std::find(plan->manaCards.begin(), plan->manaCards.end(), mana) !=
						plan->manaCards.end();
					valid = valid && result.mCardList[mana]->mIsTapped == paid;
				}
			}
			else
			{
				valid = false;
			}
		}
		valid = valid && hammerPlans.size() == 6 && combinations.size() == 6 &&
			root.mCardList[hammer]->mZone == ZONE_HAND &&
			!root.mCardList[fireMana1]->mIsTapped && !root.mCardList[fireMana2]->mIsTapped &&
			!root.mCardList[natureMana]->mIsTapped &&
			root.mCardList[target1]->mZone == ZONE_BATTLE &&
			root.mCardList[target2]->mZone == ZONE_BATTLE;

		DecisionPlanEnumerationOptions heuristicOptions;
		heuristicOptions.heuristicMana = true;
		std::vector<DecisionPlan> heuristicPlans = enumerateDecisionPlans(root, heuristicOptions);
		int heuristicHammerPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = heuristicPlans.begin();
			plan != heuristicPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator type =
				plan->action.map.find("msgtype");
			std::map<std::string, std::string>::const_iterator card =
				plan->action.map.find("card");
			if (type == plan->action.map.end() || type->second != "cardplay" ||
				card == plan->action.map.end() || std::atoi(card->second.c_str()) != hammer)
				continue;
			heuristicHammerPlans++;
			valid = valid && plan->manaCards.size() == 2 &&
				plan->manaCards[0] == fireMana1 && plan->manaCards[1] == fireMana2 &&
				plan->choices.size() == 1;
			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete;
		}
		valid = valid && heuristicHammerPlans == 2;

		DecisionPlanEnumerationOptions randomChoiceOptions;
		randomChoiceOptions.heuristicMana = true;
		randomChoiceOptions.randomChoices = true;
		randomChoiceOptions.randomIndex = [](size_t count) -> size_t
		{
			return count - 1;
		};
		std::vector<DecisionPlan> randomChoicePlans =
			enumerateDecisionPlans(root, randomChoiceOptions);
		int randomHammerPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = randomChoicePlans.begin();
			plan != randomChoicePlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator type =
				plan->action.map.find("msgtype");
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (type == plan->action.map.end() || type->second != "cardplay" ||
				card == plan->action.map.end() ||
				std::atoi(card->second.c_str()) != hammer)
				continue;
			randomHammerPlans++;
			valid = valid && plan->manaCards.size() == 2 && plan->choices.size() == 1 &&
				plan->choices[0].selection == target2;
		}
		valid = valid && randomHammerPlans == 1;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			root.mCardList.push_back(card);
			root.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int charger = addCard(lunarChargerId, ZONE_HAND);
		addCard(lunarChargerId, ZONE_MANA);
		addCard(lunarChargerId, ZONE_MANA);
		addCard(lunarChargerId, ZONE_MANA);
		int creature1 = addCard(fireCreatureId, ZONE_BATTLE);
		int creature2 = addCard(natureCreatureId, ZONE_BATTLE);

		std::vector<DecisionPlan> allPlans = enumerateDecisionPlans(root);
		std::vector<DecisionPlan> chargerPlans;
		for (std::vector<DecisionPlan>::const_iterator plan = allPlans.begin();
			plan != allPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator type = plan->action.map.find("msgtype");
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (type != plan->action.map.end() && type->second == "cardplay" &&
				card != plan->action.map.end() && std::atoi(card->second.c_str()) == charger)
				chargerPlans.push_back(*plan);
		}

		std::set<std::string> sequences;
		for (std::vector<DecisionPlan>::const_iterator plan = chargerPlans.begin();
			plan != chargerPlans.end(); ++plan)
		{
			valid = valid && plan->manaCards.size() == 3;
			std::string sequence;
			for (std::vector<DecisionChoice>::const_iterator choice = plan->choices.begin();
				choice != plan->choices.end(); ++choice)
			{
				valid = valid && choice->player == 0;
				if (!sequence.empty()) sequence += ",";
				sequence += std::to_string(choice->selection);
			}
			sequences.insert(sequence);

			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete;
		}
		std::set<std::string> expected;
		expected.insert(std::to_string(RETURN_BUTTON1));
		expected.insert(std::to_string(creature1) + "," + std::to_string(RETURN_BUTTON1));
		expected.insert(std::to_string(creature1) + "," + std::to_string(creature2));
		expected.insert(std::to_string(creature2) + "," + std::to_string(RETURN_BUTTON1));
		expected.insert(std::to_string(creature2) + "," + std::to_string(creature1));
		valid = valid && chargerPlans.size() == expected.size() && sequences == expected &&
			root.mCardList[charger]->mZone == ZONE_HAND;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int charger = addCard(slashChargerId, 0, ZONE_HAND);
		for (int mana = 0; mana < 3; ++mana)
			addCard(slashChargerId, 0, ZONE_MANA);
		int ownDeckCard = addCard(natureCreatureId, 0, ZONE_DECK);
		int opponentLowCostCard = addCard(fireCreatureId, 1, ZONE_DECK);
		int opponentHighCostCard = addCard(natureCreatureId, 1, ZONE_DECK);

		DecisionPlanEnumerationOptions allChoiceOptions;
		allChoiceOptions.heuristicMana = true;
		std::vector<DecisionPlan> allPlans = enumerateDecisionPlans(root, allChoiceOptions);
		int ownDeckPlans = 0;
		int opponentDeckPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = allPlans.begin();
			plan != allPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card == plan->action.map.end() || std::atoi(card->second.c_str()) != charger ||
				plan->choices.size() != 2)
				continue;
			if (plan->choices[0].selection == RETURN_BUTTON2 &&
				plan->choices[1].selection == ownDeckCard)
				ownDeckPlans++;
			if (plan->choices[0].selection == RETURN_BUTTON1 &&
				(plan->choices[1].selection == opponentLowCostCard ||
				 plan->choices[1].selection == opponentHighCostCard))
				opponentDeckPlans++;
		}

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicMana = true;
		aiOptions.heuristicCardPlay = true;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> aiPlans = enumerateDecisionPlans(root, aiOptions);
		int aiChargerPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = aiPlans.begin();
			plan != aiPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card == plan->action.map.end() || std::atoi(card->second.c_str()) != charger)
				continue;
			aiChargerPlans++;
			valid = valid && plan->choices.size() == 2 &&
				plan->choices[0].selection == RETURN_BUTTON1 &&
				plan->choices[1].selection == opponentHighCostCard;
			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete &&
				result.mCardList[ownDeckCard]->mZone == ZONE_DECK &&
				result.mCardList[opponentLowCostCard]->mZone == ZONE_DECK &&
				result.mCardList[opponentHighCostCard]->mZone == ZONE_GRAVEYARD;
		}
		bool slashChargerCase = ownDeckPlans == 1 && opponentDeckPlans == 2 &&
			aiChargerPlans == 1;
		if (!slashChargerCase)
		{
			std::cerr << "Slash Charger plan case: own=" << ownDeckPlans <<
				", opponent=" << opponentDeckPlans << ", ai=" << aiChargerPlans << std::endl;
		}
		valid = valid && slashChargerCase;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			root.mCardList.push_back(card);
			root.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int emeral = addCard(emeralId, ZONE_HAND);
		addCard(emeralId, ZONE_MANA);
		addCard(emeralId, ZONE_MANA);
		int nonTrigger = addCard(natureCreatureId, ZONE_HAND);
		int lowCostTrigger = addCard(solarRayId, ZONE_HAND);
		int highCostTrigger = addCard(aquaSurferId, ZONE_HAND);
		int firstShield = addCard(fireCreatureId, ZONE_SHIELD);
		addCard(natureCreatureId, ZONE_SHIELD);

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicMana = true;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(root, aiOptions);
		int emeralPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = plans.begin();
			plan != plans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card == plan->action.map.end() || std::atoi(card->second.c_str()) != emeral)
				continue;
			emeralPlans++;
			valid = valid && plan->choices.size() == 2 &&
				plan->choices[0].selection == highCostTrigger &&
				plan->choices[1].selection == firstShield;

			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete &&
				result.mCardList[highCostTrigger]->mZone == ZONE_SHIELD &&
				result.mCardList[firstShield]->mZone == ZONE_HAND &&
				result.mCardList[lowCostTrigger]->mZone == ZONE_HAND &&
				result.mCardList[nonTrigger]->mZone == ZONE_HAND;
		}
		valid = valid && emeralPlans == 1;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			root.mCardList.push_back(card);
			root.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int manaNexus = addCard(manaNexusId, ZONE_HAND);
		int highestHandValue = addCard(natureCreatureId, ZONE_MANA);
		int expensiveCard = addCard(bolshackDragonId, ZONE_MANA);
		int cheapCard = addCard(fireCreatureId, ZONE_MANA);
		int waterCard = addCard(emeralId, ZONE_MANA);

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicMana = true;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(root, aiOptions);
		int manaNexusPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = plans.begin();
			plan != plans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card == plan->action.map.end() || std::atoi(card->second.c_str()) != manaNexus)
				continue;
			manaNexusPlans++;
			valid = valid && plan->choices.size() == 1 &&
				plan->choices[0].selection == highestHandValue;
			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete &&
				result.mCardList[highestHandValue]->mZone == ZONE_SHIELD &&
				result.mCardList[expensiveCard]->mZone == ZONE_MANA &&
				result.mCardList[cheapCard]->mZone == ZONE_MANA &&
				result.mCardList[waterCard]->mZone == ZONE_MANA;
		}
		valid = valid && manaNexusPlans == 1;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			root.mCardList.push_back(card);
			root.getZone(0, ZONE_BATTLE)->addCard(card);
			card->mZone = ZONE_BATTLE;
			card->mSummoningSickness = 0;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int rondobil = addCard(rondobilId);
		int lowestBattleValue = addCard(fireCreatureId);
		int highBattleValue = addCard(bolshackDragonId);

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(root, aiOptions);
		int rondobilPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = plans.begin();
			plan != plans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator type =
				plan->action.map.find("msgtype");
			std::map<std::string, std::string>::const_iterator creature =
				plan->action.map.find("creature");
			if (type == plan->action.map.end() || type->second != "creatureusetapability" ||
				creature == plan->action.map.end() ||
				std::atoi(creature->second.c_str()) != rondobil)
				continue;
			rondobilPlans++;
			valid = valid && plan->choices.size() == 1 &&
				plan->choices[0].selection == lowestBattleValue;
			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete &&
				result.mCardList[rondobil]->mIsTapped &&
				result.mCardList[lowestBattleValue]->mZone == ZONE_SHIELD &&
				result.mCardList[highBattleValue]->mZone == ZONE_BATTLE;
		}
		valid = valid && rondobilPlans == 1;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			root.mCardList.push_back(card);
			root.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int emeral = addCard(emeralId, ZONE_HAND);
		addCard(emeralId, ZONE_MANA);
		addCard(emeralId, ZONE_MANA);
		int affordableCard = addCard(fireCreatureId, ZONE_HAND);
		int lowestHandValue = addCard(bolshackDragonId, ZONE_HAND);
		int firstShield = addCard(natureCreatureId, ZONE_SHIELD);

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicMana = true;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(root, aiOptions);
		int emeralPlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = plans.begin();
			plan != plans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card == plan->action.map.end() || std::atoi(card->second.c_str()) != emeral)
				continue;
			emeralPlans++;
			valid = valid && plan->choices.size() == 2 &&
				plan->choices[0].selection == lowestHandValue &&
				plan->choices[1].selection == firstShield;
			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete &&
				result.mCardList[lowestHandValue]->mZone == ZONE_SHIELD &&
				result.mCardList[firstShield]->mZone == ZONE_HAND &&
				result.mCardList[affordableCard]->mZone == ZONE_HAND;
		}
		valid = valid && emeralPlans == 1;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			root.mCardList.push_back(card);
			root.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int dimensionGate = addCard(dimensionGateId, ZONE_HAND);
		for (int mana = 0; mana < 3; ++mana)
			addCard(natureCreatureId, ZONE_MANA);
		int lowCostCreature = addCard(fireCreatureId, ZONE_DECK);
		int highCostCreature = addCard(bolshackDragonId, ZONE_DECK);
		int invalidSpell = addCard(solarRayId, ZONE_DECK);

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicMana = true;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(root, aiOptions);
		int dimensionGatePlans = 0;
		for (std::vector<DecisionPlan>::const_iterator plan = plans.begin();
			plan != plans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card == plan->action.map.end() ||
				std::atoi(card->second.c_str()) != dimensionGate)
				continue;
			dimensionGatePlans++;
			valid = valid && plan->choices.size() == 1 &&
				plan->choices[0].selection == highCostCreature;
			Duel result;
			result.mIsSimulation = true;
			result.mInputLoopRunning = false;
			valid = valid && result.copyFrom(root) &&
				executeDecisionPlan(result, *plan).status == DecisionPlanStatus::Complete &&
				result.mCardList[highCostCreature]->mZone == ZONE_HAND &&
				result.mCardList[lowCostCreature]->mZone == ZONE_DECK &&
				result.mCardList[invalidSpell]->mZone == ZONE_DECK;
		}
		valid = valid && dimensionGatePlans == 1;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int futureSlash = addCard(futureSlashId, 0, ZONE_HAND);
		addCard(fireCreatureId, 1, ZONE_DECK);
		int highestCost = addCard(natureCreatureId, 1, ZONE_DECK);
		ActiveDuelGuard activeGuard(root);
		valid = valid && root.getCardAiPreferredChoice(futureSlash) == highestCost;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int attacker = addCard(fireCreatureId, 0, ZONE_BATTLE);
		int brutalCharge = addCard(brutalChargeId, 0, ZONE_HAND);
		int firstCreature = addCard(fireCreatureId, 0, ZONE_DECK);
		int secondCreature = addCard(natureCreatureId, 0, ZONE_DECK);
		int firstShield = addCard(fireCreatureId, 1, ZONE_SHIELD);
		int secondShield = addCard(natureCreatureId, 1, ZONE_SHIELD);
		std::vector<int> shields;
		shields.push_back(firstShield);
		shields.push_back(secondShield);
		ActiveDuelGuard activeGuard(root);
		for (std::vector<int>::const_iterator shield = shields.begin(); shield != shields.end(); ++shield)
		{
			Message broken("creaturebreakshield");
			broken.addValue("creature", attacker);
			broken.addValue("attacker", attacker);
			broken.addValue("defender", 1);
			broken.addValue("shield", *shield);
			root.mMsgMngr.sendMessage(broken);
			root.dispatchAllMessages();
		}
		bool countedBeforeCast = root.getShieldsBrokenThisTurn(0) == 2;

		Message cast("cardmove");
		cast.addValue("card", brutalCharge);
		cast.addValue("from", ZONE_HAND);
		cast.addValue("to", ZONE_BATTLE);
		root.mMsgMngr.sendMessage(cast);
		root.dispatchAllMessages();
		bool modifierCreated = root.mCardList[brutalCharge]->mZone == ZONE_GRAVEYARD &&
			root.mCardList[brutalCharge]->mModifiers.size() == 1;

		std::vector<int> selectedCreatures;
		root.setChoiceResolver(
			[&](const Duel& position) -> int
			{
				if (position.mChoiceValidCards.empty()) return RETURN_QUIT;
				selectedCreatures.push_back(position.mChoiceValidCards.front());
				return position.mChoiceValidCards.front();
			});
		Message endTurn("endturn");
		endTurn.addValue("player", 0);
		root.mMsgMngr.sendMessage(endTurn);
		root.dispatchAllMessages();
		bool brutalChargeCase = countedBeforeCast && modifierCreated &&
			selectedCreatures.size() == 2 &&
			root.mCardList[firstCreature]->mZone == ZONE_HAND &&
			root.mCardList[secondCreature]->mZone == ZONE_HAND &&
			root.getShieldsBrokenThisTurn(0) == 0 &&
			root.mCardList[brutalCharge]->mModifiers.empty();
		if (!brutalChargeCase)
		{
			std::cerr << "Brutal Charge case: pre-cast-count=" << countedBeforeCast <<
				", modifier=" << modifierCreated << ", choices=" << selectedCreatures.size() <<
				", first-zone=" << root.mCardList[firstCreature]->mZone << ", second-zone=" <<
				root.mCardList[secondCreature]->mZone << std::endl;
		}
		valid = valid && brutalChargeCase;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int pokolul = addCard(pokolulId, 0, ZONE_BATTLE);
		int otherAttacker = addCard(fireCreatureId, 0, ZONE_BATTLE);
		int trigger = addCard(aquaSurferId, 1, ZONE_HAND);
		root.mCardList[pokolul]->mIsTapped = true;
		root.mAttacker = pokolul;
		int choices = 0;
		root.setChoiceResolver(
			[&](const Duel& position) -> int
			{
				++choices;
				return position.mChoice != NULL && position.mChoice->mButtonCount == 2 ?
					RETURN_BUTTON1 : RETURN_QUIT;
			});
		ActiveDuelGuard activeGuard(root);
		Message used("shieldtriggerused");
		used.addValue("trigger", trigger);
		root.mMsgMngr.sendMessage(used);
		root.dispatchAllMessages();
		bool currentAttackerUntapped = choices == 1 && !root.mCardList[pokolul]->mIsTapped;

		root.mCardList[pokolul]->mIsTapped = true;
		root.mAttacker = otherAttacker;
		choices = 0;
		root.mMsgMngr.sendMessage(used);
		root.dispatchAllMessages();
		bool otherAttackerIgnored = choices == 0 && root.mCardList[pokolul]->mIsTapped;
		if (!currentAttackerUntapped || !otherAttackerIgnored)
		{
			std::cerr << "Pokolul case: current=" << currentAttackerUntapped <<
				", other=" << otherAttackerIgnored << std::endl;
		}
		valid = valid && currentAttackerUntapped && otherAttackerIgnored;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int tornadoFlame = addCard(tornadoFlameId, 0, ZONE_HAND);
		int terrorPit = addCard(terrorPitId, 0, ZONE_HAND);
		int deathSmoke = addCard(deathSmokeId, 0, ZONE_HAND);
		int ghostTouch = addCard(ghostTouchId, 0, ZONE_HAND);
		int holyAwe = addCard(holyAweId, 0, ZONE_HAND);
		int crisisBoulder = addCard(crisisBoulderId, 0, ZONE_HAND);
		bool emptyBoardRejected = false;
		bool emptyHandRejected = false;
		{
			ActiveDuelGuard activeGuard(root);
				emptyBoardRejected = root.getCardAiCanCast(tornadoFlame) == 0 &&
				root.getCardAiCanCast(terrorPit) == 0 &&
				root.getCardAiCanCast(deathSmoke) == 0 &&
				root.getCardAiCanCast(holyAwe) == 0 &&
				root.getCardAiCanCast(crisisBoulder) == 0;
			emptyHandRejected = root.getCardAiCanCast(ghostTouch) == 0;
		}

		addCard(fireCreatureId, 1, ZONE_MANA);
		bool manaTargetAccepted = false;
		{
			ActiveDuelGuard activeGuard(root);
			manaTargetAccepted = root.getCardAiCanCast(crisisBoulder) == 1;
		}

		int strongCreature = addCard(bolshackDragonId, 1, ZONE_BATTLE);
		bool strongTargetRestrictions = false;
		{
			ActiveDuelGuard activeGuard(root);
			strongTargetRestrictions = root.getCardAiCanCast(tornadoFlame) == 0 &&
				root.getCardAiCanCast(terrorPit) == 1 &&
				root.getCardAiCanCast(deathSmoke) == 1 &&
				root.getCardAiCanCast(holyAwe) == 1 &&
				root.getCardAiCanCast(crisisBoulder) == 1;
		}
		root.mCardList[strongCreature]->mIsTapped = true;
		bool tappedTargetRejected = false;
		{
			ActiveDuelGuard activeGuard(root);
			tappedTargetRejected = root.getCardAiCanCast(deathSmoke) == 0 &&
				root.getCardAiCanCast(holyAwe) == 0;
		}

		addCard(fireCreatureId, 1, ZONE_BATTLE);
		addCard(fireCreatureId, 1, ZONE_HAND);
		bool restrictedTargetsAccepted = false;
		bool occupiedHandAccepted = false;
		{
			ActiveDuelGuard activeGuard(root);
			restrictedTargetsAccepted = root.getCardAiCanCast(tornadoFlame) == 1 &&
				root.getCardAiCanCast(deathSmoke) == 1 &&
				root.getCardAiCanCast(holyAwe) == 1;
			occupiedHandAccepted = root.getCardAiCanCast(ghostTouch) == 1;
		}
		valid = valid && emptyBoardRejected && emptyHandRejected && strongTargetRestrictions &&
			manaTargetAccepted && tappedTargetRejected && restrictedTargetsAccepted &&
			occupiedHandAccepted;
	}

	{
		Duel root;
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int missile = addCard(spasticMissileId, 0, ZONE_HAND);
		for (int mana = 0; mana < 3; ++mana)
			addCard(fireCreatureId, 0, ZONE_MANA);
		bool legallyCastable = false;
		bool aiCastableWithoutTarget = true;
		Message fallback;
		{
			ActiveDuelGuard activeGuard(root);
			legallyCastable = root.getCardCanCast(missile) == 1;
			aiCastableWithoutTarget = root.getCardAiCanCast(missile) == 1;
			HeuristicBot bot(0);
			fallback = bot.chooseMove(root, root.getPossibleMoves());
		}

		DecisionPlanEnumerationOptions aiOptions;
		aiOptions.heuristicMana = true;
		aiOptions.heuristicCardPlay = true;
		aiOptions.heuristicChoices = true;
		std::vector<DecisionPlan> emptyBoardPlans = enumerateDecisionPlans(root, aiOptions);
		bool plannedWithoutTarget = false;
		for (std::vector<DecisionPlan>::const_iterator plan = emptyBoardPlans.begin();
			plan != emptyBoardPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card != plan->action.map.end() && std::atoi(card->second.c_str()) == missile)
				plannedWithoutTarget = true;
		}

		int target = addCard(fireCreatureId, 1, ZONE_BATTLE);
		bool aiCastableWithTarget = false;
		{
			ActiveDuelGuard activeGuard(root);
			aiCastableWithTarget = root.getCardAiCanCast(missile) == 1;
		}
		std::vector<DecisionPlan> targetedPlans = enumerateDecisionPlans(root, aiOptions);
		bool plannedWithTarget = false;
		for (std::vector<DecisionPlan>::const_iterator plan = targetedPlans.begin();
			plan != targetedPlans.end(); ++plan)
		{
			std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
			if (card != plan->action.map.end() && std::atoi(card->second.c_str()) == missile &&
				plan->choices.size() == 1 && plan->choices[0].selection == target)
				plannedWithTarget = true;
		}
		bool spasticMissileCase = legallyCastable && !aiCastableWithoutTarget &&
			fallback.getType() != "cardplay" && !plannedWithoutTarget &&
			aiCastableWithTarget && plannedWithTarget;
		if (!spasticMissileCase)
		{
			std::cerr << "Spastic Missile AI case: legal=" << legallyCastable <<
				", ai-empty=" << aiCastableWithoutTarget << ", fallback=" <<
				fallback.getType() << ", planned-empty=" << plannedWithoutTarget <<
				", ai-target=" << aiCastableWithTarget << ", planned-target=" <<
				plannedWithTarget << std::endl;
		}
		valid = valid && spasticMissileCase;
	}
	return valid;
}

bool Application::exerciseMotorcycleMutantSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int mutantCardId = getCardIdFromName("Motorcycle Mutant");
	int creatureCardId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (mutantCardId < 0 || creatureCardId < 0) return false;

	Duel duel;
	duel.mIsSimulation = true;
	duel.mInputLoopRunning = false;
	auto addCard = [&duel](int cardId, int zone) -> int
	{
		int uid = (int)duel.mCardList.size();
		Card* card = new Card(uid, cardId, 0);
		duel.mCardList.push_back(card);
		duel.getZone(0, zone)->addCard(card);
		card->mZone = zone;
		duel.mNextUniqueId = uid + 1;
		return uid;
	};

	int mutant = addCard(mutantCardId, ZONE_HAND);
	int otherCreature = addCard(creatureCardId, ZONE_HAND);
	auto summon = [&duel](int card)
	{
		Message move("cardmove");
		move.addValue("card", card);
		move.addValue("from", ZONE_HAND);
		move.addValue("to", ZONE_BATTLE);
		move.addValue("evobait", -1);
		duel.mMsgMngr.sendMessage(move);
		duel.dispatchAllMessages();
	};

	ActiveDuelGuard activeGuard(duel);
	summon(mutant);
	bool survivedOwnSummon = duel.mCardList[mutant]->mZone == ZONE_BATTLE;
	summon(otherCreature);
	return survivedOwnSummon && duel.mCardList[mutant]->mZone == ZONE_GRAVEYARD &&
		duel.mCardList[otherCreature]->mZone == ZONE_BATTLE && duel.mMsgMngr.messages.empty();
}

bool Application::exerciseAvalancheGiantSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int giantCardId = getCardIdFromName("Avalanche Giant");
	int creatureCardId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (giantCardId < 0 || creatureCardId < 0) return false;

	Duel duel;
	duel.mIsSimulation = true;
	duel.mInputLoopRunning = false;
	auto addCard = [&duel](int cardId, int owner, int zone) -> int
	{
		int uid = static_cast<int>(duel.mCardList.size());
		Card* card = new Card(uid, cardId, owner);
		duel.mCardList.push_back(card);
		duel.getZone(owner, zone)->addCard(card);
		card->mZone = zone;
		duel.mNextUniqueId = uid + 1;
		return uid;
	};

	int giant = addCard(giantCardId, 0, ZONE_BATTLE);
	int blocker = addCard(creatureCardId, 1, ZONE_BATTLE);
	int shield = addCard(creatureCardId, 1, ZONE_SHIELD);
	duel.setChoiceResolver([shield](const Duel&) { return shield; });

	Message battle("creaturebattle");
	battle.addValue("attacker", giant);
	battle.addValue("defender", blocker);
	battle.addValue("blocked", 1);
	duel.mMsgMngr.sendMessage(battle);
	{
		ActiveDuelGuard activeGuard(duel);
		duel.dispatchAllMessages();
	}

	return duel.mCardList[shield]->mZone == ZONE_HAND &&
		duel.mCardList[giant]->mZone == ZONE_BATTLE &&
		duel.mCardList[blocker]->mZone == ZONE_GRAVEYARD &&
		!duel.hasSimulationChoiceFailure() && duel.mMsgMngr.messages.empty();
}

bool Application::exerciseMctsSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int attackerId = getCardIdFromName("Burning Mane");
	int blockerId = getCardIdFromName("Spiral Grass");
	int bombazarId = getCardIdFromName("Bombazar, Dragon of Destiny");
	if (attackerId < 0 || blockerId < 0 || bombazarId < 0) return false;

	auto prepareDuel = [attackerId, blockerId](Duel& duel, bool addBlocker) -> int
	{
		duel.mInputLoopRunning = false;
		duel.mTurn = 0;
		duel.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&duel](int cardId, int owner, int zone) -> int
		{
			int uid = (int)duel.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			duel.mCardList.push_back(card);
			duel.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			duel.mNextUniqueId = uid + 1;
			return uid;
		};
		int attacker = addCard(attackerId, 0, ZONE_BATTLE);
		duel.mCardList[attacker]->mSummoningSickness = 0;
		if (addBlocker)
		{
			addCard(blockerId, 1, ZONE_BATTLE);
			addCard(attackerId, 0, ZONE_SHIELD);
			addCard(attackerId, 0, ZONE_SHIELD);
		}
		for (int copy = 0; copy < 4; ++copy)
		{
			addCard(attackerId, 0, ZONE_DECK);
			addCard(attackerId, 1, ZONE_DECK);
		}
		return attacker;
	};

	MctsConfig config;
	config.iterations = 128;
	config.maxDepth = 6;
	config.seed = 982451653U;
	bool valid = true;
	{
		Duel root;
		int attacker = prepareDuel(root, false);
		MctsSearch search(0, config);
		MctsResult result = search.search(root);
		MctsResult repeated = search.search(root);
		MctsSession incremental(0, config);
		bool incrementalStarted = incremental.start(root);
		bool completedOnFirstBatch = incremental.advance(1);
		MctsResult partial = incremental.result();
		while (!incremental.isComplete()) incremental.advance(1);
		MctsResult incrementalResult = incremental.result();
		bool caseValid = result.hasPlan && result.failedIterations == 0 &&
			result.iterationsCompleted == config.iterations &&
			result.plan.action.getType() == "creatureattack" &&
			result.selectedVisits > 0 && std::isfinite(result.selectedMeanValue) &&
			repeated.hasPlan && repeated.plan == result.plan &&
			repeated.iterationsCompleted == result.iterationsCompleted &&
			incrementalStarted && !completedOnFirstBatch &&
			partial.iterationsCompleted == 1 && incrementalResult.hasPlan &&
			incrementalResult.plan == result.plan &&
			incrementalResult.iterationsCompleted == config.iterations &&
			incrementalResult.failedIterations == result.failedIterations &&
			root.mWinner == -1 && root.mCardList[attacker]->mZone == ZONE_BATTLE &&
			!root.mCardList[attacker]->mIsTapped;

		int attackVisits = -1;
		int endTurnVisits = -1;
		for (std::vector<MctsChildStatistics>::iterator child = result.rootChildren.begin();
			child != result.rootChildren.end(); ++child)
		{
			std::string type = child->plan.action.getType();
			if (type == "creatureattack") attackVisits = child->visits;
			else if (type == "endturn") endTurnVisits = child->visits;
		}
		caseValid = caseValid && attackVisits > endTurnVisits && endTurnVisits > 0;
		if (!caseValid)
		{
			std::cerr << "MCTS lethal case: plan=" <<
				(result.hasPlan ? result.plan.action.getType() : "none") <<
				", completed=" << result.iterationsCompleted <<
				", failed=" << result.failedIterations << ", attack visits=" << attackVisits <<
				", end visits=" << endTurnVisits << std::endl;
		}
		valid = valid && caseValid;
	}

	{
		Duel root;
		prepareDuel(root, false);
		root.mIsSimulation = true;
		for (int shield = 0; shield < 2; ++shield)
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, attackerId, 1);
			root.mCardList.push_back(card);
			root.mShields[1].addCard(card);
			card->mZone = ZONE_SHIELD;
			root.mNextUniqueId = uid + 1;
		}
		MctsSession reusable(0, config);
		bool reuseCase = reusable.start(root);
		reusable.advance(config.iterations);
		MctsResult first = reusable.result();
		reuseCase = reuseCase && first.hasPlan &&
			first.plan.action.getType() == "creatureattack" &&
			executeDecisionPlan(root, first.plan).status == DecisionPlanStatus::Complete;
		std::vector<DecisionPlan> blockPlans = enumerateDecisionPlans(root);
		reuseCase = reuseCase && blockPlans.size() == 1 &&
			blockPlans[0].action.getType() == "blockskip";
		if (blockPlans.size() == 1)
			reuseCase = reuseCase && executeDecisionPlan(root, blockPlans[0]).status ==
				DecisionPlanStatus::Complete;
		reuseCase = reuseCase && root.mAttackphase == PHASE_TARGET &&
			reusable.restart(root, config);
		MctsResult reused = reusable.result();
		reuseCase = reuseCase && reused.reusedTree && reused.reusedRootVisits > 0;
		if (!reuseCase)
		{
			std::cerr << "MCTS tree-reuse case: first=" <<
				(first.hasPlan ? first.plan.action.getType() : "none") <<
				", block-plans=" << blockPlans.size() << ", phase=" <<
				root.mAttackphase << ", reused=" << reused.reusedTree <<
				", retained-visits=" << reused.reusedRootVisits << std::endl;
		}
		valid = valid && reuseCase;
	}

	{
		Duel root;
		int attacker = prepareDuel(root, true);
		// Keep the safety case meaningful under the power/1000 + breaker*2
		// evaluator: attacking must sacrifice a materially valuable creature.
		root.mCardList[attacker]->mPower = 7000;
		root.mBattlezones[1].mCards.front()->mPower = 8000;
		MctsSearch search(0, config);
		MctsResult result = search.search(root);
		bool caseValid = result.hasPlan && result.failedIterations == 0 &&
			result.iterationsCompleted == config.iterations &&
			result.plan.action.getType() == "endturn" && root.mWinner == -1 &&
			root.mCardList[attacker]->mZone == ZONE_BATTLE &&
			!root.mCardList[attacker]->mIsTapped;
		if (!caseValid)
		{
			std::cerr << "MCTS blocker case: plan=" <<
				(result.hasPlan ? result.plan.action.getType() : "none") <<
				", completed=" << result.iterationsCompleted <<
				", failed=" << result.failedIterations << std::endl;
			for (std::vector<MctsChildStatistics>::iterator child = result.rootChildren.begin();
				child != result.rootChildren.end(); ++child)
			{
				std::cerr << "  " << child->plan.action.getType() << ": visits=" <<
					child->visits << ", mean=" << child->meanValue << std::endl;
			}
		}
		valid = valid && caseValid;
	}

	{
		int proclamationId = getCardIdFromName("Proclamation of Death");
		int highValueId = getCardIdFromName("Bolzard Dragon");
		if (proclamationId < 0 || highValueId < 0) return false;
		Duel root;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};
		int proclamation = addCard(proclamationId, 0, ZONE_HAND);
		for (int mana = 0; mana < 4; ++mana) addCard(proclamationId, 0, ZONE_MANA);
		int lowValue = addCard(attackerId, 1, ZONE_BATTLE);
		int highValue = addCard(highValueId, 1, ZONE_BATTLE);
		addCard(attackerId, 0, ZONE_DECK);
		addCard(attackerId, 1, ZONE_DECK);

		MctsConfig ownershipConfig;
		ownershipConfig.iterations = 256;
		ownershipConfig.maxDepth = 1;
		ownershipConfig.seed = 32452843U;
		MctsSearch search(0, ownershipConfig);
		MctsResult result = search.search(root);
		bool foundSpell = false;
		for (std::vector<MctsChildStatistics>::iterator child = result.rootChildren.begin();
			child != result.rootChildren.end(); ++child)
		{
			std::map<std::string, std::string>::const_iterator card =
				child->plan.action.map.find("card");
			if (child->plan.action.getType() != "cardplay" ||
				card == child->plan.action.map.end() || std::atoi(card->second.c_str()) != proclamation)
				continue;
			foundSpell = true;
			valid = valid && child->plan.choices.size() == 1 &&
				child->plan.choices[0].player == 1 &&
				child->plan.choices[0].selection == highValue;
		}
		valid = valid && result.failedIterations == 0 && foundSpell &&
			root.mCardList[lowValue]->mZone == ZONE_BATTLE &&
			root.mCardList[highValue]->mZone == ZONE_BATTLE;
	}

	{
		Duel root;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		auto addCard = [&root](int cardId, int owner, int zone) -> int
		{
			int uid = (int)root.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			root.mCardList.push_back(card);
			root.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			root.mNextUniqueId = uid + 1;
			return uid;
		};

		int bombazar = addCard(bombazarId, 0, ZONE_HAND);
		for (int mana = 0; mana < 8; ++mana)
			addCard(bombazarId, 0, ZONE_MANA);
		addCard(attackerId, 0, ZONE_DECK);
		addCard(attackerId, 1, ZONE_DECK);
		Message summon("cardmove");
		summon.addValue("card", bombazar);
		summon.addValue("from", ZONE_HAND);
		summon.addValue("to", ZONE_BATTLE);
		summon.addValue("evobait", -1);
		root.mMsgMngr.sendMessage(summon);
		{
			ActiveDuelGuard activeGuard(root);
			root.dispatchAllMessages();
		}
		root.mCardList[bombazar]->mIsTapped = true;

		MctsConfig extraTurnConfig;
		extraTurnConfig.iterations = 64;
		extraTurnConfig.maxDepth = 2;
		extraTurnConfig.exploration = 0.0;
		extraTurnConfig.seed = 49979687U;
		MctsSearch search(0, extraTurnConfig);
		MctsResult result = search.search(root);
		bool extraTurnCase = result.hasPlan && result.failedIterations == 0 &&
			result.iterationsCompleted == extraTurnConfig.iterations &&
			result.plan.action.getType() == "endturn" && result.meanValue > 0.9 &&
			result.turnHorizonCutoffs == 0 && root.mWinner == -1 &&
			root.mCardList[bombazar]->mIsTapped;
		if (!extraTurnCase)
		{
			std::cerr << "Bombazar extra-turn MCTS case: plan=" <<
				(result.hasPlan ? result.plan.action.getType() : "none") <<
				", completed=" << result.iterationsCompleted << ", failed=" <<
				result.failedIterations << ", mean=" << result.meanValue <<
				", cutoffs=" << result.turnHorizonCutoffs << std::endl;
		}
		valid = valid && extraTurnCase;
	}

	{
		Duel root;
		root.mInputLoopRunning = false;
		root.mTurn = 0;
		root.mTurnPhase = TURN_PHASE_MAIN;
		for (int copy = 0; copy < 6; ++copy)
		{
			for (int player = 0; player < 2; ++player)
			{
				int uid = (int)root.mCardList.size();
				Card* card = new Card(uid, attackerId, player);
				root.mCardList.push_back(card);
				root.mDecks[player].addCard(card);
				card->mZone = ZONE_DECK;
				root.mNextUniqueId = uid + 1;
			}
		}
		MctsConfig horizonConfig;
		horizonConfig.iterations = 16;
		horizonConfig.maxDepth = 48;
		horizonConfig.seed = 15485863U;
		root.mIsSimulation = true;
		MctsSession search(0, horizonConfig);
		bool horizonStarted = search.start(root);
		search.advance(horizonConfig.iterations);
		MctsResult result = search.result();
		bool horizonCase = result.hasPlan && result.plan.action.getType() == "endturn" &&
			horizonStarted &&
			result.iterationsCompleted == horizonConfig.iterations &&
			result.failedIterations == 0 &&
			result.turnHorizonCutoffs == horizonConfig.iterations &&
			result.forcedMovesApplied > horizonConfig.iterations;
		if (result.hasPlan)
			horizonCase = horizonCase && executeDecisionPlan(root, result.plan).status ==
				DecisionPlanStatus::Complete;
		bool opponentTurnSeen = false;
		DecisionPlanEnumerationOptions horizonOptions;
		horizonOptions.heuristicMana = true;
		horizonOptions.heuristicCardPlay = true;
		horizonOptions.heuristicChoices = true;
		horizonOptions.randomShieldTarget = true;
		for (int step = 0; horizonCase && step < 20; ++step)
		{
			if (root.mTurn != 0) opponentTurnSeen = true;
			if (opponentTurnSeen && root.mTurn == 0) break;
			std::vector<DecisionPlan> plans = enumerateDecisionPlans(root, horizonOptions);
			horizonCase = plans.size() == 1 &&
				executeDecisionPlan(root, plans.front()).status == DecisionPlanStatus::Complete;
		}
		horizonCase = horizonCase && opponentTurnSeen && root.mTurn == 0 &&
			search.restart(root, horizonConfig);
		MctsResult reusedHorizon = search.result();
		horizonCase = horizonCase && reusedHorizon.reusedTree &&
			reusedHorizon.reusedRootVisits == horizonConfig.iterations;
		if (!horizonCase)
		{
			std::cerr << "Turn-horizon MCTS case: plan=" <<
				(result.hasPlan ? result.plan.action.getType() : "none") <<
				", completed=" << result.iterationsCompleted << ", failed=" <<
				result.failedIterations << ", cutoffs=" << result.turnHorizonCutoffs <<
				", forced=" << result.forcedMovesApplied << ", reused=" <<
				reusedHorizon.reusedTree << ", retained-visits=" <<
				reusedHorizon.reusedRootVisits <<
				std::endl;
		}
		valid = valid && horizonCase;
	}

	{
		Duel root;
		prepareDuel(root, false);
		MctsConfig timedConfig;
		timedConfig.iterations = 100000;
		timedConfig.maxDepth = 6;
		timedConfig.timeBudgetMs = 100;
		timedConfig.seed = 86028121U;
		std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
		MctsSearch search(0, timedConfig);
		MctsResult result = search.search(root);
		long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - began).count();
		bool timedCase = result.hasPlan && result.timeBudgetExpired &&
			result.iterationsCompleted > 0 &&
			result.iterationsCompleted + result.failedIterations < timedConfig.iterations &&
			elapsed < 2000;
		if (!timedCase)
		{
			std::cerr << "Timed MCTS case: has-plan=" << result.hasPlan <<
				", timed-out=" << result.timeBudgetExpired << ", completed=" <<
				result.iterationsCompleted << ", failed=" << result.failedIterations <<
				", elapsed=" << elapsed << "ms" << std::endl;
		}
		valid = valid && timedCase;
	}
	return valid;
}

bool Application::exerciseLiveDecisionPlanSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int hammerId = getCardIdFromName("Crimson Hammer");
	int fireManaId = getCardIdFromName("Deadly Fighter Braid Claw");
	int targetId = getCardIdFromName("Burning Mane");
	if (hammerId < 0 || fireManaId < 0 || targetId < 0) return false;

	Duel live;
	live.mInputLoopRunning = false;
	live.mPlayerType[0] = PLAYER_AI;
	live.mTurn = 0;
	live.mTurnPhase = TURN_PHASE_MAIN;
	auto addCard = [&live](int cardId, int owner, int zone) -> int
	{
		int uid = (int)live.mCardList.size();
		Card* card = new Card(uid, cardId, owner);
		live.mCardList.push_back(card);
		live.getZone(owner, zone)->addCard(card);
		card->mZone = zone;
		live.mNextUniqueId = uid + 1;
		return uid;
	};
	int hammer = addCard(hammerId, 0, ZONE_HAND);
	int mana1 = addCard(fireManaId, 0, ZONE_MANA);
	int mana2 = addCard(fireManaId, 0, ZONE_MANA);
	int target = addCard(targetId, 1, ZONE_BATTLE);

	std::vector<DecisionPlan> plans = enumerateDecisionPlans(live);
	DecisionPlan selected;
	bool found = false;
	for (std::vector<DecisionPlan>::const_iterator plan = plans.begin(); plan != plans.end(); ++plan)
	{
		std::map<std::string, std::string>::const_iterator type = plan->action.map.find("msgtype");
		std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
		if (type != plan->action.map.end() && type->second == "cardplay" &&
			card != plan->action.map.end() &&
			std::atoi(card->second.c_str()) == hammer && plan->choices.size() == 1 &&
			plan->choices[0].selection == target)
		{
			selected = *plan;
			found = true;
			break;
		}
	}
	if (!found) return false;

	DecisionPlan illegal = selected;
	illegal.choices[0].selection = 9999;
	bool valid = commitDecisionPlan(live, illegal) == DecisionPlanCommitStatus::Illegal &&
		live.mMsgMngr.messages.empty() && live.mCardList[hammer]->mZone == ZONE_HAND;
	valid = valid && commitDecisionPlan(live, selected) == DecisionPlanCommitStatus::Committed &&
		live.mCastingCard == -1 && !live.mMsgMngr.messages.empty() &&
		live.mCardList[hammer]->mZone == ZONE_HAND;
	{
		ActiveDuelGuard activeGuard(live);
		live.dispatchAllMessages();
	}
	valid = valid && live.mMsgMngr.messages.empty() && !live.mChoiceResolver &&
		!live.hasSimulationChoiceFailure() && live.mCardList[hammer]->mZone == ZONE_GRAVEYARD &&
		live.mCardList[target]->mZone == ZONE_GRAVEYARD &&
		live.mCardList[mana1]->mIsTapped && live.mCardList[mana2]->mIsTapped;

	int proclamationId = getCardIdFromName("Proclamation of Death");
	if (proclamationId < 0) return false;
	Duel externalChoice;
	externalChoice.mInputLoopRunning = false;
	externalChoice.mPlayerType[0] = PLAYER_AI;
	externalChoice.mTurn = 0;
	externalChoice.mTurnPhase = TURN_PHASE_MAIN;
	auto addExternalCard = [&externalChoice](int cardId, int owner, int zone) -> int
	{
		int uid = (int)externalChoice.mCardList.size();
		Card* card = new Card(uid, cardId, owner);
		externalChoice.mCardList.push_back(card);
		externalChoice.getZone(owner, zone)->addCard(card);
		card->mZone = zone;
		externalChoice.mNextUniqueId = uid + 1;
		return uid;
	};
	int proclamation = addExternalCard(proclamationId, 0, ZONE_HAND);
	for (int mana = 0; mana < 4; ++mana)
		addExternalCard(proclamationId, 0, ZONE_MANA);
	addExternalCard(targetId, 1, ZONE_BATTLE);
	addExternalCard(fireManaId, 1, ZONE_BATTLE);
	std::vector<DecisionPlan> externalPlans = enumerateDecisionPlans(externalChoice);
	bool externalCommitted = false;
	for (std::vector<DecisionPlan>::const_iterator plan = externalPlans.begin();
		plan != externalPlans.end(); ++plan)
	{
		std::map<std::string, std::string>::const_iterator type = plan->action.map.find("msgtype");
		std::map<std::string, std::string>::const_iterator card = plan->action.map.find("card");
		if (type == plan->action.map.end() || type->second != "cardplay" ||
			card == plan->action.map.end() || std::atoi(card->second.c_str()) != proclamation)
			continue;
		externalCommitted = commitDecisionPlan(externalChoice, *plan) ==
			DecisionPlanCommitStatus::Committed;
		break;
	}
	valid = valid && externalCommitted && !externalChoice.mChoiceResolver &&
		!externalChoice.mMsgMngr.messages.empty();
	return valid;
}

bool Application::exerciseAiDriverSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	int attackerId = getCardIdFromName("Burning Mane");
	int shieldCreatureId = getCardIdFromName("Aqua Surfer");
	int shieldSpellId = getCardIdFromName("Spastic Missile");
	int valuableTargetId = getCardIdFromName("Bolshack Dragon");
	if (attackerId < 0 || shieldCreatureId < 0 || shieldSpellId < 0 ||
		valuableTargetId < 0)
		return false;
	MctsConfig liveMainConfig = liveMctsConfig();
	MctsConfig liveCombatConfig = liveMctsConfig(true);
	bool valid = liveMainConfig.iterations == 1024 && liveCombatConfig.iterations == 1024 &&
		liveMainConfig.timeBudgetMs == 1500 && liveCombatConfig.timeBudgetMs == 2500;

	Duel live;
	live.mInputLoopRunning = false;
	live.mPlayerType[0] = PLAYER_AI;
	live.mTurn = 0;
	live.mTurnPhase = TURN_PHASE_MAIN;
	auto addCard = [&live, attackerId](int owner, int zone) -> int
	{
		int uid = (int)live.mCardList.size();
		Card* card = new Card(uid, attackerId, owner);
		live.mCardList.push_back(card);
		live.getZone(owner, zone)->addCard(card);
		card->mZone = zone;
		live.mNextUniqueId = uid + 1;
		return uid;
	};
	int attacker = addCard(0, ZONE_BATTLE);
	live.mCardList[attacker]->mSummoningSickness = 0;
	for (int copy = 0; copy < 3; ++copy)
	{
		addCard(0, ZONE_DECK);
		addCard(1, ZONE_DECK);
	}

	MctsConfig config;
	config.iterations = 64;
	config.maxDepth = 6;
	config.seed = 49979687U;
	AiDecisionOutcome searched = playAiDecision(live, 0, "balanced", config);
	valid = valid && searched.source == AiDecisionSource::Mcts &&
		searched.action.getType() == "creatureattack" &&
		!live.mMsgMngr.messages.empty() && !live.mCardList[attacker]->mIsTapped &&
		live.mTurnPhase == TURN_PHASE_ATTACK;

	Duel fallback;
	fallback.mInputLoopRunning = false;
	fallback.mPlayerType[0] = PLAYER_AI;
	fallback.mTurn = 0;
	fallback.mTurnPhase = TURN_PHASE_MAIN;
	Card* fallbackAttacker = new Card(0, attackerId, 0);
	fallback.mCardList.push_back(fallbackAttacker);
	fallback.mBattlezones[0].addCard(fallbackAttacker);
	fallbackAttacker->mZone = ZONE_BATTLE;
	fallbackAttacker->mSummoningSickness = 0;
	fallback.mNextUniqueId = 1;
	MctsConfig disabled;
	disabled.iterations = 0;
	AiDecisionOutcome heuristic = playAiDecision(fallback, 0, "balanced", disabled);
	valid = valid && heuristic.source == AiDecisionSource::Heuristic &&
		heuristic.action.getType() == "creatureattack" && fallback.mTurn == 0 &&
		!fallback.mMsgMngr.messages.empty();

	Duel forced;
	forced.mInputLoopRunning = false;
	forced.mPlayerType[0] = PLAYER_AI;
	forced.mTurn = 0;
	forced.mTurnPhase = TURN_PHASE_MAIN;
	AiDecisionOutcome onlyMove = playAiDecision(forced, 0, "balanced", config);
	valid = valid && onlyMove.source == AiDecisionSource::Forced &&
		onlyMove.action.getType() == "endturn" && !forced.mMsgMngr.messages.empty();

	Duel manaPlacement;
	manaPlacement.mInputLoopRunning = false;
	manaPlacement.mPlayerType[0] = PLAYER_AI;
	manaPlacement.mTurn = 0;
	manaPlacement.mTurnPhase = TURN_PHASE_MANA;
	for (int copy = 0; copy < 6; ++copy)
	{
		int uid = (int)manaPlacement.mCardList.size();
		Card* card = new Card(uid, attackerId, 0);
		manaPlacement.mCardList.push_back(card);
		manaPlacement.mHands[0].addCard(card);
		card->mZone = ZONE_HAND;
		manaPlacement.mNextUniqueId = uid + 1;
	}
	AiDecisionOutcome placed = playHeuristicManaPlacement(
		manaPlacement, 0, "balanced");
	valid = valid && placed.source == AiDecisionSource::ManaHeuristic &&
		placed.action.getType() == "cardmana" && manaPlacement.mManaUsed == 1 &&
		manaPlacement.mTurnPhase == TURN_PHASE_MAIN &&
		!manaPlacement.mMsgMngr.messages.empty();

	Duel manaPayment;
	manaPayment.mInputLoopRunning = false;
	manaPayment.mPlayerType[0] = PLAYER_AI;
	manaPayment.mTurn = 0;
	manaPayment.mTurnPhase = TURN_PHASE_MAIN;
	auto addPaymentCard = [&manaPayment, attackerId](int zone) -> int
	{
		int uid = (int)manaPayment.mCardList.size();
		Card* card = new Card(uid, attackerId, 0);
		manaPayment.mCardList.push_back(card);
		manaPayment.getZone(0, zone)->addCard(card);
		card->mZone = zone;
		manaPayment.mNextUniqueId = uid + 1;
		return uid;
	};
	int spell = addPaymentCard(ZONE_HAND);
	addPaymentCard(ZONE_MANA);
	addPaymentCard(ZONE_MANA);
	Message cast("cardplay");
	cast.addValue("card", spell);
	cast.addValue("evobait", -1);
	cast.addValue("evobait2", -1);
	manaPayment.handleInterfaceInput(cast);
	AiDecisionOutcome paid = playHeuristicManaPayment(manaPayment, 0, "balanced");
	valid = valid && paid.source == AiDecisionSource::ManaHeuristic &&
		paid.action.getType() == "manatap" && manaPayment.mCastingCard == -1 &&
		manaPayment.mMsgMngr.messages.size() == 3;

	Duel shieldTarget;
	shieldTarget.mInputLoopRunning = false;
	shieldTarget.mPlayerType[0] = PLAYER_AI;
	shieldTarget.mTurn = 0;
	shieldTarget.mTurnPhase = TURN_PHASE_ATTACK;
	shieldTarget.mAttackphase = PHASE_TARGET;
	shieldTarget.mRandomGen.SetRandomSeed(104729U);
	auto addShieldCard = [&shieldTarget, attackerId](int owner, int zone) -> int
	{
		int uid = (int)shieldTarget.mCardList.size();
		Card* card = new Card(uid, attackerId, owner);
		shieldTarget.mCardList.push_back(card);
		shieldTarget.getZone(owner, zone)->addCard(card);
		card->mZone = zone;
		shieldTarget.mNextUniqueId = uid + 1;
		return uid;
	};
	int shieldAttacker = addShieldCard(0, ZONE_BATTLE);
	shieldTarget.mAttacker = shieldAttacker;
	shieldTarget.mDefender = 1;
	shieldTarget.mDefenderType = DEFENDER_PLAYER;
	shieldTarget.mBreakCount = 1;
	for (int shield = 0; shield < 3; ++shield)
		addShieldCard(1, ZONE_SHIELD);
	AiDecisionOutcome targeted = playRandomShieldTarget(shieldTarget, 0);
	valid = valid && targeted.source == AiDecisionSource::ShieldRandom &&
		targeted.action.getType() == "targetshield" &&
		shieldTarget.mShieldTargets.size() == 1 &&
		shieldTarget.mShieldTargets[0] == targeted.action.getInt("shield");

	auto prepareTrigger = [](Duel& duel, int cardId, int cardType) -> int
	{
		duel.mInputLoopRunning = false;
		duel.mPlayerType[1] = PLAYER_AI;
		duel.mTurn = 0;
		duel.mTurnPhase = TURN_PHASE_ATTACK;
		duel.mAttackphase = PHASE_TRIGGER;
		Card* trigger = new Card(0, cardId, 1);
		trigger->mType = cardType;
		trigger->mIsShieldTrigger = 1;
		duel.mCardList.push_back(trigger);
		duel.mHands[1].addCard(trigger);
		trigger->mZone = ZONE_HAND;
		duel.mShieldTargets.push_back(0);
		duel.mNextUniqueId = 1;
		return 0;
	};

	Duel creatureTrigger;
	int creatureTriggerCard = prepareTrigger(creatureTrigger, shieldCreatureId, TYPE_CREATURE);
	AiDecisionOutcome usedCreature = playHeuristicShieldTrigger(
		creatureTrigger, 1, "balanced");
	valid = valid && usedCreature.source == AiDecisionSource::ShieldTriggerHeuristic &&
		usedCreature.action.getType() == "triggeruse" &&
		usedCreature.action.getInt("trigger") == creatureTriggerCard &&
		!creatureTrigger.mAiThinking.load();

	Duel uncastableSpellTrigger;
	prepareTrigger(uncastableSpellTrigger, shieldSpellId, TYPE_SPELL);
	AiDecisionOutcome skippedSpell = playHeuristicShieldTrigger(
		uncastableSpellTrigger, 1, "balanced");
	valid = valid && skippedSpell.source == AiDecisionSource::ShieldTriggerHeuristic &&
		skippedSpell.action.getType() == "triggerskip" &&
		!uncastableSpellTrigger.mAiThinking.load();

	Duel castableSpellTrigger;
	int spellTriggerCard = prepareTrigger(castableSpellTrigger, shieldSpellId, TYPE_SPELL);
	Card* spellTarget = new Card(1, attackerId, 0);
	castableSpellTrigger.mCardList.push_back(spellTarget);
	castableSpellTrigger.mBattlezones[0].addCard(spellTarget);
	spellTarget->mZone = ZONE_BATTLE;
	castableSpellTrigger.mNextUniqueId = 2;
	AiDecisionOutcome usedSpell = playHeuristicShieldTrigger(
		castableSpellTrigger, 1, "balanced");
	valid = valid && usedSpell.source == AiDecisionSource::ShieldTriggerHeuristic &&
		usedSpell.action.getType() == "triggeruse" &&
		usedSpell.action.getInt("trigger") == spellTriggerCard;

	Duel triggerChoice;
	int choiceSpell = prepareTrigger(triggerChoice, shieldSpellId, TYPE_SPELL);
	Card* lowTarget = new Card(1, attackerId, 0);
	Card* highTarget = new Card(2, valuableTargetId, 0);
	triggerChoice.mCardList.push_back(lowTarget);
	triggerChoice.mCardList.push_back(highTarget);
	triggerChoice.mBattlezones[0].addCard(lowTarget);
	triggerChoice.mBattlezones[0].addCard(highTarget);
	lowTarget->mZone = ZONE_BATTLE;
	highTarget->mZone = ZONE_BATTLE;
	triggerChoice.mNextUniqueId = 3;
	triggerChoice.addChoice("Choose an opponent's creature to destroy", 0,
		choiceSpell, 1, LUA_REFNIL, LUA_REFNIL);
	triggerChoice.mChoiceValidCards.push_back(lowTarget->mUniqueId);
	triggerChoice.mChoiceValidCards.push_back(highTarget->mUniqueId);
	triggerChoice.mLuaCallbackSuspended = true;
	AiDecisionOutcome targetedTrigger = playHeuristicShieldTrigger(
		triggerChoice, 1, "balanced");
	valid = valid && targetedTrigger.source == AiDecisionSource::ShieldTriggerHeuristic &&
		targetedTrigger.action.getType() == "choiceselect" &&
		targetedTrigger.action.getInt("selection") == highTarget->mUniqueId &&
		!triggerChoice.mAiThinking.load();
	triggerChoice.mLuaCallbackSuspended = false;

	Duel suspendedChoice;
	suspendedChoice.mInputLoopRunning = false;
	suspendedChoice.mTurn = 0;
	suspendedChoice.mTurnPhase = TURN_PHASE_MAIN;
	auto addChoiceCard = [&suspendedChoice, attackerId]() -> int
	{
		int uid = (int)suspendedChoice.mCardList.size();
		Card* card = new Card(uid, attackerId, 1);
		suspendedChoice.mCardList.push_back(card);
		suspendedChoice.mBattlezones[1].addCard(card);
		card->mZone = ZONE_BATTLE;
		suspendedChoice.mNextUniqueId = uid + 1;
		return uid;
	};
	int firstChoice = addChoiceCard();
	int secondChoice = addChoiceCard();
	suspendedChoice.addChoice("Choose one of your creatures to put into your mana zone",
		0, firstChoice, 1, LUA_REFNIL, LUA_REFNIL);
	suspendedChoice.mChoiceValidCards.push_back(firstChoice);
	suspendedChoice.mChoiceValidCards.push_back(secondChoice);
	suspendedChoice.mLuaCallbackSuspended = true;
	AiDecisionOutcome transient = playHeuristicDecision(suspendedChoice, 1, "balanced");
	valid = valid && !suspendedChoice.isCloneable() &&
		transient.source == AiDecisionSource::Heuristic &&
		transient.action.getType() == "choiceselect" &&
		suspendedChoice.mMsgMngr.hasMoreMessages();
	suspendedChoice.mLuaCallbackSuspended = false;
	if (!valid)
	{
		std::cerr << "AI driver state: searched-source=" << (int)searched.source <<
			", searched-action=" << searched.action.getType() <<
			", queued=" << live.mMsgMngr.messages.size() <<
			", tapped=" << live.mCardList[attacker]->mIsTapped <<
			", phase=" << live.mTurnPhase << ", heuristic-source=" <<
			(int)heuristic.source << ", heuristic-action=" << heuristic.action.getType() <<
			", fallback-turn=" << fallback.mTurn << ", forced-source=" <<
			(int)onlyMove.source << ", forced-action=" << onlyMove.action.getType() << std::endl;
	}
	return valid;
}

bool Application::exerciseBackgroundMctsSmoke()
{
	int attackerId = getCardIdFromName("Burning Mane");
	if (attackerId < 0) return false;

	Duel live;
	live.mInputLoopRunning = false;
	live.mPlayerType[1] = PLAYER_AI;
	live.mTurn = 1;
	live.mTurnPhase = TURN_PHASE_ATTACK;
	live.mAttackphase = PHASE_TARGET;
	int attacker = -1;
	MctsConfig config;
	config.iterations = 128;
	config.maxDepth = 6;
	config.seed = 67867967U;
	BackgroundMctsSearch search(1, config);
	{
		std::lock_guard<std::mutex> lock(gMutex);
		auto addCard = [&live, attackerId](int owner, int zone) -> int
		{
			int uid = (int)live.mCardList.size();
			Card* card = new Card(uid, attackerId, owner);
			live.mCardList.push_back(card);
			live.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			live.mNextUniqueId = uid + 1;
			return uid;
		};
		attacker = addCard(1, ZONE_BATTLE);
		live.mCardList[attacker]->mSummoningSickness = 0;
		live.mAttacker = attacker;
		live.mDefender = 0;
		live.mDefenderType = DEFENDER_PLAYER;
		live.mBreakCount = 1;
		for (int shield = 0; shield < 3; ++shield)
			addCard(0, ZONE_SHIELD);
		for (int copy = 0; copy < 3; ++copy)
		{
			addCard(0, ZONE_DECK);
			addCard(1, ZONE_DECK);
		}
		mAiCreaturePowers.resize(live.mCardList.size());
		for (size_t card = 0; card < live.mCardList.size(); ++card)
			mAiCreaturePowers[card] = live.mCardList[card]->mPower;
		mAiCreaturePowers[attacker] = live.getCreaturePower(attacker);
		live.mAiThinkingPlayer = live.getPlayerToMove();
		live.mAiThinking = true;
		if (!search.start(live))
		{
			live.mAiThinking = false;
			live.mAiThinkingPlayer = -1;
			mAiCreaturePowers.clear();
			return false;
		}
	}

	bool uiMutexAvailable = gMutex.try_lock();
	bool liveStateStayedFrozen = false;
	if (uiMutexAvailable)
	{
		liveStateStayedFrozen = live.mTurn == 1 && live.mMsgMngr.messages.empty() &&
			live.mShieldTargets.empty() && !live.mCardList[attacker]->mIsTapped;
		gMutex.unlock();
	}

	Duel* savedDuel = mDuel;
	mDuel = &live;
	bool uiUsedCachedDecisionOwner = true;
	Uint32 deadline = SDL_GetTicks() + 5000;
	while (!search.isFinished() && SDL_GetTicks() < deadline)
	{
		renderDuel();
		uiUsedCachedDecisionOwner = uiUsedCachedDecisionOwner &&
			live.getPlayerToMove() == 1;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	mDuel = savedDuel;
	MctsResult result;
	bool finished = search.finish(result);
	bool restarted = false;
	if (finished)
	{
		std::lock_guard<std::mutex> lock(gMutex);
		restarted = search.start(live, config);
	}
	deadline = SDL_GetTicks() + 5000;
	while (restarted && !search.isFinished() && SDL_GetTicks() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	MctsResult reusedResult;
	bool finishedReuse = restarted && search.finish(reusedResult);
	if (finishedReuse) result = reusedResult;
	bool committed = false;
	{
		std::lock_guard<std::mutex> lock(gMutex);
		live.mAiThinking = false;
		live.mAiThinkingPlayer = -1;
		committed = finished && result.hasPlan &&
			commitDecisionPlan(live, result.plan) == DecisionPlanCommitStatus::Committed;
	}
	mAiCreaturePowers.clear();
	return uiMutexAvailable && liveStateStayedFrozen && finished && finishedReuse &&
		result.iterationsCompleted == config.iterations && result.failedIterations == 0 &&
		result.reusedTree && result.reusedRootVisits == config.iterations &&
		result.rootChildren.size() == 1 && result.selectedVisits > 0 &&
		uiUsedCachedDecisionOwner && committed &&
		!live.mMsgMngr.messages.empty();
}

bool Application::exerciseBundledDecksSmoke()
{
	std::vector<std::string> deckFiles;
	bool valid = collectDeckFiles("Decks", deckFiles);
	int lukiaLex = getCardIdFromName("Lukia Lex, Pinnacle Guardian");
	int bronzeArmTribe = getCardIdFromName("Bronze-Arm Tribe");
	bool flexibleNamesReady = lukiaLex >= 0 && bronzeArmTribe >= 0 &&
		getDeckCardIdFromName("lukia lex pinnacle guardian") == lukiaLex &&
		getDeckCardIdFromName("LUKIA LEX,,, PINNACLE GUARDIAN") == lukiaLex &&
		getDeckCardIdFromName("Lukia Lex,Pinnacle Guardian") == lukiaLex &&
		getDeckCardIdFromName("bronze arm tribe") == bronzeArmTribe &&
		getDeckCardIdFromName("BRONZE - ARM TRIBE") == bronzeArmTribe &&
		getDeckCardIdFromName("Lukia Lex. Pinnacle Guardian") < 0;
	if (!flexibleNamesReady)
	{
		std::cerr << "Flexible deck card-name matching smoke test failed." << std::endl;
		valid = false;
	}
	std::sort(deckFiles.begin(), deckFiles.end());
	if (deckFiles.empty())
	{
		std::cerr << "No bundled deck files were found beneath Decks/." << std::endl;
		return false;
	}

	for (size_t deck = 0; deck < deckFiles.size(); ++deck)
	{
		std::vector<int> cardIds;
		if (!loadDeckCardIds(deckFiles[deck], cardIds, 40))
		{
			std::cerr << "Invalid bundled deck: " << deckFiles[deck] << std::endl;
			valid = false;
		}
	}
	return valid;
}

bool Application::exerciseNpcRewardsSmoke()
{
	const int expectedGoldTiers[] = { 200, 400, 800, 1500, 3000 };
	for (int tier = 1; tier <= 5; ++tier)
		if (npcGoldRewardValue(tier) != expectedGoldTiers[tier - 1]) return false;
	if (npcGoldRewardValue(0) != 0 || npcGoldRewardValue(6) != 0) return false;
	for (size_t npcIndex = 0; npcIndex < mNpcs.size(); ++npcIndex)
	{
		const Npc& npc = mNpcs[npcIndex];
		for (size_t rewardIndex = 0; rewardIndex < npc.rewards.size(); ++rewardIndex)
		{
			const int tier = npc.rewards[rewardIndex].goldTier;
			if (tier < 1 || tier > 5 || npcGoldRewardValue(tier) <= 0) return false;
		}
	}
	return true;
}

bool Application::exerciseAtmosphereSmoke()
{
	OverworldAtmosphere atmosphere;
	bool valid = atmosphere.day() == 1 && atmosphere.minuteOfDay() == 8 * 60 &&
		atmosphere.clockText() == "08:00" && atmosphere.daylight() > 0.99f &&
		atmosphere.weather() == WeatherKind::Clear && atmosphere.weatherIntensity() == 0.f;

	OverworldAtmosphereState saved = atmosphere.state();
	saved.day = 2;
	saved.minuteOfDay = 23 * 60 + 59;
	saved.weather = WeatherKind::Rain;
	saved.weatherIntensity = 0.75f;
	saved.weatherRemaining = 10000;
	saved.weatherFadingOut = false;
	atmosphere.restore(saved);
	valid = valid && atmosphere.nightOverlayAlpha() > 100 &&
		atmosphere.weather() == WeatherKind::Rain &&
		std::fabs(atmosphere.weatherIntensity() - 0.75f) < 0.001f;
	atmosphere.update(500);
	valid = valid && atmosphere.day() == 3 && atmosphere.minuteOfDay() == 0 &&
		atmosphere.clockText() == "00:00";

	saved = atmosphere.state();
	saved.minuteOfDay = 6 * 60 + 45;
	saved.weather = WeatherKind::Snow;
	saved.weatherIntensity = 1.f;
	saved.weatherRemaining = 5000;
	atmosphere.restore(saved);
	valid = valid && atmosphere.warmOverlayAlpha() > 0 &&
		std::string(OverworldAtmosphere::weatherName(atmosphere.weather())) == "Snow";

	WeatherKind parsed = WeatherKind::Clear;
	valid = valid && OverworldAtmosphere::parseWeather("rain", parsed) &&
		parsed == WeatherKind::Rain && OverworldAtmosphere::parseWeather("snow", parsed) &&
		parsed == WeatherKind::Snow && !OverworldAtmosphere::parseWeather("storm", parsed);

	saved = atmosphere.state();
	saved.weather = WeatherKind::Clear;
	saved.weatherIntensity = 0.f;
	saved.weatherRemaining = 0;
	saved.weatherFadingOut = false;
	atmosphere.restore(saved);
	atmosphere.update(1000);
	return valid && atmosphere.weather() != WeatherKind::Clear &&
		atmosphere.weatherIntensity() > 0.f;
}

bool Application::exerciseWorldObjectsSmoke()
{
	std::set<std::string> templateIds;
	int signposts = 0;
	int chests = 0;
	int bushes = 0;
	int rocks = 0;
	int environment = 0;
	for (size_t index = 0; index < mWorldObjectTemplates.size(); ++index)
	{
		const WorldObjectTemplate& objectTemplate = mWorldObjectTemplates[index];
		if (objectTemplate.id.empty() ||
			!templateIds.insert(objectTemplate.id).second) return false;
		const WorldObject& object = objectTemplate.object;
		if (object.kind == WorldObjectKind::Signpost) ++signposts;
		else if (object.kind == WorldObjectKind::Chest)
		{
			++chests;
			if (object.spriteSheet.find("/!Chest.png") == std::string::npos ||
				object.spriteIndex != 0 || object.openedText.empty()) return false;
		}
		else if (object.kind == WorldObjectKind::CuttableBush) ++bushes;
		else if (object.kind == WorldObjectKind::SmashableRock) ++rocks;
		else if (object.kind == WorldObjectKind::Environment)
		{
			++environment;
			if (!object.animated || object.spriteSheet.empty() ||
				object.spriteIndex < 0 || object.spriteIndex > 7 ||
				object.spriteRow < 0 || object.spriteRow > 3) return false;
		}
	}
	if (signposts != 1 || chests != 1 || bushes != 1 || rocks != 1 ||
		environment != 8 * 8 * 4) return false;
	std::vector<WorldObjectTemplate>::const_iterator bush = std::find_if(
		mWorldObjectTemplates.begin(), mWorldObjectTemplates.end(),
		[](const WorldObjectTemplate& objectTemplate)
		{
			return objectTemplate.id == "cuttable_bush";
		});
	if (bush == mWorldObjectTemplates.end()) return false;
	WorldObject instance = createWorldObject(*bush, "cuttable_bush_smoke");
	bool valid = instance.editorCreated && instance.templateId == "cuttable_bush" &&
		instance.id == "cuttable_bush_smoke" &&
		instance.kind == WorldObjectKind::CuttableBush &&
		getCardIdFromName("Xeno Mantis") >= 0 &&
		getCardIdFromName("Smash Warrior Stagrandu") >= 0;
	if (!valid) return false;

	const int savedArea = mCurrentWorldArea;
	const WorldBuilderTab savedTab = mWorldBuilderTab;
	const bool savedPalette = mWorldBuilderObjectPalette;
	const int savedSelection = mWorldBuilderSelectedObject;
	const bool savedDirty = mWorldBuilderDirty;
	const bool savedUndoPending = mWorldBuilderUndoPending;
	const WorldBuilderUndoAction savedPendingUndo = mWorldBuilderPendingUndo;
	const std::vector<WorldBuilderUndoAction> savedUndoHistory =
		mWorldBuilderUndoHistory;
	const std::vector<WorldObject> savedObjects = mWorldObjects;
	const std::string savedNotice = mWorldBuilderNotice;
	const bool savedNoticeError = mWorldBuilderNoticeError;
	const Uint32 savedNoticeUntil = mWorldBuilderNoticeUntil;
	mCurrentWorldArea = worldAreaIndex(mWorld.start.mapId);
	mWorldBuilderTab = WorldBuilderTab::Objects;
	mWorldBuilderObjectPalette = false;
	mWorldBuilderSelectedObject = -1;
	clearWorldBuilderUndoHistory();
	int freeX = -1;
	int freeY = -1;
	if (mCurrentWorldArea >= 0)
		for (int radius = 1; radius <= 40 && freeX < 0; ++radius)
			for (int y = std::max(0, mWorld.start.y - radius);
				y <= std::min((int)currentMap().size() - 1,
				mWorld.start.y + radius) && freeX < 0; ++y)
				for (int x = std::max(0, mWorld.start.x - radius);
					x <= std::min((int)currentMap()[y].size() - 1,
					mWorld.start.x + radius); ++x)
					if (worldBuilderCanPlace(x, y, -1, -1))
					{
						freeX = x;
						freeY = y;
						break;
					}
	const size_t originalCount = mWorldObjects.size();
	const int bushIndex = (int)(bush - mWorldObjectTemplates.begin());
	valid = freeX >= 0 && addWorldBuilderObject(bushIndex, freeX, freeY) &&
		mWorldObjects.size() == originalCount + 1 &&
		mWorldObjects.back().editorCreated &&
		mWorldObjects.back().templateId == "cuttable_bush";
	deleteWorldBuilderObject();
	valid = valid && mWorldObjects.size() == originalCount;
	undoWorldBuilder();
	valid = valid && mWorldObjects.size() == originalCount + 1 &&
		mWorldObjects.back().editorCreated;
	undoWorldBuilder();
	valid = valid && mWorldObjects.size() == originalCount &&
		mWorldBuilderUndoHistory.empty() && mWorldBuilderDirty == savedDirty;
	mWorldObjects = savedObjects;
	mCurrentWorldArea = savedArea;
	mWorldBuilderTab = savedTab;
	mWorldBuilderObjectPalette = savedPalette;
	mWorldBuilderSelectedObject = savedSelection;
	mWorldBuilderDirty = savedDirty;
	mWorldBuilderUndoPending = savedUndoPending;
	mWorldBuilderPendingUndo = savedPendingUndo;
	mWorldBuilderUndoHistory = savedUndoHistory;
	mWorldBuilderNotice = savedNotice;
	mWorldBuilderNoticeError = savedNoticeError;
	mWorldBuilderNoticeUntil = savedNoticeUntil;
	return valid;
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

bool Application::exerciseHollowCardsSmoke()
{
	std::lock_guard<std::mutex> lock(gMutex);
	struct ExpectedCard
	{
		const char* name;
		int cost;
		int power;
		int breaker;
		int blocker;
	};
	const ExpectedCard expected[] = {
		{ "Hollow Soldier", 1, 3000, 1, 0 },
		{ "Hollow Hulcus", 3, 3000, 1, 0 },
		{ "Hollow Tribe", 3, 3000, 1, 0 },
		{ "Hollow Knight", 3, 6000, 2, 0 },
		{ "Hollow Dragon", 7, 15000, 3, 0 },
		{ "Hollow Guardian", 2, 5000, 1, 1 },
		{ "Hollow Angel", 6, 10000, 1, 1 },
		{ "Hollow Demon", 6, 6000, 2, 0 },
		{ "Hollow Giant", 5, 7000, 2, 0 },
		{ "Pure Hollow", 7, 11000, 2, 0 }
	};
	std::map<std::string, int> ids;
	for (size_t index = 0; index < sizeof(expected) / sizeof(expected[0]); ++index)
	{
		int cardId = getCardIdFromName(expected[index].name);
		if (cardId < 0) return false;
		Card card((int)index, cardId, 0);
		if (card.mCivilization != CIV_HOLLOW ||
			(card.mCivilizations & (1 << CIV_HOLLOW)) == 0 || card.mRace != "Hollow" ||
			card.mManaCost != expected[index].cost || card.mPower != expected[index].power ||
			card.mBreaker != expected[index].breaker || card.mIsBlocker != expected[index].blocker ||
			cardTextureById(cardId) == NULL)
			return false;
		ids[expected[index].name] = cardId;
	}
	int pitId = getCardIdFromName("Pit of Hollows");
	if (pitId < 0) return false;
	Card pitCard(100, pitId, 0);
	if (pitCard.mCivilization != CIV_HOLLOW ||
		(pitCard.mCivilizations & (1 << CIV_HOLLOW)) == 0 ||
		pitCard.mType != TYPE_SPELL || pitCard.mManaCost != 6 ||
		pitCard.mIsShieldTrigger != 1 || cardTextureById(pitId) == NULL)
		return false;
	ids["Pit of Hollows"] = pitId;

	int nonHollowId = getCardIdFromName("Bone Spider");
	if (nonHollowId < 0) return false;
	Duel* savedActiveDuel = ActiveDuel;
	bool valid = true;
	{
		Duel test;
		test.mIsSimulation = true;
		ActiveDuel = &test;
		auto addCard = [&test](int cardId, int owner, int zone) -> int
		{
			int uid = (int)test.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			test.mCardList.push_back(card);
			test.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};

		int soldier = addCard(ids["Hollow Soldier"], 0, ZONE_BATTLE);
		int knight = addCard(ids["Hollow Knight"], 0, ZONE_BATTLE);
		int pure = addCard(ids["Pure Hollow"], 0, ZONE_BATTLE);
		int guardian = addCard(ids["Hollow Guardian"], 0, ZONE_BATTLE);
		int angel = addCard(ids["Hollow Angel"], 0, ZONE_BATTLE);
		int nonHollow = addCard(nonHollowId, 0, ZONE_BATTLE);
		int enemyHollow = addCard(ids["Hollow Soldier"], 1, ZONE_BATTLE);
		int pureInHand = addCard(ids["Pure Hollow"], 0, ZONE_HAND);

		valid = test.cardHasCivilization(soldier, CIV_HOLLOW) &&
			!test.cardHasCivilization(soldier, CIV_DARKNESS) &&
			test.getCreaturePower(soldier) == 7000 && test.getCreatureBreaker(soldier) == 2 &&
			test.getCreaturePower(knight) == 10000 && test.getCreatureBreaker(knight) == 3 &&
			test.getCreaturePower(pure) == 11000 && test.getCreatureBreaker(pure) == 2 &&
			test.getCreaturePower(enemyHollow) == 3000 &&
			test.getCreatureCanEvolve(pureInHand, soldier) == 1 &&
			test.getCreatureCanEvolve(pureInHand, nonHollow) == 0 &&
			test.getCreatureIsBlocker(guardian) == 1 &&
			test.getCreatureCanAttackPlayers(guardian) == CANATTACK_NO &&
			test.getCreatureCanAttackCreature(guardian, enemyHollow) == CANATTACK_NO &&
			test.getCreatureIsBlocker(angel) == 1 &&
			test.getCreatureCanBlockRepeatedly(angel) == 1;
	}
	{
		Duel abilities;
		abilities.mIsSimulation = true;
		ActiveDuel = &abilities;
		auto addCard = [&abilities](int cardId, int owner, int zone) -> int
		{
			int uid = (int)abilities.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			abilities.mCardList.push_back(card);
			abilities.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};
		auto summon = [&abilities](int card)
		{
			Message move("cardmove");
			move.addValue("card", card);
			move.addValue("from", ZONE_HAND);
			move.addValue("to", ZONE_BATTLE);
			abilities.dispatchMessage(move);
			abilities.dispatchAllMessages();
		};

		for (int card = 0; card < 5; ++card) addCard(nonHollowId, 0, ZONE_DECK);
		int hulcus = addCard(ids["Hollow Hulcus"], 0, ZONE_HAND);
		int tribe = addCard(ids["Hollow Tribe"], 0, ZONE_HAND);
		summon(hulcus);
		valid = valid && abilities.mHands[0].mCards.size() == 3 &&
			abilities.mDecks[0].mCards.size() == 3;
		summon(tribe);
		valid = valid && abilities.mManazones[0].mCards.size() == 2 &&
			abilities.mDecks[0].mCards.size() == 1;

		addCard(nonHollowId, 1, ZONE_MANA);
		addCard(nonHollowId, 1, ZONE_MANA);
		addCard(nonHollowId, 1, ZONE_BATTLE);
		int giant = addCard(ids["Hollow Giant"], 0, ZONE_HAND);
		int demon = addCard(ids["Hollow Demon"], 0, ZONE_HAND);
		auto chooseFirst = [](const Duel& position) -> int
		{
			return position.mChoiceValidCards.empty() ? RETURN_QUIT :
				position.mChoiceValidCards.front();
		};
		abilities.setChoiceResolver(chooseFirst, 2);
		summon(giant);
		valid = valid && !abilities.mSimulationChoiceFailed &&
			abilities.mManazones[1].mCards.empty() &&
			abilities.mGraveyards[1].mCards.size() == 2;
		abilities.setChoiceResolver(chooseFirst, 1);
		summon(demon);
		valid = valid && !abilities.mSimulationChoiceFailed &&
			abilities.mBattlezones[1].mCards.empty() &&
			abilities.mGraveyards[1].mCards.size() == 3;
	}
	{
		Duel spell;
		spell.mIsSimulation = true;
		spell.mRandomGen.SetRandomSeed(65537U);
		ActiveDuel = &spell;
		auto addCard = [&spell](int cardId, int owner, int zone) -> int
		{
			int uid = (int)spell.mCardList.size();
			Card* card = new Card(uid, cardId, owner);
			spell.mCardList.push_back(card);
			spell.getZone(owner, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};

		int pit = addCard(ids["Pit of Hollows"], 0, ZONE_HAND);
		for (int i = 0; i < 2; ++i)
		{
			addCard(nonHollowId, 1, ZONE_HAND);
			addCard(nonHollowId, 1, ZONE_BATTLE);
			addCard(nonHollowId, 1, ZONE_MANA);
		}
		Message cast("cardmove");
		cast.addValue("card", pit);
		cast.addValue("from", ZONE_HAND);
		cast.addValue("to", ZONE_BATTLE);
		spell.dispatchMessage(cast);
		spell.dispatchAllMessages();
		valid = valid && spell.mCardList[pit]->mZone == ZONE_GRAVEYARD &&
			spell.mHands[1].mCards.size() == 1 &&
			spell.mBattlezones[1].mCards.size() == 1 &&
			spell.mManazones[1].mCards.size() == 1 &&
			spell.mGraveyards[1].mCards.size() == 3;
	}
	{
		Duel payment;
		payment.mIsSimulation = true;
		ActiveDuel = &payment;
		auto addCard = [&payment](int cardId, int zone) -> int
		{
			int uid = (int)payment.mCardList.size();
			Card* card = new Card(uid, cardId, 0);
			payment.mCardList.push_back(card);
			payment.getZone(0, zone)->addCard(card);
			card->mZone = zone;
			return uid;
		};
		auto setCivilization = [&payment](int card, int civilization)
		{
			payment.mCardList[card]->mCivilization = civilization;
			payment.mCardList[card]->mCivilizations = 1 << civilization;
		};

		int hollowMana = addCard(ids["Hollow Soldier"], ZONE_MANA);
		int fireMana = addCard(nonHollowId, ZONE_MANA);
		int waterMana = addCard(nonHollowId, ZONE_MANA);
		setCivilization(fireMana, CIV_FIRE);
		setCivilization(waterMana, CIV_WATER);
		int fireCard = addCard(nonHollowId, ZONE_HAND);
		setCivilization(fireCard, CIV_FIRE);
		payment.mCardList[fireCard]->mManaCost = 1;
		valid = valid && payment.canPayForCard(0, fireCard) &&
			payment.isThereUntappedManaOfCiv(0, CIV_NATURE);

		int hollowCard = addCard(ids["Hollow Guardian"], ZONE_HAND);
		payment.mCardList[hollowMana]->tap();
		valid = valid && payment.canPayForCard(0, hollowCard) &&
			payment.isThereUntappedManaOfCiv(0, CIV_HOLLOW);
		payment.mTurn = 0;
		payment.mCastingCard = hollowCard;
		payment.mCastingCivilizations = 1 << CIV_HOLLOW;
		payment.mCastingCost = 2;
		valid = valid && payment.canTapManaForCasting(fireMana) &&
			payment.canTapManaForCasting(waterMana);
		payment.resetCasting();
		payment.mCardList[hollowMana]->untap();

		int dualCard = addCard(nonHollowId, ZONE_HAND);
		payment.mCardList[dualCard]->mManaCost = 2;
		payment.mCardList[dualCard]->mCivilization = CIV_FIRE;
		payment.mCardList[dualCard]->mCivilizations =
			(1 << CIV_FIRE) | (1 << CIV_DARKNESS);
		valid = valid && payment.canPayForCard(0, dualCard);
		payment.mCardList[fireMana]->mCivilizations = 1 << CIV_WATER;
		payment.mCardList[fireMana]->mCivilization = CIV_WATER;
		valid = valid && !payment.canPayForCard(0, dualCard);
	}
	ActiveDuel = savedActiveDuel;
	return valid;
}

bool Application::exerciseUntapAfterBlockSmoke()
{
	int spiralGrassId = getCardIdFromName("Spiral Grass");
	int attackerId = getCardIdFromName("Deadly Fighter Braid Claw");
	if (spiralGrassId < 0 || attackerId < 0) return false;

	Duel* savedActiveDuel = ActiveDuel;
	bool valid = true;
	for (int blocked = 0; blocked <= 1; ++blocked)
	{
		Duel test;
		test.mIsSimulation = true;
		ActiveDuel = &test;

		Card* blocker = new Card(0, spiralGrassId, 0);
		Card* attacker = new Card(1, attackerId, 1);
		test.mCardList.push_back(blocker);
		test.mCardList.push_back(attacker);
		test.mBattlezones[0].addCard(blocker);
		test.mBattlezones[1].addCard(attacker);
		blocker->mZone = ZONE_BATTLE;
		attacker->mZone = ZONE_BATTLE;
		blocker->tap();

		Message battle("creaturebattle");
		battle.addValue("attacker", attacker->mUniqueId);
		battle.addValue("defender", blocker->mUniqueId);
		battle.addValue("blocked", blocked);
		test.dispatchMessage(battle);
		test.dispatchAllMessages();

		valid = valid && blocker->mZone == ZONE_BATTLE &&
			blocker->mIsTapped == (blocked == 0);
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
	int cardId = getCardIdFromName("Burning Mane");
	if (cardId < 0) return false;
	Duel duel;
	duel.mInputLoopRunning = false;
	duel.mTurn = 0;
	duel.mTurnPhase = TURN_PHASE_MANA;
	auto addCard = [&duel, cardId](int zone, int cost, int civilizations) -> int
	{
		int uid = static_cast<int>(duel.mCardList.size());
		Card* card = new Card(uid, cardId, 0);
		card->mManaCost = cost;
		card->mCivilizations = civilizations;
		duel.mCardList.push_back(card);
		duel.getZone(0, zone)->addCard(card);
		card->mZone = zone;
		duel.mNextUniqueId = uid + 1;
		return uid;
	};

	addCard(ZONE_MANA, 1, 1 << CIV_LIGHT);
	addCard(ZONE_MANA, 1, 1 << CIV_LIGHT);
	int expensive = addCard(ZONE_HAND, 7, 1 << CIV_LIGHT);
	int cheapNewCivilization = addCard(ZONE_HAND, 2, 1 << CIV_NATURE);
	int thirdHandCard = addCard(ZONE_HAND, 5, 1 << CIV_FIRE);
	int creature = addCard(ZONE_BATTLE, 1, 1 << CIV_NATURE);
	duel.mCardList[creature]->mPower = 7000;
	duel.mCardList[creature]->mBreaker = 2;

	Message expensiveCharge("cardmana");
	expensiveCharge.addValue("card", expensive);
	Message cheapCharge("cardmana");
	cheapCharge.addValue("card", cheapNewCivilization);
	std::vector<Message> moves;
	moves.push_back(cheapCharge);
	moves.push_back(expensiveCharge);
	HeuristicBot bot(0);
	Message placement;
	bool found = bot.chooseManaPlacement(duel, moves, placement);
	double expensiveDelta = AiScoring::manaPlacementDelta(duel, 0, expensive);
	double cheapDelta = AiScoring::manaPlacementDelta(duel, 0, cheapNewCivilization);
	double battleValue = AiScoring::battleCreatureValue(duel, creature);
	double manaValue = AiScoring::manaZoneValue(duel, 0);
	double unaffordableValue = AiScoring::handCardValue(*duel.mCardList[expensive], 0);
	double affordableValue = AiScoring::handCardValue(*duel.mCardList[cheapNewCivilization], 2);
	bool shieldValues = std::abs(AiScoring::shieldZoneValue(0)) < 0.0001;
	for (int shields = 1; shields <= 5; ++shields)
	{
		shieldValues = shieldValues &&
			std::abs(AiScoring::shieldZoneValue(shields) -
				aiParam("evaluation.shield_count_" + std::to_string(shields) + "_value")) < 0.0001;
	}
	shieldValues = shieldValues &&
		std::abs(AiScoring::shieldZoneValue(7) -
			(aiParam("evaluation.shield_count_5_value") +
				2.0 * aiParam("evaluation.shield_above_5_value"))) < 0.0001;
	bool valid = found && messageInt(placement, "card") == expensive &&
		expensiveDelta > cheapDelta && std::abs(battleValue - 11.0) < 0.0001 &&
		std::abs(manaValue - 4.1) < 0.0001 &&
		std::abs(unaffordableValue - 1.0) < 0.0001 &&
		std::abs(affordableValue - 1.5) < 0.0001 && shieldValues;

	for (int mana = 0; mana < 5; ++mana)
		addCard(ZONE_MANA, 1, 1 << CIV_LIGHT);
	Card* removedFromHand = duel.mCardList[thirdHandCard];
	duel.mHands[0].removeCard(removedFromHand);
	duel.mGraveyards[0].addCard(removedFromHand);
	removedFromHand->mZone = ZONE_GRAVEYARD;
	Message boundaryPlacement;
	bool placesAtMaximumCost = bot.chooseManaPlacement(duel, moves, boundaryPlacement);

	addCard(ZONE_MANA, 1, 1 << CIV_LIGHT);
	Message scarcePlacement;
	bool placesAboveMaximumWithScarceHand =
		bot.chooseManaPlacement(duel, moves, scarcePlacement);

	duel.mGraveyards[0].removeCard(removedFromHand);
	duel.mHands[0].addCard(removedFromHand);
	removedFromHand->mZone = ZONE_HAND;
	Message fullHandPlacement;
	bool placesAboveMaximumWithThreeCards =
		bot.chooseManaPlacement(duel, moves, fullHandPlacement);
	return valid && placesAtMaximumCost && !placesAboveMaximumWithScarceHand &&
		placesAboveMaximumWithThreeCards;
}

bool Application::exerciseKnockoutScoringSmoke()
{
	int attackerCardId = getCardIdFromName("Deadly Fighter Braid Claw");
	int unblockableCardId = getCardIdFromName("Candy Drop");
	int blockerCardId = getCardIdFromName("Bloody Squito");
	int repeatBlockerCardId = getCardIdFromName("Spiral Grass");
	int terrorPitCardId = getCardIdFromName("Terror Pit");
	if (attackerCardId < 0 || unblockableCardId < 0 || blockerCardId < 0 ||
		repeatBlockerCardId < 0 || terrorPitCardId < 0) return false;

	auto addCard = [](Duel& duel, int cardId, int owner, int zone) -> int
	{
		int uid = static_cast<int>(duel.mCardList.size());
		Card* card = new Card(uid, cardId, owner);
		duel.mCardList.push_back(card);
		duel.getZone(owner, zone)->addCard(card);
		card->mZone = zone;
		card->mSummoningSickness = 0;
		duel.mNextUniqueId = uid + 1;
		return uid;
	};
	auto prepare = [](Duel& duel)
	{
		duel.mIsSimulation = true;
		duel.mInputLoopRunning = false;
		duel.mTurn = 0;
		duel.mTurnPhase = TURN_PHASE_MAIN;
		duel.mAttackphase = PHASE_NONE;
	};

	Duel directAttack;
	prepare(directAttack);
	int regular = addCard(directAttack, attackerCardId, 0, ZONE_BATTLE);
	addCard(directAttack, blockerCardId, 1, ZONE_BATTLE);
	bool fullyBlocked = !AiScoring::hasKnockout(directAttack, 0);
	int unblockable = addCard(directAttack, unblockableCardId, 0, ZONE_BATTLE);
	bool unblockableGetsThrough = AiScoring::hasKnockout(directAttack, 0);
	double knockoutValue = AiScoring::playerValue(directAttack, 0);
	directAttack.mCardList[unblockable]->mIsTapped = true;
	double ordinaryValue = AiScoring::playerValue(directAttack, 0);
	bool bonusApplied = std::abs((knockoutValue - ordinaryValue) -
		aiParam("evaluation.knockout_bonus")) < 0.0001;
	directAttack.mCardList[regular]->mIsTapped = true;
	bool noAttackers = !AiScoring::hasKnockout(directAttack, 0);

	Duel shieldRace;
	prepare(shieldRace);
	addCard(shieldRace, attackerCardId, 1, ZONE_SHIELD);
	addCard(shieldRace, attackerCardId, 0, ZONE_BATTLE);
	bool oneAttackerCannotFinish = !AiScoring::hasKnockout(shieldRace, 0);
	addCard(shieldRace, attackerCardId, 0, ZONE_BATTLE);
	bool twoAttackerKo = AiScoring::hasKnockout(shieldRace, 0);
	addCard(shieldRace, blockerCardId, 1, ZONE_BATTLE);
	bool optimalBlockStopsKo = !AiScoring::hasKnockout(shieldRace, 0);
	addCard(shieldRace, attackerCardId, 0, ZONE_BATTLE);
	bool extraAttackerRestoresKo = AiScoring::hasKnockout(shieldRace, 0);

	Duel doubleBreaker;
	prepare(doubleBreaker);
	addCard(doubleBreaker, attackerCardId, 1, ZONE_SHIELD);
	addCard(doubleBreaker, attackerCardId, 1, ZONE_SHIELD);
	int breaker = addCard(doubleBreaker, attackerCardId, 0, ZONE_BATTLE);
	doubleBreaker.mCardList[breaker]->mBreaker = 2;
	addCard(doubleBreaker, attackerCardId, 0, ZONE_BATTLE);
	bool breakerPlusFinisherKo = AiScoring::hasKnockout(doubleBreaker, 0);

	Duel repeatBlock;
	prepare(repeatBlock);
	addCard(repeatBlock, attackerCardId, 0, ZONE_BATTLE);
	addCard(repeatBlock, attackerCardId, 0, ZONE_BATTLE);
	addCard(repeatBlock, repeatBlockerCardId, 1, ZONE_BATTLE);
	bool repeatedBlocksStopKo = !AiScoring::hasKnockout(repeatBlock, 0);

	Duel shieldTriggerTargeting;
	prepare(shieldTriggerTargeting);
	int highValueTapped = addCard(shieldTriggerTargeting, attackerCardId, 0, ZONE_BATTLE);
	shieldTriggerTargeting.mCardList[highValueTapped]->mPower = 12000;
	shieldTriggerTargeting.mCardList[highValueTapped]->mBreaker = 3;
	shieldTriggerTargeting.mCardList[highValueTapped]->mIsTapped = true;
	int doubleBreakerTarget = addCard(shieldTriggerTargeting, attackerCardId, 0, ZONE_BATTLE);
	shieldTriggerTargeting.mCardList[doubleBreakerTarget]->mBreaker = 2;
	int summoningSickTarget = addCard(shieldTriggerTargeting, attackerCardId, 0, ZONE_BATTLE);
	shieldTriggerTargeting.mCardList[summoningSickTarget]->mBreaker = 4;
	shieldTriggerTargeting.mCardList[summoningSickTarget]->mSummoningSickness = 1;
	addCard(shieldTriggerTargeting, attackerCardId, 0, ZONE_BATTLE);
	addCard(shieldTriggerTargeting, attackerCardId, 1, ZONE_SHIELD);
	int terrorPit = addCard(shieldTriggerTargeting, terrorPitCardId, 1, ZONE_HAND);
	shieldTriggerTargeting.mAttackphase = PHASE_TRIGGER;
	shieldTriggerTargeting.mCastingCard = terrorPit;
	bool stableProbeSuppressed = !AiScoring::hasKnockout(shieldTriggerTargeting, 0);
	bool transientProbeFindsKo = AiScoring::hasKnockout(shieldTriggerTargeting, 0, false);
	int knockoutPreferred = RETURN_NOTHING;
	{
		ActiveDuelGuard activeGuard(shieldTriggerTargeting);
		knockoutPreferred = shieldTriggerTargeting.getCardAiPreferredChoice(terrorPit);
	}
	addCard(shieldTriggerTargeting, attackerCardId, 1, ZONE_SHIELD);
	addCard(shieldTriggerTargeting, attackerCardId, 1, ZONE_SHIELD);
	int ordinaryPreferred = RETURN_NOTHING;
	{
		ActiveDuelGuard activeGuard(shieldTriggerTargeting);
		ordinaryPreferred = shieldTriggerTargeting.getCardAiPreferredChoice(terrorPit);
	}

	return fullyBlocked && unblockableGetsThrough && bonusApplied && noAttackers &&
		oneAttackerCannotFinish && twoAttackerKo && optimalBlockStopsKo &&
		extraAttackerRestoresKo && breakerPlusFinisherKo && repeatedBlocksStopKo &&
		stableProbeSuppressed && transientProbeFindsKo &&
		knockoutPreferred == doubleBreakerTarget && ordinaryPreferred == highValueTapped;
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
	for (size_t mapIndex = 0; mapIndex < mWorld.maps.size(); ++mapIndex)
	{
		std::set<std::pair<int, int> > largeTreeAnchors;
		for (std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator tile =
			mWorld.maps[mapIndex].tileLayers.begin();
			tile != mWorld.maps[mapIndex].tileLayers.end(); ++tile)
		{
			worldDataReady = worldDataReady && tile->second.index ==
				RtpTilesetRenderer::canonicalTileIndex(tile->second.family,
					tile->second.sheet, tile->second.index);
			int treeWidth = 0;
			int treeHeight = 0;
			if (!RtpTilesetRenderer::treeAutotileFootprint(tile->second,
				treeWidth, treeHeight) || treeWidth != 2) continue;
			int y = std::get<0>(tile->first);
			int x = std::get<1>(tile->first);
			for (int horizontalSide = -1; horizontalSide <= 1;
				horizontalSide += 2)
				worldDataReady = worldDataReady && largeTreeAnchors.count(
					std::make_pair(x + horizontalSide, y)) == 0;
			largeTreeAnchors.insert(std::make_pair(x, y));
		}
	}
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
	spriteSheetsReady = spriteSheetsReady && SpriteSheetRenderer::characterSourceRect(
		"Resources/Graphics/Characters/!Chest.png", 0, 0, 1, false, 0,
		384, 256, spriteFrame) && spriteFrame.x == 32 && spriteFrame.y == 0 &&
		spriteFrame.w == 32 && spriteFrame.h == 32;
	spriteSheetsReady = spriteSheetsReady && SpriteSheetRenderer::characterSourceRect(
		"Resources/Graphics/Characters/!Chest.png", 0, 0, -1, false, 0,
		384, 256, spriteFrame) && spriteFrame.x == 32 && spriteFrame.y == 96;
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
	const RtpTilesetFamily tilesetFamilies[] = { RtpTilesetFamily::Dungeon,
		RtpTilesetFamily::Inside, RtpTilesetFamily::Outside,
		RtpTilesetFamily::World };
	for (int family = 0; family < 4 && fullTilesetReady; ++family)
	{
		for (int tile = 0; tile < 16; ++tile)
			fullTilesetReady = fullTilesetReady &&
				RtpTilesetRenderer::inferredLayer(RtpTileReference(
					tilesetFamilies[family], RtpTileSheet::A1, tile)) ==
					(tile >= 1 && tile <= 3 ? RtpRenderLayer::Decoration :
						RtpRenderLayer::Ground);
		for (int tile = 0; tile < 32; ++tile)
			fullTilesetReady = fullTilesetReady &&
				RtpTilesetRenderer::inferredLayer(RtpTileReference(
					tilesetFamilies[family], RtpTileSheet::A2, tile)) ==
					(tile % 8 >= 4 ? RtpRenderLayer::Decoration :
						RtpRenderLayer::Ground);
		RtpTileCollision overlayCollision = family == 3 ? RtpTileCollision::Ignore :
			RtpTileCollision::Blocked;
		fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::collision(
			RtpTileReference(tilesetFamilies[family], RtpTileSheet::A1, 1)) ==
			overlayCollision && RtpTilesetRenderer::collision(RtpTileReference(
				tilesetFamilies[family], RtpTileSheet::A2, 4)) == overlayCollision;
	}
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
	fullTilesetReady = fullTilesetReady &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 94) == 93 &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 123) == 112 &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 139) == 128 &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 165) == 157 &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 177) == 168 &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 180) == 172 &&
		RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily::Outside,
			RtpTileSheet::B, 102) == 102;
	const int treeAutotiles[] = { 93, 112, 128, 157, 168, 172 };
	const char* treeNames[] = { "Tree", "Large Tree", "Large Snowy Tree",
		"Snowy Tree", "Spooky Tree", "Palm Tree" };
	SDL_Rect treeTarget = { 96, 96, 32, 32 };
	for (int tree = 0; tree < 6 && fullTilesetReady; ++tree)
	{
		RtpTileReference reference(RtpTilesetFamily::Outside, RtpTileSheet::B,
			treeAutotiles[tree], RtpRenderLayer::Decoration);
		int treeWidth = 0;
		int treeHeight = 0;
		const char* name = RtpTilesetRenderer::treeAutotileName(reference.family,
			reference.sheet, reference.index);
		fullTilesetReady = RtpTilesetRenderer::isTreeAutotile(reference) &&
			RtpTilesetRenderer::treeAutotileFootprint(reference, treeWidth, treeHeight) &&
			treeWidth == (tree == 1 || tree == 2 ? 2 : 1) && treeHeight == 2 &&
			name != NULL && std::string(name) == treeNames[tree] &&
			rtpTiles.draw(reference, 0, treeTarget) &&
			rtpTiles.draw(reference, RtpTilesetRenderer::West, treeTarget) &&
			rtpTiles.drawTreeLayer(reference, RtpRenderLayer::Decoration, treeTarget) &&
			rtpTiles.drawTreeLayer(reference, RtpRenderLayer::Foreground, treeTarget) &&
			!rtpTiles.drawTreeLayer(reference, RtpRenderLayer::Ground, treeTarget);
	}
	RtpTileReference largeTree(RtpTilesetFamily::Outside, RtpTileSheet::B, 112,
		RtpRenderLayer::Decoration);
	RtpTileReference largeSnowyTree(RtpTilesetFamily::Outside, RtpTileSheet::B, 128,
		RtpRenderLayer::Decoration);
	RtpTileReference smallTree(RtpTilesetFamily::Outside, RtpTileSheet::B, 93,
		RtpRenderLayer::Decoration);
	fullTilesetReady = fullTilesetReady &&
		RtpTilesetRenderer::largeTreeAnchorsConflict(
			largeTree, 10, 10, largeSnowyTree, 11, 10) &&
		!RtpTilesetRenderer::largeTreeAnchorsConflict(
			largeTree, 10, 10, largeSnowyTree, 11, 11) &&
		!RtpTilesetRenderer::largeTreeAnchorsConflict(
			largeTree, 10, 10, largeSnowyTree, 12, 10) &&
		!RtpTilesetRenderer::largeTreeAnchorsConflict(
			largeTree, 10, 10, largeSnowyTree, 10, 11) &&
		!RtpTilesetRenderer::largeTreeAnchorsConflict(
			largeTree, 10, 10, largeSnowyTree, 11, 12) &&
		!RtpTilesetRenderer::largeTreeAnchorsConflict(
			largeTree, 10, 10, smallTree, 11, 10);
	fullTilesetReady = fullTilesetReady && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A1, 0)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A2, 0)) ==
		RtpTileCollision::Walkable && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A2, 3)) ==
		RtpTileCollision::Walkable && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A2, 4)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::A2, 21)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::Inside, RtpTileSheet::A2, 7)) ==
		RtpTileCollision::Blocked && RtpTilesetRenderer::collision(
		RtpTileReference(RtpTilesetFamily::World, RtpTileSheet::A2, 4)) ==
		RtpTileCollision::Ignore && RtpTilesetRenderer::collision(
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
		nativeWorld.objectDefinitions == mWorld.objectDefinitions &&
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
			object.rewardDeckName == "Wayfarer's Cache" && !object.openedText.empty() &&
			object.spriteSheet == "Resources/Graphics/Characters/!Chest.png" &&
			object.spriteIndex == 0;
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
				reward.goldTier != npc.rewards[rewardIndex].goldTier ||
				reward.goldTier < 1 || reward.goldTier > 5 ||
				npcGoldRewardValue(reward.goldTier) <= 0)
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
	int savedBuilderBrushSize = mWorldBuilderBrushSize;
	bool savedBuilderShowGrid = mWorldBuilderShowGrid;
	int savedBuilderSelectedNpc = mWorldBuilderSelectedNpc;
	int savedBuilderSelectedObject = mWorldBuilderSelectedObject;
	bool savedBuilderObjectPalette = mWorldBuilderObjectPalette;
	int savedBuilderSelectedObjectTemplate = mWorldBuilderSelectedObjectTemplate;
	std::vector<WorldObject> savedWorldObjects = mWorldObjects;
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
	SDL_Event toggleGridKey = {};
	toggleGridKey.type = SDL_KEYDOWN;
	toggleGridKey.key.keysym.sym = SDLK_g;
	handleWorldBuilderEvent(toggleGridKey);
	bool gridToggleReady = mWorldBuilderShowGrid != savedBuilderShowGrid;
	SDL_Event toggleGridButton = {};
	toggleGridButton.type = SDL_MOUSEBUTTONDOWN;
	toggleGridButton.button.button = SDL_BUTTON_LEFT;
	toggleGridButton.button.x = 1200;
	toggleGridButton.button.y = 40;
	handleWorldBuilderEvent(toggleGridButton);
	gridToggleReady = gridToggleReady &&
		mWorldBuilderShowGrid == savedBuilderShowGrid;
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
	mCurrentWorldArea = builderArea;
	mWorldBuilderTab = WorldBuilderTab::Objects;
	mWorldBuilderObjectPalette = false;
	mWorldBuilderListScroll = 0;
	int bushTemplate = -1;
	for (size_t index = 0; index < mWorldObjectTemplates.size(); ++index)
		if (mWorldObjectTemplates[index].id == "cuttable_bush")
			bushTemplate = (int)index;
	int objectTestX = -1;
	int objectTestY = -1;
	if (mWorld.start.mapId == currentMapId())
		for (int radius = 1; radius <= 40 && objectTestX < 0; ++radius)
			for (int y = std::max(0, mWorld.start.y - radius);
				y <= std::min((int)currentMap().size() - 1,
				mWorld.start.y + radius) && objectTestX < 0; ++y)
				for (int x = std::max(0, mWorld.start.x - radius);
					x <= std::min((int)currentMap()[y].size() - 1,
					mWorld.start.x + radius); ++x)
					if (worldBuilderCanPlace(x, y, -1, -1))
					{
						objectTestX = x;
						objectTestY = y;
						break;
					}
	clearWorldBuilderUndoHistory();
	bool objectCreationReady = bushTemplate >= 0 && bushTemplate < 13 &&
		objectTestX >= 0;
	SDL_Event addPaletteClick = {};
	addPaletteClick.type = SDL_MOUSEBUTTONDOWN;
	addPaletteClick.button.button = SDL_BUTTON_LEFT;
	addPaletteClick.button.x = 1050;
	addPaletteClick.button.y = 675;
	handleWorldBuilderEvent(addPaletteClick);
	objectCreationReady = objectCreationReady && mWorldBuilderObjectPalette;
	if (bushTemplate >= 0 && bushTemplate < 13)
	{
		SDL_Event templateClick = {};
		templateClick.type = SDL_MOUSEBUTTONDOWN;
		templateClick.button.button = SDL_BUTTON_LEFT;
		templateClick.button.x = 1100;
		templateClick.button.y = 151 + bushTemplate * 39 + 8;
		handleWorldBuilderEvent(templateClick);
		objectCreationReady = objectCreationReady &&
			mWorldBuilderSelectedObjectTemplate == bushTemplate;
	}
	const size_t objectCountBeforeCreation = mWorldObjects.size();
	const bool dirtyBeforeObjectCreation = mWorldBuilderDirty;
	if (objectCreationReady)
		objectCreationReady = addWorldBuilderObject(bushTemplate,
			objectTestX, objectTestY);
	objectCreationReady = objectCreationReady &&
		mWorldObjects.size() == objectCountBeforeCreation + 1 &&
		mWorldObjects.back().kind == WorldObjectKind::CuttableBush &&
		mWorldObjects.back().editorCreated &&
		mWorldObjects.back().templateId == "cuttable_bush";
	SDL_Event placedPaletteClick = addPaletteClick;
	placedPaletteClick.button.x = 1190;
	handleWorldBuilderEvent(placedPaletteClick);
	objectCreationReady = objectCreationReady && !mWorldBuilderObjectPalette;
	deleteWorldBuilderObject();
	objectCreationReady = objectCreationReady &&
		mWorldObjects.size() == objectCountBeforeCreation;
	undoWorldBuilder();
	objectCreationReady = objectCreationReady &&
		mWorldObjects.size() == objectCountBeforeCreation + 1 &&
		mWorldObjects.back().editorCreated;
	undoWorldBuilder();
	objectCreationReady = objectCreationReady &&
		mWorldObjects.size() == objectCountBeforeCreation &&
		mWorldBuilderUndoHistory.empty() &&
		mWorldBuilderDirty == dirtyBeforeObjectCreation;
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
	for (int row = 1; row + 1 < (int)currentMap().size() && catalogTestX < 0; ++row)
		for (int column = 1; column + 3 < (int)currentMap()[row].size(); ++column)
			if (!worldBuilderRequiresWalkable(column, row) &&
				!worldBuilderRequiresWalkable(column + 1, row) &&
				!worldBuilderRequiresWalkable(column + 2, row) &&
				!worldBuilderRequiresWalkable(column + 3, row) &&
				!worldBuilderRequiresWalkable(column + 1, row + 1))
			{
				catalogTestX = column;
				catalogTestY = row;
				break;
			}
	if (catalogTestX < 0) return false;
	std::map<std::tuple<int, int, int>, RtpTileReference>& connectionLayers =
		mWorld.maps[builderArea].tileLayers;
	connectionLayers.erase(std::make_tuple(catalogTestY, catalogTestX,
		(int)RtpRenderLayer::Ground));
	connectionLayers.erase(std::make_tuple(catalogTestY, catalogTestX + 1,
		(int)RtpRenderLayer::Ground));
	connectionLayers.erase(std::make_tuple(catalogTestY, catalogTestX + 1,
		(int)RtpRenderLayer::Decoration));
	RtpTileReference wallTile(RtpTilesetFamily::Outside, RtpTileSheet::A3, 8,
		RtpRenderLayer::Ground);
	connectionLayers.insert(std::make_pair(std::make_tuple(catalogTestY,
		catalogTestX, (int)RtpRenderLayer::Ground), wallTile));
	connectionLayers.insert(std::make_pair(std::make_tuple(catalogTestY,
		catalogTestX + 1, (int)RtpRenderLayer::Decoration),
		RtpTileReference(RtpTilesetFamily::Outside, RtpTileSheet::B, 67,
			RtpRenderLayer::Decoration)));
	bool wallOpeningConnectionsRemoved =
		(worldTileConnections(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Ground) & RtpTilesetRenderer::East) == 0;
	connectionLayers.insert(std::make_pair(std::make_tuple(catalogTestY,
		catalogTestX + 1, (int)RtpRenderLayer::Ground), wallTile));
	wallOpeningConnectionsRemoved = wallOpeningConnectionsRemoved &&
		(worldTileConnections(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Ground) & RtpTilesetRenderer::East) != 0;
	mWorld.maps[builderArea].tileLayers = savedTileLayers;
	int brushTestX = -1;
	int brushTestY = -1;
	for (int row = 2; row + 2 < (int)currentMap().size() && brushTestX < 0; ++row)
		for (int column = 2; column + 2 < (int)currentMap()[row].size(); ++column)
		{
			bool freeArea = true;
			for (int y = row - 1; y <= row + 1 && freeArea; ++y)
				for (int x = column - 1; x <= column + 1; ++x)
					if (worldBuilderRequiresWalkable(x, y)) freeArea = false;
			if (freeArea)
			{
				brushTestX = column;
				brushTestY = row;
				break;
			}
		}
	if (brushTestX < 0) return false;
	clearWorldBuilderUndoHistory();
	mWorldBuilderTab = WorldBuilderTab::Tiles;
	mWorldBuilderTileCategory = 2;
	mWorldBuilderTileSheet = (int)RtpTileSheet::A2;
	mWorldBuilderCatalogTile = 1;
	mWorld.maps[builderArea].tileLayers.erase(std::make_tuple(catalogTestY,
		catalogTestX, (int)RtpRenderLayer::Ground));
	mWorld.maps[builderArea].tileLayers.erase(std::make_tuple(catalogTestY,
		catalogTestX + 1, (int)RtpRenderLayer::Ground));
	bool dirtyBeforeUndoTest = mWorldBuilderDirty;
	beginWorldBuilderUndoAction();
	paintWorldBuilderTile(catalogTestX, catalogTestY);
	paintWorldBuilderTile(catalogTestX + 1, catalogTestY);
	commitWorldBuilderUndoAction();
	bool undoReady = mWorldBuilderUndoHistory.size() == 1 &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Ground) != NULL &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX + 1, catalogTestY,
			RtpRenderLayer::Ground) != NULL;
	SDL_Event undoClick = {};
	undoClick.type = SDL_MOUSEBUTTONDOWN;
	undoClick.button.button = SDL_BUTTON_LEFT;
	undoClick.button.x = 1050;
	undoClick.button.y = 740;
	handleWorldBuilderEvent(undoClick);
	undoReady = undoReady && mWorldBuilderUndoHistory.empty() &&
		mWorldBuilderDirty == dirtyBeforeUndoTest &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Ground) == NULL &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX + 1, catalogTestY,
			RtpRenderLayer::Ground) == NULL;
	paintWorldBuilderTile(catalogTestX, catalogTestY);
	std::string undoNpcMap = mNpcs[0].mapId;
	int undoNpcX = mNpcs[0].x;
	int undoNpcY = mNpcs[0].y;
	mWorldBuilderTab = WorldBuilderTab::Npcs;
	mWorldBuilderSelectedNpc = 0;
	beginWorldBuilderUndoAction();
	placeWorldBuilderSelection(catalogTestX, catalogTestY);
	commitWorldBuilderUndoAction();
	SDL_Event undoKey = {};
	undoKey.type = SDL_KEYDOWN;
	undoKey.key.keysym.sym = SDLK_z;
	undoKey.key.keysym.mod = KMOD_CTRL;
	handleWorldBuilderEvent(undoKey);
	undoReady = undoReady && mWorldBuilderUndoHistory.empty() &&
		mNpcs[0].mapId == undoNpcMap && mNpcs[0].x == undoNpcX &&
		mNpcs[0].y == undoNpcY;
	mWorldBuilderTab = WorldBuilderTab::Tiles;
	mWorldBuilderTileCategory = 2;
	mWorldBuilderTileSheet = (int)RtpTileSheet::A2;
	mWorldBuilderCatalogTile = 1;
	mWorldBuilderBrushSize = 1;
	SDL_Event increaseBrush = {};
	increaseBrush.type = SDL_MOUSEBUTTONDOWN;
	increaseBrush.button.button = SDL_BUTTON_LEFT;
	increaseBrush.button.x = 1230;
	increaseBrush.button.y = 660;
	handleWorldBuilderEvent(increaseBrush);
	SDL_Event increaseBrushKey = {};
	increaseBrushKey.type = SDL_KEYDOWN;
	increaseBrushKey.key.keysym.sym = SDLK_RIGHTBRACKET;
	handleWorldBuilderEvent(increaseBrushKey);
	bool brushAreaReady = mWorldBuilderBrushSize == 3 &&
		worldBuilderBrushResizable();
	applyWorldBuilderBrushStroke(-1, -1, brushTestX, brushTestY, false);
	for (int y = brushTestY - 1; y <= brushTestY + 1; ++y)
		for (int x = brushTestX - 1; x <= brushTestX + 1; ++x)
		{
			const RtpTileReference* brushed = worldTileLayer(
				mWorld.maps[builderArea], x, y, RtpRenderLayer::Ground);
			brushAreaReady = brushAreaReady && brushed != NULL &&
				brushed->family == RtpTilesetFamily::Outside &&
				brushed->sheet == RtpTileSheet::A2 && brushed->index == 1;
		}
	applyWorldBuilderBrushStroke(-1, -1, brushTestX, brushTestY, true);
	for (int y = brushTestY - 1; y <= brushTestY + 1; ++y)
		for (int x = brushTestX - 1; x <= brushTestX + 1; ++x)
			brushAreaReady = brushAreaReady && worldTileLayer(
				mWorld.maps[builderArea], x, y, RtpRenderLayer::Ground) == NULL;
	mWorldBuilderTileCategory = 0;
	mWorldBuilderTileSheet = (int)RtpTileSheet::C;
	mWorldBuilderCatalogTile = 31;
	handleWorldBuilderEvent(increaseBrush);
	handleWorldBuilderEvent(increaseBrushKey);
	brushAreaReady = brushAreaReady && !worldBuilderBrushResizable() &&
		mWorldBuilderBrushSize == 3;
	mWorldBuilderTileCategory = 2;
	mWorldBuilderTileSheet = (int)RtpTileSheet::B;
	mWorldBuilderCatalogTile = 94;
	applyWorldBuilderBrushStroke(-1, -1, brushTestX, brushTestY, false);
	for (int y = brushTestY - 1; y <= brushTestY + 1; ++y)
		for (int x = brushTestX - 1; x <= brushTestX + 1; ++x)
		{
			const RtpTileReference* brushedTree = worldTileLayer(
				mWorld.maps[builderArea], x, y, RtpRenderLayer::Decoration);
			brushAreaReady = brushAreaReady && brushedTree != NULL &&
				brushedTree->index == 93;
		}
	applyWorldBuilderBrushStroke(-1, -1, brushTestX, brushTestY, true);
	for (int y = brushTestY - 1; y <= brushTestY + 1; ++y)
		for (int x = brushTestX - 1; x <= brushTestX + 1; ++x)
			brushAreaReady = brushAreaReady && worldTileLayer(
				mWorld.maps[builderArea], x, y, RtpRenderLayer::Decoration) == NULL;
	mWorldBuilderBrushSize = 1;
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
	mWorldBuilderTileCategory = 2;
	mWorldBuilderTileSheet = (int)RtpTileSheet::B;
	mWorldBuilderCatalogTile = 94;
	applyWorldBuilderBrushStroke(catalogTestX, catalogTestY,
		catalogTestX + 3, catalogTestY, false);
	const RtpTileReference* paintedTree = worldTileLayer(mWorld.maps[builderArea],
		catalogTestX, catalogTestY, RtpRenderLayer::Decoration);
	bool forestStrokeReady = true;
	for (int offset = 0; offset < 4; ++offset)
	{
		const RtpTileReference* strokeTree = worldTileLayer(mWorld.maps[builderArea],
			catalogTestX + offset, catalogTestY, RtpRenderLayer::Decoration);
		forestStrokeReady = forestStrokeReady && strokeTree != NULL &&
			strokeTree->index == 93;
	}
	catalogPaintingReady = catalogPaintingReady && paintedTree != NULL &&
		paintedTree->index == 93 && forestStrokeReady &&
		(worldTileConnections(mWorld.maps[builderArea], catalogTestX, catalogTestY,
			RtpRenderLayer::Decoration) & RtpTilesetRenderer::East) != 0 &&
		(worldTileConnections(mWorld.maps[builderArea], catalogTestX + 3, catalogTestY,
			RtpRenderLayer::Decoration) & RtpTilesetRenderer::West) != 0;
	for (int rowOffset = 0; rowOffset < 2; ++rowOffset)
		for (int offset = 0; offset < 4; ++offset)
			mWorld.maps[builderArea].tileLayers.erase(std::make_tuple(
				catalogTestY + rowOffset, catalogTestX + offset,
				(int)RtpRenderLayer::Decoration));
	mWorldBuilderCatalogTile = 113;
	applyWorldBuilderBrushStroke(catalogTestX, catalogTestY,
		catalogTestX + 3, catalogTestY, false);
	bool largeTreeSpacingReady = true;
	for (int offset = 0; offset < 4; ++offset)
	{
		const RtpTileReference* strokeTree = worldTileLayer(mWorld.maps[builderArea],
			catalogTestX + offset, catalogTestY, RtpRenderLayer::Decoration);
		largeTreeSpacingReady = largeTreeSpacingReady &&
			(offset % 2 == 0 ? strokeTree != NULL && strokeTree->index == 112 :
				strokeTree == NULL);
	}
	mWorldBuilderCatalogTile = 129;
	paintWorldBuilderTile(catalogTestX + 1, catalogTestY);
	largeTreeSpacingReady = largeTreeSpacingReady &&
		worldTileLayer(mWorld.maps[builderArea], catalogTestX + 1, catalogTestY,
			RtpRenderLayer::Decoration) == NULL;
	mWorldBuilderCatalogTile = 113;
	paintWorldBuilderTile(catalogTestX + 1, catalogTestY + 1);
	const RtpTileReference* verticallyPackedTree = worldTileLayer(
		mWorld.maps[builderArea], catalogTestX + 1, catalogTestY + 1,
		RtpRenderLayer::Decoration);
	largeTreeSpacingReady = largeTreeSpacingReady && verticallyPackedTree != NULL &&
		verticallyPackedTree->index == 112;
	catalogPaintingReady = catalogPaintingReady && largeTreeSpacingReady;
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
	mWorldBuilderBrushSize = savedBuilderBrushSize;
	mWorldBuilderShowGrid = savedBuilderShowGrid;
	mWorldBuilderSelectedNpc = savedBuilderSelectedNpc;
	mWorldBuilderSelectedObject = savedBuilderSelectedObject;
	mWorldBuilderObjectPalette = savedBuilderObjectPalette;
	mWorldBuilderSelectedObjectTemplate = savedBuilderSelectedObjectTemplate;
	mWorldObjects = savedWorldObjects;
	mWorld.maps[builderArea].tileLayers = savedTileLayers;
	mWorldBuilderDirty = savedBuilderDirty;
	mMouseX = savedMouseX;
	mMouseY = savedMouseY;
	mWorldBuilderTab = savedBuilderTab;
	mCurrentWorldArea = savedBuilderArea;
	mScreen = savedScreen;
	if (!npcDoubleClickReady || !objectDoubleClickReady || !shardListedAsObject ||
		!objectCreationReady ||
		!npcBuilderViewReady || !tilePaletteReady || !catalogPaintingReady ||
		!brushAreaReady || !wallOpeningConnectionsRemoved || !undoReady ||
		!gridToggleReady ||
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
