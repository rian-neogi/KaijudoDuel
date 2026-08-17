#pragma once

#include <string>
#include <vector>

enum class WorldObjectKind
{
	Signpost,
	DeckChest
};

struct WorldObject
{
	std::string id;
	std::string name;
	std::string mapId;
	std::string text;
	std::string openedText;
	std::string rewardDeck;
	std::string rewardDeckName;
	std::string spriteSheet;
	int spriteIndex;
	int x;
	int y;
	WorldObjectKind kind;
};

bool loadWorldObjectsFromLua(const std::string& path,
	std::vector<WorldObject>& objects, std::string& error);
