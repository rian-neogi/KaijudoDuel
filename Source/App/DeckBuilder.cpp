#include "Application.h"

#include "AppSupport.h"
#include "Game/Card.h"
#include "Game/CardData.h"
#include "Game/Deck.h"
#include "Landmarks.h"
#include "SoundManager.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

using namespace AppSupport;

namespace
{
	constexpr int COLLECTION_PAGE_SIZE = 15;
	constexpr int MINIMUM_DECK_SIZE = 40;
	const char* PLAYER_DATA_DIRECTORY = "PlayerData";
	const SDL_Rect BACK_BUTTON = { 20, 20, 120, 46 };
	const SDL_Rect NEW_DECK_BUTTON = { 154, 20, 150, 46 };
	const SDL_Rect SEARCH_BOX = { 326, 20, 430, 46 };
	const SDL_Rect RENAME_BUTTON = { 775, 20, 140, 46 };
	const SDL_Rect SAVE_BUTTON = { 930, 20, 140, 46 };
	const SDL_Rect ACTIVE_BUTTON = { 1085, 20, 175, 46 };
	const SDL_Rect DECK_LIST_PANEL = { 20, 92, 225, 676 };
	const SDL_Rect COLLECTION_PANEL = { 260, 92, 680, 676 };
	const SDL_Rect DECK_PANEL = { 955, 92, 305, 676 };
	const SDL_Rect PREVIOUS_PAGE = { 285, 720, 145, 36 };
	const SDL_Rect NEXT_PAGE = { 770, 720, 145, 36 };

	std::string lowerText(const std::string& value)
	{
		std::string result = value;
		for (size_t i = 0; i < result.size(); ++i)
			result[i] = (char)std::tolower((unsigned char)result[i]);
		return result;
	}

	std::string fileStem(const std::string& path)
	{
		size_t slash = path.find_last_of("/\\");
		std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
		if (name.size() > 4 && lowerText(name.substr(name.size() - 4)) == ".txt")
			name.resize(name.size() - 4);
		return name;
	}

	bool fileExists(const std::string& path)
	{
		std::ifstream file(path.c_str());
		return file.good();
	}

	bool ensureDirectory(const std::string& path)
	{
		if (mkdir(path.c_str(), 0755) == 0 || errno == EEXIST) return true;
		std::fprintf(stderr, "Could not create player-data directory %s\n", path.c_str());
		return false;
	}

	std::vector<std::string> textFilesIn(const std::string& directory)
	{
		std::vector<std::string> result;
		DIR* folder = opendir(directory.c_str());
		if (folder == NULL) return result;
		for (dirent* entry = readdir(folder); entry != NULL; entry = readdir(folder))
		{
			std::string name = entry->d_name;
			if (name.size() <= 4 || lowerText(name.substr(name.size() - 4)) != ".txt") continue;
			result.push_back(directory + "/" + name);
		}
		closedir(folder);
		std::sort(result.begin(), result.end());
		return result;
	}

	std::string safeDeckName(const std::string& name)
	{
		std::string safe;
		for (size_t i = 0; i < name.size(); ++i)
		{
			unsigned char ch = (unsigned char)name[i];
		if (std::isalnum(ch) || ch == ' ' || ch == '-' || ch == '_' || ch == '\'')
			safe.push_back((char)ch);
		}
		while (!safe.empty() && safe.back() == ' ') safe.pop_back();
		while (!safe.empty() && safe.front() == ' ') safe.erase(safe.begin());
		return safe.empty() ? "New Deck" : safe;
	}

	bool parseDeckLine(const std::string& raw, int& count, std::string& name)
	{
		std::string line = deckLineWithoutComment(raw);
		if (!line.empty() && line.back() == '\r') line.pop_back();
		size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos) return false;
		size_t last = line.find_last_not_of(" \t");
		line = line.substr(first, last - first + 1);
		size_t space = line.find_first_of(" \t");
		if (space == std::string::npos) return false;
		count = std::atoi(line.substr(0, space).c_str());
		first = line.find_first_not_of(" \t", space);
		if (first == std::string::npos) return false;
		name = line.substr(first);
		return count > 0 && !name.empty();
	}
}

void Application::loadSettings()
{
	if (mSettingsLoaded) return;
	mSettingsLoaded = true;
	ensureDirectory(PLAYER_DATA_DIRECTORY);

	std::ifstream settings("PlayerData/settings.txt");
	std::string setting;
	while (std::getline(settings, setting))
	{
		if (setting.find("music_volume=") == 0)
			mMusicVolume = std::max(0, std::min(100,
				std::atoi(setting.substr(13).c_str())));
		else if (setting.find("sound_volume=") == 0)
			mSoundVolume = std::max(0, std::min(100,
				std::atoi(setting.substr(13).c_str())));
		else if (setting.find("auto_choose_only_action=") == 0)
			mAutoChooseOnlyAction = std::atoi(setting.substr(24).c_str()) != 0;
	}
	if (mSoundManager != NULL)
	{
		mSoundManager->setMusicVolume(mMusicVolume);
		mSoundManager->setSoundVolume(mSoundVolume);
	}
}

