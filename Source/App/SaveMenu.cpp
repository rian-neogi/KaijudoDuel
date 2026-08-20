#include "Application.h"

#include "AppSupport.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

using namespace AppSupport;

namespace
{
	const char* PLAYER_DATA_ROOT = "PlayerData";
	const SDL_Rect NEW_GAME_BUTTON = { 440, 354, 400, 64 };
	const SDL_Rect LOAD_GAME_BUTTON = { 440, 436, 400, 64 };
	const SDL_Rect EXIT_BUTTON = { 440, 518, 400, 64 };
	const SDL_Rect LOAD_BACK_BUTTON = { 42, 718, 180, 50 };
	const int LOAD_ROW_Y = 184;
	const int LOAD_ROW_HEIGHT = 62;
	const int LOAD_VISIBLE_ROWS = 7;
	const char* STARTER_DECKS[] = {
		"Darkness.txt", "Fire.txt", "Light.txt", "Nature.txt", "Water.txt"
	};
	const int STARTER_DECK_COUNT = sizeof(STARTER_DECKS) / sizeof(STARTER_DECKS[0]);

	bool pathExists(const std::string& path)
	{
		struct stat info;
		return stat(path.c_str(), &info) == 0;
	}

	bool directoryExists(const std::string& path)
	{
		struct stat info;
		return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
	}

	bool ensureDirectory(const std::string& path)
	{
		return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
	}

	bool copyFileIfMissing(const std::string& source, const std::string& destination)
	{
		if (pathExists(destination)) return true;
		std::ifstream input(source.c_str(), std::ios::binary);
		if (!input.good()) return false;
		std::ofstream output(destination.c_str(), std::ios::binary | std::ios::trunc);
		if (!output.good()) return false;
		output << input.rdbuf();
		return output.good();
	}

	bool isSafeSlotName(const std::string& name)
	{
		return !name.empty() && name != "." && name != ".." &&
			name.find('/') == std::string::npos && name.find('\\') == std::string::npos;
	}

	SDL_Rect loadRowRect(int row)
	{
		return { 270, LOAD_ROW_Y + row * LOAD_ROW_HEIGHT, 740, 50 };
	}
}

std::string Application::playerDataPath(const std::string& leaf) const
{
	std::string directory = mActiveSaveDirectory.empty() ? PLAYER_DATA_ROOT :
		mActiveSaveDirectory;
	return leaf.empty() ? directory : directory + "/" + leaf;
}

std::string Application::playerDeckDirectory() const
{
	return playerDataPath("Decks");
}

void Application::migrateLegacyPlayerData()
{
	ensureDirectory(PLAYER_DATA_ROOT);
	bool hasLegacyFiles = pathExists("PlayerData/collection.txt") ||
		pathExists("PlayerData/progress.txt") || pathExists("PlayerData/profile.txt");
	DIR* oldDecks = opendir("PlayerData/Decks");
	if (!hasLegacyFiles && oldDecks == NULL) return;
	if (oldDecks != NULL) closedir(oldDecks);

	const std::string legacyDirectory = "PlayerData/Legacy Save";
	const std::string legacyDecks = legacyDirectory + "/Decks";
	if (!ensureDirectory(legacyDirectory) || !ensureDirectory(legacyDecks)) return;
	copyFileIfMissing("PlayerData/collection.txt", legacyDirectory + "/collection.txt");
	copyFileIfMissing("PlayerData/progress.txt", legacyDirectory + "/progress.txt");
	copyFileIfMissing("PlayerData/profile.txt", legacyDirectory + "/profile.txt");

	oldDecks = opendir("PlayerData/Decks");
	if (oldDecks != NULL)
	{
		for (dirent* entry = readdir(oldDecks); entry != NULL; entry = readdir(oldDecks))
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..") continue;
			copyFileIfMissing("PlayerData/Decks/" + name, legacyDecks + "/" + name);
		}
		closedir(oldDecks);
	}
	std::ofstream info((legacyDirectory + "/save.info").c_str(), std::ios::trunc);
	if (info.good()) info << "name=Legacy Save\n";
}

