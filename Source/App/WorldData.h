#pragma once

#include "RtpTile.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>

struct WorldMap
{
	std::string id;
	std::string name;
	bool indoor = false;
	int columns = 0;
	int rows = 0;
	bool catalogOnly = false;
	std::vector<std::string> tiles;
	std::map<std::tuple<int, int, int>, RtpTileReference> tileLayers;
	std::map<std::pair<int, int>, std::string> tags;

	int width() const;
	int height() const;
	bool contains(int x, int y) const;
	bool hasTag(int x, int y, const std::string& tag) const;
	void swap(WorldMap& other);
};

struct WorldRegion
{
	std::string id;
	std::string name;
	std::string mapId;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool connector = false;

	bool contains(const std::string& candidateMapId, int candidateX,
		int candidateY) const;
};

struct WorldPortal
{
	std::string fromMap;
	int fromX = 0;
	int fromY = 0;
	std::string toMap;
	int toX = 0;
	int toY = 0;

	bool hasEndpoint(const std::string& mapId, int x, int y) const;
};

struct WorldPosition
{
	std::string mapId;
	int x = -1;
	int y = -1;

	bool operator==(const WorldPosition& other) const;
};

class WorldData
{
public:
	std::vector<WorldMap> maps;
	std::vector<WorldRegion> regions;
	std::vector<WorldPortal> portals;
	WorldPosition start;
	std::map<std::string, WorldPosition> npcPositions;
	std::map<std::string, WorldPosition> objectPositions;
	std::map<std::string, WorldPosition> shardPositions;

	int mapIndex(const std::string& id) const;
	WorldMap* map(const std::string& id);
	const WorldMap* map(const std::string& id) const;
	const WorldRegion* regionAt(const std::string& mapId, int x, int y) const;
	bool hasPortalEndpoint(const std::string& mapId, int x, int y) const;
	bool validateStructure(std::string& error) const;
	void swap(WorldData& other);
};