void Application::ensurePlayerDataLoaded()
{
	if (mPlayerDataLoaded) return;
	mPlayerDataLoaded = true;
	ensureDirectory(playerDataPath());
	ensureDirectory(playerDeckDirectory());
	mCollectionCounts.assign(gCardDatabase.size(), 0);
	loadSettings();

	std::ifstream collection(playerDataPath("collection.txt").c_str());
	bool collectionLoaded = false;
	if (collection.good())
	{
		std::string line;
		while (std::getline(collection, line))
		{
			int count = 0;
			std::string name;
			if (!parseDeckLine(line, count, name)) continue;
			int cardId = getDeckCardIdFromName(name);
			if (cardId >= 0 && cardId < (int)mCollectionCounts.size())
			{
				mCollectionCounts[cardId] = count;
				collectionLoaded = true;
			}
		}
	}

	auto loadDeckFile = [this](const std::string& path)
	{
		PlayerDeck deck;
		deck.name = fileStem(path);
		deck.path = path;
		deck.managed = true;
		std::ifstream file(path.c_str());
		std::string line;
		while (std::getline(file, line))
		{
			int count = 0;
			std::string name;
			if (!parseDeckLine(line, count, name)) continue;
			int cardId = getDeckCardIdFromName(name);
			if (cardId >= 0) deck.cards[cardId] += count;
		}
		mPlayerDecks.push_back(deck);
	};

	std::vector<std::string> playerDecks = textFilesIn(playerDeckDirectory());
	for (size_t i = 0; i < playerDecks.size(); ++i) loadDeckFile(playerDecks[i]);

	mActiveDeckPath = STARTER_DECK_PATH;
	std::ifstream profile(playerDataPath("profile.txt").c_str());
	std::string line;
	while (std::getline(profile, line))
	{
		if (line.find("active=") != 0) continue;
		std::string configuredPath = line.substr(7);
		const std::string legacyDeckPrefix = "PlayerData/Decks/";
		if (configuredPath.find(legacyDeckPrefix) == 0)
			configuredPath = playerDeckDirectory() + "/" +
				configuredPath.substr(legacyDeckPrefix.size());
		for (size_t i = 0; i < mPlayerDecks.size(); ++i)
			if (mPlayerDecks[i].path == configuredPath)
			{
				mActiveDeckPath = configuredPath;
				break;
			}
	}
	mActiveDeckIndex = -1;
	for (size_t i = 0; i < mPlayerDecks.size(); ++i)
		if (mPlayerDecks[i].path == mActiveDeckPath) mActiveDeckIndex = (int)i;
	if (mEditingDeckIndex < 0 && !mPlayerDecks.empty())
		mEditingDeckIndex = mActiveDeckIndex >= 0 ? mActiveDeckIndex : 0;

	if (!collectionLoaded)
	{
		if (!mPlayerDecks.empty())
		{
			for (size_t deck = 0; deck < mPlayerDecks.size(); ++deck)
				for (std::map<int, int>::const_iterator card = mPlayerDecks[deck].cards.begin();
					card != mPlayerDecks[deck].cards.end(); ++card)
					if (card->first >= 0 && card->first < (int)mCollectionCounts.size())
						mCollectionCounts[card->first] = std::max(
							mCollectionCounts[card->first], card->second);
		}
		else
		{
			std::ifstream starter(STARTER_DECK_PATH);
			std::string starterLine;
			while (std::getline(starter, starterLine))
			{
				int count = 0;
				std::string name;
				if (!parseDeckLine(starterLine, count, name)) continue;
				int cardId = getDeckCardIdFromName(name);
				if (cardId >= 0 && cardId < (int)mCollectionCounts.size())
					mCollectionCounts[cardId] += count;
			}
		}
	}

	std::ifstream progress(playerDataPath("progress.txt").c_str());
	std::string savedMap;
	int savedX = 0;
	int savedY = 0;
	int savedFacingX = 0;
	int savedFacingY = 1;
	bool hasSavedX = false;
	bool hasSavedY = false;
	OverworldAtmosphereState atmosphere = mAtmosphere.state();
	const std::string atmosphereDay = "atmosphere.day=";
	const std::string atmosphereMinute = "atmosphere.minute=";
	const std::string atmosphereWeather = "atmosphere.weather=";
	const std::string atmosphereIntensity = "atmosphere.intensity=";
	const std::string atmosphereRemaining = "atmosphere.remaining=";
	const std::string atmosphereFading = "atmosphere.fading=";
	const std::string atmosphereSeed = "atmosphere.seed=";
	while (std::getline(progress, line))
	{
		if (line.find("world.map=") == 0)
		{
			savedMap = line.substr(10);
			continue;
		}
		if (line.find("world.x=") == 0)
		{
			savedX = std::atoi(line.substr(8).c_str());
			hasSavedX = true;
			continue;
		}
		if (line.find("world.y=") == 0)
		{
			savedY = std::atoi(line.substr(8).c_str());
			hasSavedY = true;
			continue;
		}
		if (line.find("world.facing_x=") == 0)
		{
			savedFacingX = std::max(-1, std::min(1,
				std::atoi(line.substr(15).c_str())));
			continue;
		}
		if (line.find("world.facing_y=") == 0)
		{
			savedFacingY = std::max(-1, std::min(1,
				std::atoi(line.substr(15).c_str())));
			continue;
		}
		if (line.find(atmosphereDay) == 0)
		{
			atmosphere.day = std::max(1, std::atoi(line.substr(atmosphereDay.size()).c_str()));
			continue;
		}
		if (line.find(atmosphereMinute) == 0)
		{
			atmosphere.minuteOfDay = std::atoi(line.substr(atmosphereMinute.size()).c_str());
			continue;
		}
		if (line.find(atmosphereWeather) == 0)
		{
			WeatherKind weather;
			if (OverworldAtmosphere::parseWeather(
				line.substr(atmosphereWeather.size()), weather)) atmosphere.weather = weather;
			continue;
		}
		if (line.find(atmosphereIntensity) == 0)
		{
			atmosphere.weatherIntensity = std::atoi(
				line.substr(atmosphereIntensity.size()).c_str()) / 1000.f;
			continue;
		}
		if (line.find(atmosphereRemaining) == 0)
		{
			atmosphere.weatherRemaining = (unsigned int)std::strtoul(
				line.substr(atmosphereRemaining.size()).c_str(), NULL, 10);
			continue;
		}
		if (line.find(atmosphereFading) == 0)
		{
			atmosphere.weatherFadingOut =
				std::atoi(line.substr(atmosphereFading.size()).c_str()) != 0;
			continue;
		}
		if (line.find(atmosphereSeed) == 0)
		{
			atmosphere.weatherSeed = (unsigned int)std::strtoul(
				line.substr(atmosphereSeed.size()).c_str(), NULL, 10);
			continue;
		}
		if (line.find("money=") == 0)
		{
			mMoney = std::max(0, std::atoi(line.substr(6).c_str()));
			continue;
		}
		if (line.find("story.stage=") == 0)
		{
			mStoryStage = std::max(0, std::min(4, std::atoi(line.substr(12).c_str())));
			continue;
		}
		if (line.find("story.clues=") == 0)
		{
			mStoryClues = std::max(0, std::atoi(line.substr(13).c_str()));
			continue;
		}
		const std::string landmarkPrefix = "landmark.";
		if (line.find(landmarkPrefix) == 0)
		{
			size_t equals = line.find('=', landmarkPrefix.size());
			if (equals == std::string::npos || std::atoi(line.substr(equals + 1).c_str()) != 1)
				continue;
			std::string landmarkId = line.substr(landmarkPrefix.size(),
				equals - landmarkPrefix.size());
			if (Landmarks::find(landmarkId) != NULL)
				mDiscoveredLandmarks.insert(landmarkId);
			continue;
		}
		const std::string collectedPrefix = "shard.collected.";
		const std::string mercerPrefix = "shard.mercer.";
		if (line.find(collectedPrefix) == 0 || line.find(mercerPrefix) == 0)
		{
			bool givenToMercer = line.find(mercerPrefix) == 0;
			const std::string& prefix = givenToMercer ? mercerPrefix : collectedPrefix;
			size_t equals = line.find('=', prefix.size());
			if (equals == std::string::npos || std::atoi(line.substr(equals + 1).c_str()) != 1)
				continue;
			std::string shardId = line.substr(prefix.size(), equals - prefix.size());
			for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
			{
				if (mMercerStock.shards[i].id != shardId) continue;
				mCollectedShards.insert(shardId);
				if (givenToMercer) mMercerShards.insert(shardId);
				break;
			}
			continue;
		}
		const std::string openedObjectPrefix = "object.opened.";
		if (line.find(openedObjectPrefix) == 0)
		{
			size_t equals = line.find('=', openedObjectPrefix.size());
			if (equals == std::string::npos ||
				std::atoi(line.substr(equals + 1).c_str()) != 1) continue;
			std::string objectId = line.substr(openedObjectPrefix.size(),
				equals - openedObjectPrefix.size());
			for (size_t i = 0; i < mWorldObjects.size(); ++i)
				if (mWorldObjects[i].id == objectId &&
					(mWorldObjects[i].kind == WorldObjectKind::DeckChest ||
					 mWorldObjects[i].kind == WorldObjectKind::Chest))
					mOpenedWorldObjects.insert(objectId);
			continue;
		}
		const std::string clearedObjectPrefix = "object.cleared.";
		if (line.find(clearedObjectPrefix) == 0)
		{
			size_t equals = line.find('=', clearedObjectPrefix.size());
			if (equals == std::string::npos ||
				std::atoi(line.substr(equals + 1).c_str()) != 1) continue;
			std::string objectId = line.substr(clearedObjectPrefix.size(),
				equals - clearedObjectPrefix.size());
			for (size_t i = 0; i < mWorldObjects.size(); ++i)
				if (mWorldObjects[i].id == objectId &&
					(mWorldObjects[i].kind == WorldObjectKind::CuttableBush ||
					 mWorldObjects[i].kind == WorldObjectKind::SmashableRock))
					mClearedWorldObjects.insert(objectId);
			continue;
		}
		if (line.find("npc.") != 0) continue;
		size_t equals = line.find('=');
		if (equals == std::string::npos) continue;
		std::string npcName = line.substr(4, equals - 4);
		int wins = std::max(0, std::atoi(line.substr(equals + 1).c_str()));
		for (size_t i = 0; i < mNpcs.size(); ++i)
			if (mNpcs[i].isDuelist() &&
				(mNpcs[i].id == npcName || mNpcs[i].name == npcName))
				mNpcs[i].wins = std::min(mNpcs[i].maxWins, wins);
	}
	mAtmosphere.restore(atmosphere);
	int savedArea = worldAreaIndex(savedMap);
	if (savedArea >= 0 && hasSavedX && hasSavedY)
	{
		const std::vector<std::string>& tiles = mWorld.maps[savedArea].tiles;
		bool validPosition = savedY >= 0 && savedY < (int)tiles.size() && savedX >= 0 &&
			savedX < (int)tiles[savedY].size() &&
			worldTileWalkable(mWorld.maps[savedArea], savedX, savedY);
		if (validPosition)
		{
			mCurrentWorldArea = savedArea;
			mPlayerX = savedX;
			mPlayerY = savedY;
			mVisualX = (float)savedX;
			mVisualY = (float)savedY;
			if (std::abs(savedFacingX) + std::abs(savedFacingY) == 1)
			{
				mFacingX = savedFacingX;
				mFacingY = savedFacingY;
			}
		}
	}
	initializeStory();
}