void Application::refreshSaveSlots()
{
	mSaveSlots.clear();
	DIR* folder = opendir(PLAYER_DATA_ROOT);
	if (folder == NULL) return;
	for (dirent* entry = readdir(folder); entry != NULL; entry = readdir(folder))
	{
		std::string name = entry->d_name;
		if (!isSafeSlotName(name)) continue;
		std::string path = std::string(PLAYER_DATA_ROOT) + "/" + name;
		if (!directoryExists(path)) continue;
		if (!pathExists(path + "/save.info") && !pathExists(path + "/progress.txt") &&
			!pathExists(path + "/collection.txt")) continue;
		mSaveSlots.push_back(name);
	}
	closedir(folder);
	std::sort(mSaveSlots.begin(), mSaveSlots.end());
	if (mLoadGameSelection >= (int)mSaveSlots.size())
		mLoadGameSelection = std::max(0, (int)mSaveSlots.size() - 1);
}

void Application::resetPlayerDataState()
{
	mPlayerDataLoaded = false;
	mAtmosphere.reset();
	mMoney = 0;
	mCollectedShards.clear();
	mMercerShards.clear();
	mOpenedWorldObjects.clear();
	mClearedWorldObjects.clear();
	mCollectionCounts.clear();
	mPlayerDecks.clear();
	mActiveDeckPath = STARTER_DECK_PATH;
	mActiveDeckIndex = -1;
	mEditingDeckIndex = -1;
	mDeckCollectionPage = 0;
	mDeckListScroll = 0;
	mDeckContentsScroll = 0;
	mStoryStage = 0;
	mStoryClues = 0;
	mStoryScene = StoryScene::None;
	mStoryScenePage = 0;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		mNpcs[i].wins = 0;
		mNpcs[i].setPosition(mNpcs[i].homeX, mNpcs[i].homeY);
	}
	int startArea = worldAreaIndex(mWorld.start.mapId);
	if (startArea >= 0) mCurrentWorldArea = startArea;
	mPlayerX = mWorld.start.x;
	mPlayerY = mWorld.start.y;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	mFacingX = 0;
	mFacingY = 1;
	mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
	mMoveIntentX = mMoveIntentY = 0;
	mOpeningPortal = -1;
	mDialogueNpc = -1;
	mDialogueObject = -1;
	mDialogueAction = DialogueAction::None;
	mNpcMenuNpc = -1;
	mRouteChallengeNpc = -1;
	mSuppressedRouteChallenges.clear();
	mPauseMenuOpen = false;
	mLastWorldRegionId.clear();
	mRegionBannerName.clear();
}

void Application::startNewGame()
{
	refreshSaveSlots();
	int number = 1;
	std::string name;
	do
	{
		name = "Save " + std::to_string(number++);
	} while (directoryExists(std::string(PLAYER_DATA_ROOT) + "/" + name));

	mActiveSaveName = name;
	mActiveSaveDirectory = std::string(PLAYER_DATA_ROOT) + "/" + name;
	if (!ensureDirectory(mActiveSaveDirectory) || !ensureDirectory(playerDeckDirectory()))
	{
		mMainMenuNotice = "Unable to create the new save folder.";
		mActiveSaveName.clear();
		mActiveSaveDirectory.clear();
		return;
	}
	for (int starter = 0; starter < STARTER_DECK_COUNT; ++starter)
	{
		std::string source = std::string("Decks/Starter/") + STARTER_DECKS[starter];
		std::string destination = playerDeckDirectory() + "/" + STARTER_DECKS[starter];
		if (copyFileIfMissing(source, destination)) continue;
		mMainMenuNotice = "Unable to copy the bundled starter decks.";
		mActiveSaveName.clear();
		mActiveSaveDirectory.clear();
		return;
	}
	resetPlayerDataState();
	std::ofstream profile(playerDataPath("profile.txt").c_str(), std::ios::trunc);
	if (profile.good()) profile << "active=" << playerDeckDirectory() << "/Fire.txt\n";
	profile.close();
	std::ofstream info(playerDataPath("save.info").c_str(), std::ios::trunc);
	if (info.good()) info << "name=" << name << "\n";
	info.close();
	ensurePlayerDataLoaded();
	saveCurrentGame();
	mScreen = Screen::Overworld;
	mNotice = "New adventure started in " + name + ".";
	mNoticeUntil = SDL_GetTicks() + 4000;
	if (mWindow != NULL) SDL_SetWindowTitle(mWindow,
		("Kaijudo Duel - " + name).c_str());
}

