#include "WorldData.h"

#include <set>
#include <utility>

int WorldMap::width() const
{
	return columns > 0 ? columns : (tiles.empty() ? 0 : (int)tiles[0].size());
}

int WorldMap::height() const
{
	return rows > 0 ? rows : (int)tiles.size();
}

bool WorldMap::contains(int x, int y) const
{
	return x >= 0 && y >= 0 && x < width() && y < height();
}

bool WorldMap::hasTag(int x, int y, const std::string& tag) const
{
	std::map<std::pair<int, int>, std::string>::const_iterator found =
		tags.find(std::make_pair(x, y));
	return found != tags.end() && found->second == tag;
}

void WorldMap::swap(WorldMap& other)
{
	id.swap(other.id);
	name.swap(other.name);
	std::swap(indoor, other.indoor);
	std::swap(columns, other.columns);
	std::swap(rows, other.rows);
	std::swap(catalogOnly, other.catalogOnly);
	tiles.swap(other.tiles);
	tileLayers.swap(other.tileLayers);
	tags.swap(other.tags);
}

bool WorldRegion::contains(const std::string& candidateMapId, int candidateX,
	int candidateY) const
{
	return mapId == candidateMapId && candidateX >= x && candidateY >= y &&
		candidateX < x + width && candidateY < y + height;
}

bool WorldPortal::hasEndpoint(const std::string& mapId, int x, int y) const
{
	return (fromMap == mapId && fromX == x && fromY == y) ||
		(toMap == mapId && toX == x && toY == y);
}

bool WorldPosition::operator==(const WorldPosition& other) const
{
	return mapId == other.mapId && x == other.x && y == other.y;
}

int WorldData::mapIndex(const std::string& id) const
{
	for (size_t index = 0; index < maps.size(); ++index)
		if (maps[index].id == id) return (int)index;
	return -1;
}

WorldMap* WorldData::map(const std::string& id)
{
	int index = mapIndex(id);
	return index < 0 ? NULL : &maps[index];
}

const WorldMap* WorldData::map(const std::string& id) const
{
	int index = mapIndex(id);
	return index < 0 ? NULL : &maps[index];
}

const WorldRegion* WorldData::regionAt(const std::string& mapId, int x, int y) const
{
	for (size_t index = 0; index < regions.size(); ++index)
		if (regions[index].contains(mapId, x, y)) return &regions[index];
	return NULL;
}

bool WorldData::hasPortalEndpoint(const std::string& mapId, int x, int y) const
{
	for (size_t index = 0; index < portals.size(); ++index)
		if (portals[index].hasEndpoint(mapId, x, y)) return true;
	return false;
}

bool WorldData::validateStructure(std::string& error) const
{
	error.clear();
	if (maps.empty())
	{
		error = "world needs at least one map";
		return false;
	}
	std::set<std::string> mapIds;
	for (size_t index = 0; index < maps.size(); ++index)
	{
		const WorldMap& candidate = maps[index];
		if (candidate.id.empty() || candidate.name.empty() || candidate.width() <= 0 ||
			candidate.height() <= 0 || !mapIds.insert(candidate.id).second)
		{
			error = "world maps need unique IDs, names, and non-empty dimensions";
			return false;
		}
		for (size_t row = 0; row < candidate.tiles.size(); ++row)
			if ((int)candidate.tiles[row].size() != candidate.width())
			{
				error = "map '" + candidate.id + "' is not rectangular";
				return false;
			}
		for (std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator tile =
			candidate.tileLayers.begin(); tile != candidate.tileLayers.end(); ++tile)
			if (!candidate.contains(std::get<1>(tile->first), std::get<0>(tile->first)) ||
				std::get<2>(tile->first) != (int)tile->second.layer || tile->second.index < 0)
			{
				error = "map '" + candidate.id + "' has an invalid tile-layer entry";
				return false;
			}
		for (std::map<std::pair<int, int>, std::string>::const_iterator tag =
			candidate.tags.begin(); tag != candidate.tags.end(); ++tag)
			if (!candidate.contains(tag->first.first, tag->first.second) || tag->second.empty())
			{
				error = "map '" + candidate.id + "' has an invalid gameplay tag";
				return false;
			}
	}
	const WorldMap* startMap = map(start.mapId);
	if (startMap == NULL || !startMap->contains(start.x, start.y))
	{
		error = "world start is outside its map";
		return false;
	}
	std::set<std::string> regionIds;
	for (size_t index = 0; index < regions.size(); ++index)
	{
		const WorldRegion& candidate = regions[index];
		const WorldMap* owner = map(candidate.mapId);
		if (candidate.id.empty() || candidate.name.empty() || owner == NULL ||
			candidate.width <= 0 || candidate.height <= 0 ||
			!owner->contains(candidate.x, candidate.y) ||
			!owner->contains(candidate.x + candidate.width - 1,
				candidate.y + candidate.height - 1) ||
			!regionIds.insert(candidate.id).second)
		{
			error = "world has an invalid region";
			return false;
		}
		for (size_t otherIndex = 0; otherIndex < index; ++otherIndex)
		{
			const WorldRegion& other = regions[otherIndex];
			bool overlaps = candidate.mapId == other.mapId &&
				candidate.x < other.x + other.width && candidate.x + candidate.width > other.x &&
				candidate.y < other.y + other.height && candidate.y + candidate.height > other.y;
			if (overlaps)
			{
				error = "region '" + candidate.id + "' overlaps region '" + other.id + "'";
				return false;
			}
		}
	}
	std::set<std::tuple<std::string, int, int> > portalOrigins;
	for (size_t index = 0; index < portals.size(); ++index)
	{
		const WorldPortal& portal = portals[index];
		const WorldMap* from = map(portal.fromMap);
		const WorldMap* to = map(portal.toMap);
		if (from == NULL || to == NULL || !from->contains(portal.fromX, portal.fromY) ||
			!to->contains(portal.toX, portal.toY) ||
			!portalOrigins.insert(std::make_tuple(portal.fromMap,
				portal.fromX, portal.fromY)).second)
		{
			error = "world has an invalid or duplicate portal origin";
			return false;
		}
	}
	std::set<std::tuple<std::string, int, int> > occupied;
	occupied.insert(std::make_tuple(start.mapId, start.x, start.y));
	for (size_t index = 0; index < portals.size(); ++index)
	{
		occupied.insert(std::make_tuple(portals[index].fromMap,
			portals[index].fromX, portals[index].fromY));
		occupied.insert(std::make_tuple(portals[index].toMap,
			portals[index].toX, portals[index].toY));
	}
	const std::map<std::string, WorldPosition>* positionGroups[] = {
		&npcPositions, &objectPositions, &shardPositions
	};
	for (int group = 0; group < 3; ++group)
		for (std::map<std::string, WorldPosition>::const_iterator position =
			positionGroups[group]->begin(); position != positionGroups[group]->end(); ++position)
		{
			const WorldMap* owner = map(position->second.mapId);
			if (position->first.empty() || owner == NULL ||
				!owner->contains(position->second.x, position->second.y) ||
				!occupied.insert(std::make_tuple(position->second.mapId,
					position->second.x, position->second.y)).second)
			{
				error = "world has an invalid or occupied entity position";
				return false;
			}
		}
	return true;
}

void WorldData::swap(WorldData& other)
{
	maps.swap(other.maps);
	regions.swap(other.regions);
	portals.swap(other.portals);
	std::swap(start, other.start);
	npcPositions.swap(other.npcPositions);
	objectPositions.swap(other.objectPositions);
	shardPositions.swap(other.shardPositions);
}