bool Application::savePlayerProgress()
{
	if (!ensureDirectory(playerDataPath())) return false;
	std::ofstream collection(playerDataPath("collection.txt").c_str(), std::ios::trunc);
	if (!collection.good()) return false;
	for (size_t i = 0; i < mCollectionCounts.size() && i < gCardDatabase.size(); ++i)
		if (mCollectionCounts[i] > 0)
			collection << mCollectionCounts[i] << " " << gCardDatabase[i].Name << "\n";

	std::ofstream progress(playerDataPath("progress.txt").c_str(), std::ios::trunc);
	if (!progress.good()) return false;
	progress << "money=" << std::max(0, mMoney) << "\n";
	progress << "story.stage=" << mStoryStage << "\n";
	progress << "story.clues=" << mStoryClues << "\n";
	progress << "world.map=" << currentMapId() << "\n";
	progress << "world.x=" << mPlayerX << "\n";
	progress << "world.y=" << mPlayerY << "\n";
	progress << "world.facing_x=" << mFacingX << "\n";
	progress << "world.facing_y=" << mFacingY << "\n";
	const OverworldAtmosphereState atmosphere = mAtmosphere.state();
	const char* weather = atmosphere.weather == WeatherKind::Rain ? "rain" :
		(atmosphere.weather == WeatherKind::Snow ? "snow" : "clear");
	progress << "atmosphere.day=" << atmosphere.day << "\n";
	progress << "atmosphere.minute=" << atmosphere.minuteOfDay << "\n";
	progress << "atmosphere.weather=" << weather << "\n";
	progress << "atmosphere.intensity=" <<
		(int)std::round(atmosphere.weatherIntensity * 1000.f) << "\n";
	progress << "atmosphere.remaining=" << atmosphere.weatherRemaining << "\n";
	progress << "atmosphere.fading=" << (atmosphere.weatherFadingOut ? 1 : 0) << "\n";
	progress << "atmosphere.seed=" << atmosphere.weatherSeed << "\n";
	for (size_t i = 0; i < Landmarks::COUNT; ++i)
		if (mDiscoveredLandmarks.count(Landmarks::DEFINITIONS[i].id))
			progress << "landmark." << Landmarks::DEFINITIONS[i].id << "=1\n";
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const std::string& shardId = mMercerStock.shards[i].id;
		if (mCollectedShards.count(shardId)) progress << "shard.collected." << shardId << "=1\n";
		if (mMercerShards.count(shardId)) progress << "shard.mercer." << shardId << "=1\n";
	}
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
	{
		if (mOpenedWorldObjects.count(mWorldObjects[i].id))
			progress << "object.opened." << mWorldObjects[i].id << "=1\n";
		if (mClearedWorldObjects.count(mWorldObjects[i].id))
			progress << "object.cleared." << mWorldObjects[i].id << "=1\n";
	}
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].isDuelist()) progress << "npc." << mNpcs[i].id << "=" << mNpcs[i].wins << "\n";
	return collection.good() && progress.good();
}