bool Application::loadSaveSlot(const std::string& name)
{
	if (!isSafeSlotName(name)) return false;
	std::string directory = std::string(PLAYER_DATA_ROOT) + "/" + name;
	if (!directoryExists(directory)) return false;
	mActiveSaveName = name;
	mActiveSaveDirectory = directory;
	resetPlayerDataState();
	ensurePlayerDataLoaded();
	mScreen = Screen::Overworld;
	mNotice = "Loaded " + name + ".";
	mNoticeUntil = SDL_GetTicks() + 3500;
	if (mWindow != NULL) SDL_SetWindowTitle(mWindow,
		("Kaijudo Duel - " + name).c_str());
	return true;
}

bool Application::saveCurrentGame()
{
	if (mActiveSaveDirectory.empty()) return false;
	bool success = true;
	for (size_t i = 0; i < mPlayerDecks.size(); ++i)
		if (mPlayerDecks[i].dirty && !saveDeck((int)i)) success = false;
	if (!ensureDirectory(mActiveSaveDirectory) || !ensureDirectory(playerDeckDirectory()))
		return false;
	std::ofstream profile(playerDataPath("profile.txt").c_str(), std::ios::trunc);
	if (!profile.good()) success = false;
	else profile << "active=" << mActiveDeckPath << "\n";
	std::ofstream info(playerDataPath("save.info").c_str(), std::ios::trunc);
	if (!info.good()) success = false;
	else info << "name=" << mActiveSaveName << "\n";
	return savePlayerProgress() && success;
}

void Application::handleMainMenuEvent(const SDL_Event& event)
{
	auto activate = [this]()
	{
		if (mMainMenuSelection == 0) startNewGame();
		else if (mMainMenuSelection == 1)
		{
			refreshSaveSlots();
			if (mSaveSlots.empty()) mMainMenuNotice = "No saved games are available.";
			else
			{
				mLoadGameSelection = 0;
				mLoadGameScroll = 0;
				mScreen = Screen::LoadGame;
			}
		}
		else mRunning = false;
	};
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_UP || key == SDLK_w) mMainMenuSelection = (mMainMenuSelection + 2) % 3;
		else if (key == SDLK_DOWN || key == SDLK_s) mMainMenuSelection = (mMainMenuSelection + 1) % 3;
		else if (key == SDLK_RETURN || key == SDLK_SPACE) activate();
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEMOTION) return;
	int x, y;
	if (event.type == SDL_MOUSEMOTION) logicalMouse(event.motion.x, event.motion.y, x, y);
	else
	{
		if (event.button.button != SDL_BUTTON_LEFT) return;
		logicalMouse(event.button.x, event.button.y, x, y);
	}
	if (contains(NEW_GAME_BUTTON, x, y)) mMainMenuSelection = 0;
	else if (contains(LOAD_GAME_BUTTON, x, y)) mMainMenuSelection = 1;
	else if (contains(EXIT_BUTTON, x, y)) mMainMenuSelection = 2;
	else return;
	if (event.type == SDL_MOUSEBUTTONDOWN) activate();
}

void Application::handleLoadGameEvent(const SDL_Event& event)
{
	int count = (int)mSaveSlots.size();
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			mScreen = Screen::MainMenu;
			return;
		}
		if (count > 0 && (key == SDLK_UP || key == SDLK_w))
			mLoadGameSelection = (mLoadGameSelection + count - 1) % count;
		else if (count > 0 && (key == SDLK_DOWN || key == SDLK_s))
			mLoadGameSelection = (mLoadGameSelection + 1) % count;
		else if (count > 0 && (key == SDLK_RETURN || key == SDLK_SPACE))
			loadSaveSlot(mSaveSlots[mLoadGameSelection]);
		mLoadGameScroll = std::max(0, std::min(mLoadGameSelection,
			std::max(0, count - LOAD_VISIBLE_ROWS)));
		return;
	}
	if (event.type == SDL_MOUSEWHEEL)
	{
		mLoadGameScroll = std::max(0, std::min(std::max(0, count - LOAD_VISIBLE_ROWS),
			mLoadGameScroll - event.wheel.y));
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(LOAD_BACK_BUTTON, x, y))
	{
		mScreen = Screen::MainMenu;
		return;
	}
	for (int row = 0; row < LOAD_VISIBLE_ROWS; ++row)
	{
		int index = mLoadGameScroll + row;
		if (index >= count || !contains(loadRowRect(row), x, y)) continue;
		mLoadGameSelection = index;
		loadSaveSlot(mSaveSlots[index]);
		return;
	}
}

