#pragma once

#include <string>
#include <vector>

enum class WorldObjectKind
{
	Signpost,
	DeckChest,
	Chest,
	CuttableBush,
	SmashableRock,
	Environment
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
	std::string templateId;
	int spriteIndex = -1;
	int spriteRow = 0;
	int x = 0;
	int y = 0;
	WorldObjectKind kind = WorldObjectKind::Signpost;
	bool animated = false;
	bool editorCreated = false;
};

struct WorldObjectTemplate
{
	std::string id;
	WorldObject object;
};

bool loadWorldObjectsFromLua(const std::string& path,
	std::vector<WorldObject>& objects, std::vector<WorldObjectTemplate>& templates,
	std::string& error);
WorldObject createWorldObject(const WorldObjectTemplate& objectTemplate,
	const std::string& id);
const char* worldObjectKindName(WorldObjectKind kind);