void Application::saveSettings()
{
	ensureDirectory(PLAYER_DATA_DIRECTORY);
	std::ofstream settings("PlayerData/settings.txt", std::ios::trunc);
	settings << "music_volume=" << mMusicVolume << "\n";
	settings << "sound_volume=" << mSoundVolume << "\n";
	settings << "auto_choose_only_action=" << (mAutoChooseOnlyAction ? 1 : 0) << "\n";
}

void Application::awardNpcVictory(int npcIndex)
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return;
	ensurePlayerDataLoaded();
	Npc& npc = mNpcs[npcIndex];
	if (!npc.canBattle()) return;

	NpcReward reward = npc.nextReward();
	const int goldReward = npcGoldRewardValue(reward.goldTier);
	++npc.wins;
	mMoney += goldReward;
	int rewardId = getCardIdFromName(reward.card);
	bool awardedCard = rewardId >= 0 && rewardId < (int)mCollectionCounts.size();
	if (awardedCard)
	{
		++mCollectionCounts[rewardId];
		mPendingRewardCardId = rewardId;
		mPendingRewardGold = goldReward;
	}
	updateStoryProgress();
	savePlayerProgress();

	mNotice = "Victory over " + npc.name + "! ";
	if (awardedCard) mNotice += "+1 " + reward.card + " and ";
	mNotice += "+" + std::to_string(goldReward) + " gold.";
	mNoticeUntil = SDL_GetTicks() + 6500;
}

bool Application::awardDeckReward(const std::string& sourcePath,
	const std::string& requestedName, std::string& error)
{
	error.clear();
	ensurePlayerDataLoaded();
	std::string resolvedPath;
	if (!resolveDeckPath(sourcePath, resolvedPath))
	{
		error = "The abandoned deck file is missing.";
		return false;
	}

	PlayerDeck reward;
	reward.name = safeDeckName(requestedName);
	std::ifstream source(resolvedPath.c_str());
	std::string line;
	int total = 0;
	while (std::getline(source, line))
	{
		int count = 0;
		std::string cardName;
		if (!parseDeckLine(line, count, cardName)) continue;
		int cardId = getDeckCardIdFromName(cardName);
		if (cardId < 0 || cardId >= (int)mCollectionCounts.size())
		{
			error = "The abandoned deck contains an unknown card: " + cardName;
			return false;
		}
		reward.cards[cardId] += count;
		total += count;
	}
	if (total < MINIMUM_DECK_SIZE)
	{
		error = "The abandoned deck does not contain at least 40 cards.";
		return false;
	}

	std::string baseName = reward.name;
	for (int suffix = 2;; ++suffix)
	{
		bool duplicate = false;
		for (size_t i = 0; i < mPlayerDecks.size(); ++i)
			if (mPlayerDecks[i].name == reward.name) duplicate = true;
		if (!duplicate) break;
		reward.name = baseName + " " + std::to_string(suffix);
	}
	for (std::map<int, int>::const_iterator card = reward.cards.begin();
		card != reward.cards.end(); ++card)
		mCollectionCounts[card->first] += card->second;
	reward.dirty = true;
	mPlayerDecks.push_back(reward);
	int rewardIndex = (int)mPlayerDecks.size() - 1;
	if (saveDeck(rewardIndex)) return true;

	for (std::map<int, int>::const_iterator card = reward.cards.begin();
		card != reward.cards.end(); ++card)
		mCollectionCounts[card->first] -= card->second;
	mPlayerDecks.pop_back();
	error = "The abandoned deck could not be saved.";
	return false;
}

int Application::deckCardCount(const PlayerDeck& deck) const
{
	int count = 0;
	for (std::map<int, int>::const_iterator card = deck.cards.begin(); card != deck.cards.end(); ++card)
		count += card->second;
	return count;
}

bool Application::deckHasMinimumCards(const PlayerDeck& deck) const
{
	return deckCardCount(deck) >= MINIMUM_DECK_SIZE;
}