void Application::renderMainMenu()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 10, 18, 31);
	fillRect({ 0, 0, LOGICAL_WIDTH, 255 }, 18, 38, 61);
	fillRect({ 0, 248, LOGICAL_WIDTH, 7 }, 204, 151, 59);
	drawText("KAIJUDO", 430, 92, color(245, 208, 102), 62, 500);
	drawText("DUEL", 545, 169, color(226, 235, 247), 38, 260);
	drawText("A world of cards, Crests, and resonance", 390, 273,
		color(145, 172, 207), 17, 520);

	auto button = [this](const SDL_Rect& rect, const std::string& label, int selection,
		bool danger)
	{
		bool selected = mMainMenuSelection == selection;
		fillRect(rect, danger ? (selected ? 105 : 72) : (selected ? 50 : 29),
			danger ? (selected ? 43 : 34) : (selected ? 73 : 47),
			danger ? (selected ? 48 : 42) : (selected ? 108 : 74), 250);
		outlineRect(rect, selected ? 236 : 104, selected ? 187 : 130,
			selected ? 87 : 170, 255, selected ? 3 : 2);
		drawText(label, rect.x + 115, rect.y + 18, color(239, 243, 249), 23, 260);
	};
	button(NEW_GAME_BUTTON, "New Game", 0, false);
	button(LOAD_GAME_BUTTON, "Load Game", 1, false);
	button(EXIT_BUTTON, "Exit", 2, true);
	drawText(std::to_string(mSaveSlots.size()) + (mSaveSlots.size() == 1 ?
		" save available" : " saves available"), 548, 608, color(133, 156, 187), 14);
	if (!mMainMenuNotice.empty())
		drawText(mMainMenuNotice, 350, 652, color(238, 167, 105), 15, 580);
	drawText("W/S or arrows + Enter", 523, 737, color(117, 138, 166), 13);
}

void Application::renderLoadGame()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 10, 18, 31);
	drawText("LOAD GAME", 72, 54, color(245, 208, 102), 38);
	drawText("Choose an adventure to continue", 72, 111, color(151, 177, 209), 17);
	fillRect({ 245, 158, 790, 486 }, 17, 28, 44, 248);
	outlineRect({ 245, 158, 790, 486 }, 81, 111, 151, 255, 2);
	for (int row = 0; row < LOAD_VISIBLE_ROWS; ++row)
	{
		int index = mLoadGameScroll + row;
		if (index >= (int)mSaveSlots.size()) break;
		SDL_Rect rect = loadRowRect(row);
		bool selected = index == mLoadGameSelection;
		fillRect(rect, selected ? 48 : 27, selected ? 69 : 39,
			selected ? 101 : 58, 250);
		outlineRect(rect, selected ? 229 : 74, selected ? 181 : 96,
			selected ? 82 : 129, 255, selected ? 3 : 1);
		drawText(mSaveSlots[index], rect.x + 24, rect.y + 13,
			color(236, 240, 247), 20, 500);
		drawText("Continue", rect.x + 610, rect.y + 16,
			selected ? color(244, 207, 112) : color(119, 142, 172), 13, 100);
	}
	if (mSaveSlots.empty())
		drawText("No saved games found.", 495, 370, color(158, 174, 194), 18);
	fillRect(LOAD_BACK_BUTTON, 35, 50, 75, 250);
	outlineRect(LOAD_BACK_BUTTON, 112, 149, 205, 255, 2);
	drawText("Back", LOAD_BACK_BUTTON.x + 58, LOAD_BACK_BUTTON.y + 13,
		color(238, 241, 247), 18);
	drawText("Enter/click: load  •  Esc: back", 477, 688,
		color(126, 149, 180), 14);
}