std::vector<int> Application::filteredCollection() const
{
	std::vector<int> cards;
	std::string search = lowerText(mDeckSearch);
	for (size_t i = 0; i < gCardDatabase.size(); ++i)
	{
		if (i >= mCollectionCounts.size() || mCollectionCounts[i] <= 0) continue;
		if (!search.empty() && lowerText(gCardDatabase[i].Name).find(search) == std::string::npos) continue;
		cards.push_back((int)i);
	}
	return cards;
}

std::string Application::availableDeckPath(const std::string& name, const std::string& currentPath) const
{
	std::string base = playerDeckDirectory() + "/" + safeDeckName(name);
	std::string candidate = base + ".txt";
	if (candidate == currentPath || !fileExists(candidate)) return candidate;
	for (int suffix = 2; suffix < 1000; ++suffix)
	{
		candidate = base + " " + std::to_string(suffix) + ".txt";
		if (candidate == currentPath || !fileExists(candidate)) return candidate;
	}
	return base + " copy.txt";
}

bool Application::saveDeck(int deckIndex)
{
	if (deckIndex < 0 || deckIndex >= (int)mPlayerDecks.size()) return false;
	ensureDirectory(playerDataPath());
	ensureDirectory(playerDeckDirectory());
	PlayerDeck& deck = mPlayerDecks[deckIndex];
	deck.name = safeDeckName(deck.name);
	std::string newPath = availableDeckPath(deck.name, deck.managed ? deck.path : "");
	if (deck.managed && deck.path != newPath && !deck.path.empty())
		std::rename(deck.path.c_str(), newPath.c_str());

	std::ofstream output(newPath.c_str(), std::ios::trunc);
	if (!output.good())
	{
		showDeckNotice("Could not save deck.");
		return false;
	}
	for (std::map<int, int>::const_iterator card = deck.cards.begin(); card != deck.cards.end(); ++card)
		if (card->first >= 0 && card->first < (int)gCardDatabase.size() && card->second > 0)
			output << card->second << " " << gCardDatabase[card->first].Name << "\n";
	output.close();

	bool wasActive = deckIndex == mActiveDeckIndex;
	bool revertedToStarter = false;
	deck.path = newPath;
	deck.managed = true;
	deck.dirty = false;
	if (wasActive)
	{
		if (deckHasMinimumCards(deck))
		{
			mActiveDeckPath = deck.path;
			std::ofstream profile(playerDataPath("profile.txt").c_str(), std::ios::trunc);
			profile << "active=" << mActiveDeckPath << "\n";
		}
		else
		{
			revertedToStarter = true;
			mActiveDeckIndex = -1;
			for (size_t i = 0; i < mPlayerDecks.size(); ++i)
				if ((int)i != deckIndex && deckHasMinimumCards(mPlayerDecks[i]) &&
					(mActiveDeckIndex < 0 || mPlayerDecks[i].name == "Fire"))
					mActiveDeckIndex = (int)i;
			mActiveDeckPath = mActiveDeckIndex >= 0 ?
				mPlayerDecks[mActiveDeckIndex].path : STARTER_DECK_PATH;
			std::ofstream profile(playerDataPath("profile.txt").c_str(), std::ios::trunc);
			profile << "active=" << mActiveDeckPath << "\n";
			showDeckNotice("Deck saved; the starter deck is active until this deck has at least 40 cards.");
		}
	}

	savePlayerProgress();
	if (!revertedToStarter) showDeckNotice("Deck saved.");
	return true;
}

void Application::setActiveDeck(int deckIndex)
{
	if (deckIndex < 0 || deckIndex >= (int)mPlayerDecks.size()) return;
	if (!deckHasMinimumCards(mPlayerDecks[deckIndex]))
	{
		showDeckNotice("An active deck must contain at least 40 cards.");
		return;
	}
	if (mPlayerDecks[deckIndex].dirty || !mPlayerDecks[deckIndex].managed)
		if (!saveDeck(deckIndex)) return;
	mActiveDeckIndex = deckIndex;
	mActiveDeckPath = mPlayerDecks[deckIndex].path;
	ensureDirectory(playerDataPath());
	std::ofstream profile(playerDataPath("profile.txt").c_str(), std::ios::trunc);
	profile << "active=" << mActiveDeckPath << "\n";
	showDeckNotice("Active deck updated.");
}

void Application::createDeck()
{
	PlayerDeck deck;
	int number = 1;
	do
	{
		deck.name = "New Deck " + std::to_string(number++);
		bool used = false;
		for (size_t i = 0; i < mPlayerDecks.size(); ++i)
			if (mPlayerDecks[i].name == deck.name) used = true;
		if (!used) break;
	} while (number < 1000);
	deck.dirty = true;
	mPlayerDecks.insert(mPlayerDecks.begin(), deck);
	if (mActiveDeckIndex >= 0) ++mActiveDeckIndex;
	mEditingDeckIndex = 0;
	mDeckListScroll = 0;
	mDeckContentsScroll = 0;
	showDeckNotice("New deck created. Add cards, rename it, then save.");
}

void Application::showDeckNotice(const std::string& notice)
{
	mDeckNotice = notice;
	mDeckNoticeUntil = SDL_GetTicks() + 3500;
}

void Application::enterDeckBuilder()
{
	ensurePlayerDataLoaded();
	mPauseMenuOpen = false;
	mScreen = Screen::DeckBuilder;
	mDeckSearchFocused = false;
	mDeckRenameFocused = false;
	mDeckCollectionPage = 0;
	mDeckHoveredCard = -1;
	mDeckCardHitboxes.clear();
	if (mPlayerDecks.empty()) createDeck();
}

void Application::leaveDeckBuilder()
{
	for (size_t i = 0; i < mPlayerDecks.size(); ++i)
		if (mPlayerDecks[i].dirty) saveDeck((int)i);
	SDL_StopTextInput();
	mDeckSearchFocused = false;
	mDeckRenameFocused = false;
	mDeckHoveredCard = -1;
	mDeckCardHitboxes.clear();
	mScreen = Screen::Overworld;
}

void Application::handleDeckBuilderEvent(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
		return;
	}
	if (event.type == SDL_TEXTINPUT && (mDeckSearchFocused || mDeckRenameFocused))
	{
		std::string& text = mDeckRenameFocused ? mDeckNameInput : mDeckSearch;
		if (text.size() < 48) text += event.text.text;
		if (mDeckSearchFocused) mDeckCollectionPage = 0;
		return;
	}
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if ((mDeckSearchFocused || mDeckRenameFocused) && key == SDLK_BACKSPACE)
		{
			std::string& text = mDeckRenameFocused ? mDeckNameInput : mDeckSearch;
			if (!text.empty()) text.pop_back();
			if (mDeckSearchFocused) mDeckCollectionPage = 0;
			return;
		}
		if (mDeckRenameFocused && key == SDLK_RETURN)
		{
			if (mEditingDeckIndex >= 0 && !mDeckNameInput.empty())
			{
				mPlayerDecks[mEditingDeckIndex].name = safeDeckName(mDeckNameInput);
				mPlayerDecks[mEditingDeckIndex].dirty = true;
			}
			mDeckRenameFocused = false;
			SDL_StopTextInput();
			return;
		}
		if (key == SDLK_ESCAPE)
		{
			if (mDeckSearchFocused || mDeckRenameFocused)
			{
				mDeckSearchFocused = false;
				mDeckRenameFocused = false;
				SDL_StopTextInput();
			}
			else leaveDeckBuilder();
			return;
		}
	}
	if (event.type == SDL_MOUSEWHEEL)
	{
		int mouseX = mMouseX;
		int mouseY = mMouseY;
		int previousPage = mDeckCollectionPage;
		int previousListScroll = mDeckListScroll;
		int previousContentsScroll = mDeckContentsScroll;
		int wheel = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ?
			-event.wheel.y : event.wheel.y;
		if (contains(COLLECTION_PANEL, mouseX, mouseY))
		{
			int pages = std::max(1, ((int)filteredCollection().size() + COLLECTION_PAGE_SIZE - 1) / COLLECTION_PAGE_SIZE);
			mDeckCollectionPage = std::max(0, std::min(pages - 1, mDeckCollectionPage - wheel));
		}
		else if (contains(DECK_LIST_PANEL, mouseX, mouseY))
		{
			int maximum = std::max(0, (int)mPlayerDecks.size() - 12);
			mDeckListScroll = std::max(0, std::min(maximum, mDeckListScroll - wheel));
		}
		else if (contains(DECK_PANEL, mouseX, mouseY))
		{
			int cards = mEditingDeckIndex >= 0 && mEditingDeckIndex < (int)mPlayerDecks.size() ?
				(int)mPlayerDecks[mEditingDeckIndex].cards.size() : 0;
			mDeckContentsScroll = std::max(0,
				std::min(std::max(0, cards - 15), mDeckContentsScroll - wheel));
		}
		if ((previousPage != mDeckCollectionPage || previousListScroll != mDeckListScroll ||
			previousContentsScroll != mDeckContentsScroll) && mSoundManager != NULL)
			mSoundManager->playSound(SOUND_UI_SCROLL);
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(BACK_BUTTON, x, y)) { leaveDeckBuilder(); return; }
	if (contains(NEW_DECK_BUTTON, x, y)) { createDeck(); return; }
	if (contains(SEARCH_BOX, x, y))
	{
		mDeckSearchFocused = true;
		mDeckRenameFocused = false;
		SDL_StartTextInput();
		return;
	}
	if (contains(RENAME_BUTTON, x, y) && mEditingDeckIndex >= 0)
	{
		mDeckNameInput = mPlayerDecks[mEditingDeckIndex].name;
		mDeckRenameFocused = true;
		mDeckSearchFocused = false;
		SDL_StartTextInput();
		return;
	}
	if (contains(SAVE_BUTTON, x, y)) { saveDeck(mEditingDeckIndex); return; }
	if (contains(ACTIVE_BUTTON, x, y)) { setActiveDeck(mEditingDeckIndex); return; }
	if (contains(PREVIOUS_PAGE, x, y))
	{
		int previousPage = mDeckCollectionPage;
		mDeckCollectionPage = std::max(0, mDeckCollectionPage - 1);
		if (previousPage != mDeckCollectionPage && mSoundManager != NULL)
			mSoundManager->playSound(SOUND_UI_SCROLL);
		return;
	}
	if (contains(NEXT_PAGE, x, y))
	{
		int pages = std::max(1, ((int)filteredCollection().size() + COLLECTION_PAGE_SIZE - 1) / COLLECTION_PAGE_SIZE);
		int previousPage = mDeckCollectionPage;
		mDeckCollectionPage = std::min(pages - 1, mDeckCollectionPage + 1);
		if (previousPage != mDeckCollectionPage && mSoundManager != NULL)
			mSoundManager->playSound(SOUND_UI_SCROLL);
		return;
	}

	if (contains(DECK_LIST_PANEL, x, y))
	{
		if (y < 132) return;
		int row = (y - 132) / 48;
		int index = mDeckListScroll + row;
		if (row >= 0 && index >= 0 && index < (int)mPlayerDecks.size())
		{
			mEditingDeckIndex = index;
			mDeckContentsScroll = 0;
		}
		return;
	}
	if (mEditingDeckIndex < 0 || mEditingDeckIndex >= (int)mPlayerDecks.size()) return;
	PlayerDeck& deck = mPlayerDecks[mEditingDeckIndex];
	if (contains(COLLECTION_PANEL, x, y) && y >= 150 && y < 690)
	{
		std::vector<int> cards = filteredCollection();
		for (int slot = 0; slot < COLLECTION_PAGE_SIZE; ++slot)
		{
			int column = slot % 5;
			int row = slot / 5;
			SDL_Rect cardRect = { 276 + column * 130, 150 + row * 180, 108, 150 };
			if (!contains(cardRect, x, y)) continue;
			int position = mDeckCollectionPage * COLLECTION_PAGE_SIZE + slot;
			if (position < 0 || position >= (int)cards.size()) return;
			int cardId = cards[position];
			std::map<int, int>::iterator existing = deck.cards.find(cardId);
			int current = existing == deck.cards.end() ? 0 : existing->second;
			int owned = cardId < (int)mCollectionCounts.size() ? mCollectionCounts[cardId] : 0;
			if (current >= std::min(4, owned)) showDeckNotice("No more owned copies are available.");
			else
			{
				if (existing == deck.cards.end()) deck.cards.insert(std::make_pair(cardId, 1));
				else ++existing->second;
				deck.dirty = true;
				if (mSoundManager != NULL) mSoundManager->playSound(SOUND_UI_CARD_ADD);
			}
			return;
		}
		return;
	}
	if (contains(DECK_PANEL, x, y) && y >= 205)
	{
		int row = (y - 205) / 33;
		int position = mDeckContentsScroll + row;
		if (row >= 0 && row < 15 && position >= 0 && position < (int)deck.cards.size())
		{
			std::map<int, int>::iterator card = deck.cards.begin();
			std::advance(card, position);
			if (--card->second <= 0) deck.cards.erase(card);
			deck.dirty = true;
			if (mSoundManager != NULL) mSoundManager->playSound(SOUND_UI_CARD_REMOVE);
		}
	}
}

void Application::renderDeckBuilder()
{
	ensurePlayerDataLoaded();
	mDeckHoveredCard = -1;
	for (std::vector<DeckCardHitbox>::reverse_iterator hitbox = mDeckCardHitboxes.rbegin();
		hitbox != mDeckCardHitboxes.rend(); ++hitbox)
	{
		if (contains(hitbox->rect, mMouseX, mMouseY))
		{
			mDeckHoveredCard = hitbox->cardId;
			break;
		}
	}
	mDeckCardHitboxes.clear();
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 11, 18, 29);
	auto button = [this](const SDL_Rect& rect, const std::string& label, bool gold)
	{
		fillRect(rect, gold ? 93 : 33, gold ? 68 : 48, gold ? 31 : 72, 250);
		outlineRect(rect, gold ? 215 : 107, gold ? 165 : 145, gold ? 67 : 199, 255, 2);
		drawText(label, rect.x + 17, rect.y + 12, color(239, 242, 248), 16, rect.w - 30);
	};
	button(BACK_BUTTON, "Back", false);
	button(NEW_DECK_BUTTON, "New Deck", false);
	button(RENAME_BUTTON, "Rename", false);
	button(SAVE_BUTTON, "Save", false);
	button(ACTIVE_BUTTON, "Set Active", true);
	fillRect(SEARCH_BOX, 23, 32, 49, 250);
	outlineRect(SEARCH_BOX, mDeckSearchFocused ? 102 : 76, mDeckSearchFocused ? 174 : 111,
		mDeckSearchFocused ? 231 : 158, 255, 2);
	drawText(mDeckSearch.empty() ? "Search collection..." : mDeckSearch,
		SEARCH_BOX.x + 15, SEARCH_BOX.y + 12,
		mDeckSearch.empty() ? color(139, 155, 181) : color(231, 235, 243), 16);

	fillRect(DECK_LIST_PANEL, 18, 26, 41, 248);
	outlineRect(DECK_LIST_PANEL, 82, 112, 160, 255, 2);
	fillRect(COLLECTION_PANEL, 17, 25, 39, 248);
	outlineRect(COLLECTION_PANEL, 82, 112, 160, 255, 2);
	fillRect(DECK_PANEL, 18, 26, 41, 248);
	outlineRect(DECK_PANEL, 82, 112, 160, 255, 2);
	drawText("MY DECKS", 36, 105, color(242, 205, 105), 19);
	drawText("COLLECTION", 278, 105, color(242, 205, 105), 19);
	drawText("Gold " + std::to_string(mMoney), 812, 108, color(245, 205, 88), 14);
	drawText("CURRENT DECK", 975, 105, color(242, 205, 105), 19);

	int visibleDecks = 12;
	mDeckListScroll = std::min(mDeckListScroll, std::max(0, (int)mPlayerDecks.size() - visibleDecks));
	for (int i = 0; i < visibleDecks && mDeckListScroll + i < (int)mPlayerDecks.size(); ++i)
	{
		int index = mDeckListScroll + i;
		SDL_Rect row = { 30, 132 + i * 48, 205, 42 };
		bool selected = index == mEditingDeckIndex;
		bool active = index == mActiveDeckIndex;
		if (active)
		{
			fillRect(row, selected ? 91 : 68, selected ? 76 : 57,
				selected ? 43 : 35, 245);
			outlineRect(row, 232, 184, 76, 255, selected ? 3 : 2);
		}
		else
		{
			fillRect(row, selected ? 54 : 27, selected ? 72 : 38,
				selected ? 103 : 57, 245);
			if (selected) outlineRect(row, 111, 161, 225, 255, 2);
		}
		drawText(mPlayerDecks[index].name, row.x + 8, row.y + 6,
			active ? color(255, 226, 145) : color(228, 233, 242), 13, 151);
		drawText(std::to_string(deckCardCount(mPlayerDecks[index])), row.x + 172, row.y + 9,
			color(190, 204, 224), 12);
	}

	std::vector<int> collection = filteredCollection();
	int pages = std::max(1, ((int)collection.size() + COLLECTION_PAGE_SIZE - 1) / COLLECTION_PAGE_SIZE);
	mDeckCollectionPage = std::min(mDeckCollectionPage, pages - 1);
	for (int slot = 0; slot < COLLECTION_PAGE_SIZE; ++slot)
	{
		int position = mDeckCollectionPage * COLLECTION_PAGE_SIZE + slot;
		if (position >= (int)collection.size()) break;
		int cardId = collection[position];
		int column = slot % 5;
		int row = slot / 5;
		SDL_Rect cardRect = { 276 + column * 130, 150 + row * 180, 108, 150 };
		SDL_Texture* texture = cardTextureById(cardId);
		if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &cardRect);
		else
		{
			fillRect(cardRect, 220, 207, 176);
			drawText(gCardDatabase[cardId].Name, cardRect.x + 5, cardRect.y + 52,
				color(39, 30, 23), 11, cardRect.w - 10);
		}
		SDL_Color civ = civilizationColor(gCardDatabase[cardId].Civilization);
		outlineRect(cardRect, civ.r, civ.g, civ.b, 255, 3);
		mDeckCardHitboxes.push_back({ cardRect, cardId });
		int used = 0;
		if (mEditingDeckIndex >= 0)
		{
			std::map<int, int>::const_iterator found = mPlayerDecks[mEditingDeckIndex].cards.find(cardId);
			if (found != mPlayerDecks[mEditingDeckIndex].cards.end()) used = found->second;
		}
		SDL_Rect copies = { cardRect.x + 4, cardRect.y + cardRect.h - 23, 55, 20 };
		fillRect(copies, 10, 16, 25, 235);
		drawText(std::to_string(used) + "/" + std::to_string(mCollectionCounts[cardId]),
			copies.x + 5, copies.y + 2, color(245, 225, 166), 11);
	}
	button(PREVIOUS_PAGE, "< Previous", false);
	button(NEXT_PAGE, "Next >", false);
	drawText("Page " + std::to_string(mDeckCollectionPage + 1) + "/" + std::to_string(pages),
		555, 729, color(170, 187, 211), 13);

	if (mEditingDeckIndex >= 0 && mEditingDeckIndex < (int)mPlayerDecks.size())
	{
		PlayerDeck& deck = mPlayerDecks[mEditingDeckIndex];
		auto drawCivilizationRow = [this](const SDL_Rect& row, const CardData& card,
			bool alternate)
		{
			int civilizations = card.Civilizations;
			if (civilizations == 0 && card.Civilization >= CIV_LIGHT &&
				card.Civilization <= CIV_HOLLOW)
				civilizations = 1 << card.Civilization;
			std::vector<int> colors;
			for (int civilization = CIV_LIGHT; civilization <= CIV_HOLLOW; ++civilization)
				if ((civilizations & (1 << civilization)) != 0)
					colors.push_back(civilization);
			if (colors.empty()) colors.push_back(card.Civilization);
			int consumed = 0;
			for (size_t colorIndex = 0; colorIndex < colors.size(); ++colorIndex)
			{
				int remaining = row.w - consumed;
				int width = colorIndex + 1 == colors.size() ? remaining :
					row.w / (int)colors.size();
				SDL_Color civ = civilizationColor(colors[colorIndex]);
				int shade = alternate ? 43 : 49;
				fillRect({ row.x + consumed, row.y, width, row.h },
					(Uint8)(civ.r * shade / 100), (Uint8)(civ.g * shade / 100),
					(Uint8)(civ.b * shade / 100), 250);
				fillRect({ row.x + consumed, row.y, width, 4 }, civ.r, civ.g, civ.b, 255);
				consumed += width;
			}
			outlineRect(row, 106, 121, 142, 255, 1);
		};
		std::string title = mDeckRenameFocused ? mDeckNameInput + "_" : deck.name;
		drawText(title, 975, 136, color(231, 236, 244), 18, 265);
		int total = deckCardCount(deck);
		drawText(std::to_string(total) + " cards  •  40 minimum" +
			(deck.dirty ? "  (unsaved)" : ""), 975, 169,
			deckHasMinimumCards(deck) ? color(105, 222, 132) : color(236, 169, 87), 14, 265);
		int visibleRows = 15;
		mDeckContentsScroll = std::min(mDeckContentsScroll, std::max(0, (int)deck.cards.size() - visibleRows));
		int i = 0;
		for (std::map<int, int>::const_iterator card = deck.cards.begin(); card != deck.cards.end(); ++card, ++i)
		{
			if (i < mDeckContentsScroll || i >= mDeckContentsScroll + visibleRows) continue;
			int visible = i - mDeckContentsScroll;
			SDL_Rect row = { 970, 205 + visible * 33, 274, 29 };
			drawCivilizationRow(row, gCardDatabase[card->first], visible % 2 != 0);
			drawText(gCardDatabase[card->first].Name, row.x + 6, row.y + 6,
				color(242, 245, 249), 12, 225);
			drawText("x" + std::to_string(card->second), row.x + 242, row.y + 6,
				color(245, 208, 119), 12);
			mDeckCardHitboxes.push_back({ row, card->first });
		}
		drawText("Click collection cards to add.", 973, 710, color(155, 174, 201), 12);
		drawText("Click deck entries to remove.", 973, 733, color(155, 174, 201), 12);
	}
	if (!mDeckNotice.empty() && SDL_GetTicks() < mDeckNoticeUntil)
	{
		fillRect({ 360, 76, 560, 38 }, 20, 36, 48, 245);
		drawText(mDeckNotice, 375, 86, color(118, 228, 151), 14, 530);
	}
	renderDeckBuilderHoverPreview();
}

void Application::renderDeckBuilderHoverPreview()
{
	if (mDeckHoveredCard < 0 || mDeckHoveredCard >= (int)gCardDatabase.size()) return;
	const CardData& card = gCardDatabase[mDeckHoveredCard];
	const int width = 300;
	const int height = 420;
	int x = mMouseX >= LOGICAL_WIDTH / 2 ? 610 : 650;
	if (mMouseX >= 940) x = 635;
	int y = 165;
	SDL_Rect shadow = { x + 10, y + 12, width, height };
	fillRect(shadow, 2, 5, 9, 170);
	SDL_Rect preview = { x, y, width, height };
	SDL_Texture* texture = cardTextureById(mDeckHoveredCard);
	if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &preview);
	else
	{
		fillRect(preview, 220, 207, 176);
		drawText(card.Name, preview.x + 18, preview.y + 150, color(39, 30, 23), 22, preview.w - 36);
	}
	SDL_Color civ = civilizationColor(card.Civilization);
	outlineRect(preview, civ.r, civ.g, civ.b, 255, 5);
}
