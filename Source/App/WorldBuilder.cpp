#include "Application.h"

#include "AppSupport.h"
#include "LuaInclude.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <tuple>

using namespace AppSupport;

namespace
{
	const SDL_Rect BUILDER_PANEL = { 1008, 18, 256, 764 };
	const SDL_Rect BUILDER_PREVIOUS_MAP = { 1022, 61, 32, 34 };
	const SDL_Rect BUILDER_NEXT_MAP = { 1218, 61, 32, 34 };
	const SDL_Rect BUILDER_TILES_TAB = { 1022, 105, 70, 35 };
	const SDL_Rect BUILDER_NPCS_TAB = { 1096, 105, 70, 35 };
	const SDL_Rect BUILDER_SHARDS_TAB = { 1170, 105, 80, 35 };
	const SDL_Rect BUILDER_SAVE = { 1022, 724, 228, 44 };
	const int BUILDER_LIST_Y = 151;
	const int BUILDER_LIST_ROW = 39;
	const int BUILDER_LIST_ROWS = 13;
	const char TILE_TYPES[] = { '.', '=', '~', 'H', 'T', '#', 'W', 'D', 'F', 'C',
		'B', 'A', 'S', 'M', 'E' };
	const char* TILE_NAMES[] = { "Grass", "Path", "Water", "House", "Tree", "Forest",
		"Wood Wall", "Door", "Wood Floor", "Counter", "Bonfire", "Feast Table",
		"Duel Sand", "Marble", "Workshop Tools" };
	const int TILE_TYPE_COUNT = sizeof(TILE_TYPES) / sizeof(TILE_TYPES[0]);

	SDL_Rect paletteRect(int index)
	{
		return { 1022 + (index % 2) * 116, 151 + (index / 2) * 55, 108, 45 };
	}

	void clampMapCamera(const std::vector<std::string>& map, int& cameraX, int& cameraY)
	{
		cameraX = std::max(0, std::min(std::max(0, (int)map[0].size() - MAP_VIEW_COLUMNS),
			cameraX));
		cameraY = std::max(0, std::min(std::max(0, (int)map.size() - MAP_VIEW_ROWS),
			cameraY));
	}

	void centerMapCamera(const std::vector<std::string>& map, int x, int y,
		int& cameraX, int& cameraY)
	{
		cameraX = x - MAP_VIEW_COLUMNS / 2;
		cameraY = y - MAP_VIEW_ROWS / 2;
		clampMapCamera(map, cameraX, cameraY);
	}

	bool mapCellAt(int x, int y, const std::vector<std::string>& map,
		int cameraX, int cameraY, int& cellX, int& cellY)
	{
		if (map.empty() || map[0].empty()) return false;
		if (x < MAP_X || x >= MAP_X + MAP_VIEW_WIDTH || y < MAP_Y ||
			y >= MAP_Y + MAP_VIEW_HEIGHT) return false;
		int originX = mapOriginX((int)map[0].size()) - cameraX * TILE;
		int originY = mapOriginY((int)map.size()) - cameraY * TILE;
		if (x < originX || y < originY) return false;
		cellX = (x - originX) / TILE;
		cellY = (y - originY) / TILE;
		return cellY >= 0 && cellY < (int)map.size() && cellX >= 0 &&
			cellX < (int)map[cellY].size();
	}

	bool writeTemporary(const std::string& path, const std::string& contents, std::string& error)
	{
		std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
		if (!output)
		{
			error = "could not write " + path;
			return false;
		}
		output << contents;
		if (!output)
		{
			error = "write failed for " + path;
			return false;
		}
		return true;
	}
}

bool Application::loadWorldMap(const std::string& path, std::string& error,
	bool allowMissingPositions)
{
	error.clear();
	lua_State* state = luaL_newstate();
	if (state == NULL)
	{
		error = "could not create Lua state";
		return false;
	}
	luaL_openlibs(state);
	if (luaL_loadfile(state, path.c_str()) != LUA_OK || lua_pcall(state, 0, 1, 0) != LUA_OK)
	{
		const char* message = lua_tostring(state, -1);
		error = message == NULL ? "unknown Lua error" : message;
		lua_close(state);
		return false;
	}
	if (!lua_istable(state, -1))
	{
		error = "world file must return a table";
		lua_close(state);
		return false;
	}
	const int root = lua_gettop(state);
	auto stringField = [state](int table, const char* key) -> std::string
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		std::string value = lua_isstring(state, -1) ? lua_tostring(state, -1) : "";
		lua_pop(state, 1);
		return value;
	};
	auto intField = [state](int table, const char* key) -> int
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		int value = lua_isnumber(state, -1) ? (int)lua_tointeger(state, -1) : -1;
		lua_pop(state, 1);
		return value;
	};
	auto walkable = [](char tile)
	{
		return tile == '.' || tile == '=' || tile == 'F' || tile == 'D' || tile == 'S';
	};
	auto inBounds = [](const WorldArea& area, int x, int y) -> bool
	{
		return y >= 0 && y < (int)area.tiles.size() && x >= 0 &&
			x < (int)area.tiles[y].size();
	};

	std::vector<WorldArea> areas;
	std::set<std::string> areaIds;
	lua_getfield(state, root, "maps");
	if (!lua_istable(state, -1) || lua_rawlen(state, -1) == 0)
	{
		error = "world file needs at least one map";
		lua_close(state);
		return false;
	}
	const size_t mapCount = lua_rawlen(state, -1);
	for (size_t index = 1; index <= mapCount; ++index)
	{
		lua_rawgeti(state, -1, (lua_Integer)index);
		const int mapTable = lua_gettop(state);
		WorldArea area;
		area.id = stringField(mapTable, "id");
		area.name = stringField(mapTable, "name");
		lua_getfield(state, mapTable, "indoor");
		area.indoor = lua_toboolean(state, -1) != 0;
		lua_pop(state, 1);
		if (area.id.empty() || area.name.empty() || !areaIds.insert(area.id).second)
		{
			error = "map entries need unique id and name fields";
			lua_close(state);
			return false;
		}
		lua_getfield(state, mapTable, "tiles");
		if (!lua_istable(state, -1) || lua_rawlen(state, -1) == 0 ||
			lua_rawlen(state, -1) > WORLD_MAX_ROWS)
		{
			error = "map '" + area.id + "' must contain between 1 and " +
				std::to_string(WORLD_MAX_ROWS) + " rows";
			lua_close(state);
			return false;
		}
		const int rowCount = (int)lua_rawlen(state, -1);
		size_t columnCount = 0;
		for (int row = 1; row <= rowCount; ++row)
		{
			lua_rawgeti(state, -1, row);
			std::string value = lua_isstring(state, -1) ? lua_tostring(state, -1) : "";
			lua_pop(state, 1);
			if (row == 1) columnCount = value.size();
			if (columnCount == 0 || columnCount > WORLD_MAX_COLUMNS ||
				value.size() != columnCount ||
				value.find_first_not_of(".=~HT#WDFCBASME") != std::string::npos)
			{
				error = "map '" + area.id + "' row " + std::to_string(row) +
					" must match its first row and contain between 1 and " +
					std::to_string(WORLD_MAX_COLUMNS) + " valid tile characters";
				lua_close(state);
				return false;
			}
			area.tiles.push_back(value);
		}
		lua_pop(state, 2);
		areas.push_back(area);
	}
	lua_pop(state, 1);
	auto areaIndex = [&areas](const std::string& id) -> int
	{
		for (size_t i = 0; i < areas.size(); ++i) if (areas[i].id == id) return (int)i;
		return -1;
	};

	lua_getfield(state, root, "start");
	std::string startMap = lua_istable(state, -1) ? stringField(-1, "map") : "";
	int startX = lua_istable(state, -1) ? intField(-1, "x") : -1;
	int startY = lua_istable(state, -1) ? intField(-1, "y") : -1;
	lua_pop(state, 1);
	int startArea = areaIndex(startMap);
	if (startArea < 0 || !inBounds(areas[startArea], startX, startY) ||
		!walkable(areas[startArea].tiles[startY][startX]))
	{
		error = "world start must reference a walkable map tile";
		lua_close(state);
		return false;
	}

	std::vector<WorldPortal> portals;
	lua_getfield(state, root, "portals");
	if (!lua_istable(state, -1))
	{
		error = "world file needs a portals table";
		lua_close(state);
		return false;
	}
	std::set<std::tuple<std::string, int, int> > occupied;
	occupied.insert(std::make_tuple(startMap, startX, startY));
	for (size_t index = 1; index <= lua_rawlen(state, -1); ++index)
	{
		lua_rawgeti(state, -1, (lua_Integer)index);
		const int portalTable = lua_gettop(state);
		WorldPortal portal;
		lua_getfield(state, portalTable, "from");
		portal.fromMap = lua_istable(state, -1) ? stringField(-1, "map") : "";
		portal.fromX = lua_istable(state, -1) ? intField(-1, "x") : -1;
		portal.fromY = lua_istable(state, -1) ? intField(-1, "y") : -1;
		lua_pop(state, 1);
		lua_getfield(state, portalTable, "to");
		portal.toMap = lua_istable(state, -1) ? stringField(-1, "map") : "";
		portal.toX = lua_istable(state, -1) ? intField(-1, "x") : -1;
		portal.toY = lua_istable(state, -1) ? intField(-1, "y") : -1;
		lua_pop(state, 2);
		int fromArea = areaIndex(portal.fromMap);
		int toArea = areaIndex(portal.toMap);
		if (fromArea < 0 || toArea < 0 ||
			!inBounds(areas[fromArea], portal.fromX, portal.fromY) ||
			!inBounds(areas[toArea], portal.toX, portal.toY) ||
			!walkable(areas[fromArea].tiles[portal.fromY][portal.fromX]) ||
			!walkable(areas[toArea].tiles[portal.toY][portal.toX]) ||
			!occupied.insert(std::make_tuple(portal.fromMap, portal.fromX, portal.fromY)).second)
		{
			error = "portal " + std::to_string(index) + " has invalid endpoints";
			lua_close(state);
			return false;
		}
		occupied.insert(std::make_tuple(portal.toMap, portal.toX, portal.toY));
		portals.push_back(portal);
	}
	lua_pop(state, 1);

	struct LoadedPosition { std::string map; int x; int y; };
	std::vector<LoadedPosition> npcPositions(mNpcs.size());
	std::vector<LoadedPosition> shardPositions(mMercerStock.shards.size());
	auto readPositions = [&](const char* field, const std::vector<std::string>& ids,
		std::vector<LoadedPosition>& positions) -> bool
	{
		lua_getfield(state, root, field);
		if (!lua_istable(state, -1))
		{
			error = std::string("world file needs a '") + field + "' position table";
			lua_pop(state, 1);
			return false;
		}
		const int table = lua_gettop(state);
		for (size_t i = 0; i < ids.size(); ++i)
		{
			lua_getfield(state, table, ids[i].c_str());
			if (!lua_istable(state, -1))
			{
				lua_pop(state, 1);
				if (allowMissingPositions) { positions[i] = { "", -1, -1 }; continue; }
				error = std::string("missing ") + field + " position for '" + ids[i] + "'";
				lua_pop(state, 1);
				return false;
			}
			positions[i].map = stringField(-1, "map");
			positions[i].x = intField(-1, "x");
			positions[i].y = intField(-1, "y");
			lua_pop(state, 1);
			int mapIndex = areaIndex(positions[i].map);
			if (mapIndex < 0 || !inBounds(areas[mapIndex], positions[i].x, positions[i].y) ||
				!walkable(areas[mapIndex].tiles[positions[i].y][positions[i].x]) ||
				!occupied.insert(std::make_tuple(positions[i].map, positions[i].x, positions[i].y)).second)
			{
				error = std::string("invalid or occupied ") + field + " position for '" + ids[i] + "'";
				lua_pop(state, 1);
				return false;
			}
		}
		lua_pop(state, 1);
		return true;
	};
	std::vector<std::string> npcIds;
	for (size_t i = 0; i < mNpcs.size(); ++i) npcIds.push_back(mNpcs[i].id);
	std::vector<std::string> shardIds;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i) shardIds.push_back(mMercerStock.shards[i].id);
	if (!readPositions("npcs", npcIds, npcPositions) ||
		!readPositions("shards", shardIds, shardPositions))
	{
		lua_close(state);
		return false;
	}
	bool placedMissing = false;
	auto placeMissing = [&](std::vector<LoadedPosition>& positions) -> bool
	{
		for (size_t i = 0; i < positions.size(); ++i)
		{
			if (!positions[i].map.empty()) continue;
			bool placed = false;
			for (size_t map = 0; map < areas.size() && !placed; ++map)
				for (int y = 0; y < (int)areas[map].tiles.size() && !placed; ++y)
					for (int x = 0; x < (int)areas[map].tiles[y].size() && !placed; ++x)
						if (walkable(areas[map].tiles[y][x]) &&
							occupied.insert(std::make_tuple(areas[map].id, x, y)).second)
						{
							positions[i] = { areas[map].id, x, y };
							placed = placedMissing = true;
						}
			if (!placed) { error = "no free walkable tile is available for new world entities"; return false; }
		}
		return true;
	};
	if (!placeMissing(npcPositions) || !placeMissing(shardPositions))
	{
		lua_close(state);
		return false;
	}
	lua_close(state);
	mWorldAreas.swap(areas);
	mWorldPortals.swap(portals);
	mWorldStartMap = startMap;
	mWorldStartX = startX;
	mWorldStartY = startY;
	mCurrentWorldArea = worldAreaIndex(startMap);
	mPlayerX = startX;
	mPlayerY = startY;
	mVisualX = (float)startX;
	mVisualY = (float)startY;
	centerMapCamera(currentMap(), startX, startY, mWorldBuilderCameraX,
		mWorldBuilderCameraY);
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		mNpcs[i].mapId = npcPositions[i].map;
		mNpcs[i].setPosition(npcPositions[i].x, npcPositions[i].y);
	}
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		mMercerStock.shards[i].mapId = shardPositions[i].map;
		mMercerStock.shards[i].x = shardPositions[i].x;
		mMercerStock.shards[i].y = shardPositions[i].y;
	}
	if (placedMissing)
	{
		mWorldBuilderDirty = true;
		mWorldBuilderNotice = "New Lua entities were placed automatically. Review and save the world.";
		mWorldBuilderNoticeError = false;
		mWorldBuilderNoticeUntil = static_cast<Uint32>(-1);
	}
	return true;
}

std::vector<std::string>& Application::currentMap()
{
	return mWorldAreas[mCurrentWorldArea].tiles;
}

const std::vector<std::string>& Application::currentMap() const
{
	return mWorldAreas[mCurrentWorldArea].tiles;
}

const std::string& Application::currentMapId() const
{
	return mWorldAreas[mCurrentWorldArea].id;
}

int Application::worldAreaIndex(const std::string& id) const
{
	for (size_t i = 0; i < mWorldAreas.size(); ++i)
		if (mWorldAreas[i].id == id) return (int)i;
	return -1;
}

bool Application::isPortalAt(const std::string& mapId, int x, int y) const
{
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
		if ((mWorldPortals[i].fromMap == mapId && mWorldPortals[i].fromX == x &&
			mWorldPortals[i].fromY == y) ||
			(mWorldPortals[i].toMap == mapId && mWorldPortals[i].toX == x &&
			mWorldPortals[i].toY == y)) return true;
	return false;
}

bool Application::beginPortalAt(int x, int y)
{
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
	{
		const WorldPortal& portal = mWorldPortals[i];
		if (portal.fromMap != currentMapId() || portal.fromX != x || portal.fromY != y)
			continue;
		if (worldAreaIndex(portal.toMap) < 0) return false;
		mOpeningPortal = (int)i;
		mPortalAnimationStarted = SDL_GetTicks();
		mDialogueNpc = -1;
		return true;
	}
	return false;
}

bool Application::activatePortalAt(int x, int y)
{
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
	{
		const WorldPortal& portal = mWorldPortals[i];
		if (portal.fromMap != currentMapId() || portal.fromX != x || portal.fromY != y) continue;
		int destination = worldAreaIndex(portal.toMap);
		if (destination < 0) return false;
		mCurrentWorldArea = destination;
		mPlayerX = portal.toX;
		mPlayerY = portal.toY;
		mVisualX = (float)mPlayerX;
		mVisualY = (float)mPlayerY;
		mOpeningPortal = -1;
		mPortalAnimationStarted = 0;
		mDialogueNpc = -1;
		mNotice = mWorldAreas[destination].indoor ? "Entered " + mWorldAreas[destination].name + "." :
			"Returned to " + mWorldAreas[destination].name + ".";
		mNoticeUntil = SDL_GetTicks() + 2500;
		return true;
	}
	return false;
}

void Application::showWorldBuilderNotice(const std::string& notice, bool error)
{
	mWorldBuilderNotice = notice;
	mWorldBuilderNoticeError = error;
	mWorldBuilderNoticeUntil = SDL_GetTicks() + 4500;
}

bool Application::worldBuilderCanPlace(int x, int y, int ignoredNpc, int ignoredShard) const
{
	if (!isWalkable(x, y) ||
		(currentMapId() == mWorldStartMap && x == mWorldStartX && y == mWorldStartY) ||
		isPortalAt(currentMapId(), x, y)) return false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if ((int)i != ignoredNpc && mNpcs[i].mapId == currentMapId() &&
			mNpcs[i].x == x && mNpcs[i].y == y) return false;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		if ((int)i != ignoredShard && mMercerStock.shards[i].mapId == currentMapId() &&
			mMercerStock.shards[i].x == x &&
			mMercerStock.shards[i].y == y) return false;
	return true;
}

void Application::paintWorldBuilderTile(int x, int y)
{
	std::vector<std::string>& map = currentMap();
	if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size()) return;
	bool walkable = mWorldBuilderTile == '.' || mWorldBuilderTile == '=' ||
		mWorldBuilderTile == 'F' || mWorldBuilderTile == 'D' || mWorldBuilderTile == 'S';
	if (!walkable)
	{
		bool hasNpc = false;
		for (size_t i = 0; i < mNpcs.size(); ++i)
			if (mNpcs[i].mapId == currentMapId() && mNpcs[i].x == x && mNpcs[i].y == y) hasNpc = true;
		if ((currentMapId() == mWorldStartMap && x == mWorldStartX && y == mWorldStartY) ||
			hasNpc || isPortalAt(currentMapId(), x, y))
		{
			showWorldBuilderNotice("Move the player start or NPC before blocking this tile.", true);
			return;
		}
		for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
			if (mMercerStock.shards[i].mapId == currentMapId() &&
				mMercerStock.shards[i].x == x && mMercerStock.shards[i].y == y)
			{
				showWorldBuilderNotice("Move the shard before blocking this tile.", true);
				return;
			}
	}
	if (map[y][x] == mWorldBuilderTile) return;
	map[y][x] = mWorldBuilderTile;
	mWorldBuilderDirty = true;
}

void Application::placeWorldBuilderSelection(int x, int y)
{
	if (mWorldBuilderTab == WorldBuilderTab::Npcs && mWorldBuilderSelectedNpc >= 0 &&
		mWorldBuilderSelectedNpc < (int)mNpcs.size())
	{
		if (!worldBuilderCanPlace(x, y, mWorldBuilderSelectedNpc, -1))
		{
			showWorldBuilderNotice("NPCs require an empty grass/path tile.", true);
			return;
		}
		Npc& npc = mNpcs[mWorldBuilderSelectedNpc];
		if (npc.mapId == currentMapId() && npc.x == x && npc.y == y) return;
		npc.mapId = currentMapId();
		npc.setPosition(x, y);
		mWorldBuilderDirty = true;
	}
	else if (mWorldBuilderTab == WorldBuilderTab::Shards && mWorldBuilderSelectedShard >= 0 &&
		mWorldBuilderSelectedShard < (int)mMercerStock.shards.size())
	{
		if (!worldBuilderCanPlace(x, y, -1, mWorldBuilderSelectedShard))
		{
			showWorldBuilderNotice("Shards require an empty grass/path tile.", true);
			return;
		}
		MercerShard& shard = mMercerStock.shards[mWorldBuilderSelectedShard];
		if (shard.mapId == currentMapId() && shard.x == x && shard.y == y) return;
		shard.mapId = currentMapId();
		shard.x = x;
		shard.y = y;
		mWorldBuilderDirty = true;
	}
}

bool Application::saveWorldBuilder(std::string& error)
{
	std::ostringstream world;
	world << "-- World Builder data. This file is entirely maintained by the World Builder.\n"
		<< "-- Tile legend: . grass, = path, ~ water, H house, T tree, # forest,\n"
		<< "-- W wooden wall, D door, F wooden floor, C counter, B bonfire,\n"
		<< "-- A feast table, S dueling sand, M marble, E workshop tools.\n"
		<< "return {\n\tmaps = {\n";
	for (size_t area = 0; area < mWorldAreas.size(); ++area)
	{
		world << "\t\t{\n\t\t\tid = \"" << mWorldAreas[area].id << "\",\n"
			<< "\t\t\tname = \"" << mWorldAreas[area].name << "\",\n"
			<< "\t\t\tindoor = " << (mWorldAreas[area].indoor ? "true" : "false") << ",\n"
			<< "\t\t\ttiles = {\n";
		for (size_t row = 0; row < mWorldAreas[area].tiles.size(); ++row)
			world << "\t\t\t\t\"" << mWorldAreas[area].tiles[row] << "\"" <<
				(row + 1 == mWorldAreas[area].tiles.size() ? "\n" : ",\n");
		world << "\t\t\t}\n\t\t}" << (area + 1 == mWorldAreas.size() ? "\n" : ",\n");
	}
	world << "\t},\n\tstart = { map = \"" << mWorldStartMap << "\", x = " << mWorldStartX <<
		", y = " << mWorldStartY << " },\n\tportals = {\n";
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
	{
		const WorldPortal& portal = mWorldPortals[i];
		world << "\t\t{ from = { map = \"" << portal.fromMap << "\", x = " << portal.fromX <<
			", y = " << portal.fromY << " },\n\t\t\tto = { map = \"" << portal.toMap <<
			"\", x = " << portal.toX << ", y = " << portal.toY << " } }" <<
			(i + 1 == mWorldPortals.size() ? "\n" : ",\n");
	}
	world << "\t},\n\tnpcs = {\n";
	for (size_t i = 0; i < mNpcs.size(); ++i)
		world << "\t\t[\"" << mNpcs[i].id << "\"] = { map = \"" << mNpcs[i].mapId <<
			"\", x = " << mNpcs[i].x << ", y = " << mNpcs[i].y << " },\n";
	world << "\t},\n\tshards = {\n";
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		world << "\t\t[\"" << mMercerStock.shards[i].id << "\"] = { map = \"" <<
			mMercerStock.shards[i].mapId << "\", x = " << mMercerStock.shards[i].x <<
			", y = " << mMercerStock.shards[i].y << " },\n";
	world << "\t}\n}\n";

	const std::string worldTemp = "Lua/World.lua.worldbuilder.tmp";
	if (!writeTemporary(worldTemp, world.str(), error))
	{
		std::remove(worldTemp.c_str());
		return false;
	}
	if (std::rename(worldTemp.c_str(), "Lua/World.lua") != 0)
	{
		error = "could not replace Lua/World.lua: " + std::string(std::strerror(errno));
		std::remove(worldTemp.c_str());
		return false;
	}
	mWorldBuilderDirty = false;
	return true;
}

void Application::handleWorldBuilderEvent(const SDL_Event& event)
{
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			mRunning = false;
			return;
		}
		if (key == SDLK_s && (event.key.keysym.mod & KMOD_CTRL))
		{
			std::string error;
			if (saveWorldBuilder(error)) showWorldBuilderNotice("World saved to Lua.");
			else showWorldBuilderNotice("Save failed: " + error, true);
			return;
		}
		if (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN)
		{
			mWorldBuilderCameraX += key == SDLK_LEFT ? -1 : (key == SDLK_RIGHT ? 1 : 0);
			mWorldBuilderCameraY += key == SDLK_UP ? -1 : (key == SDLK_DOWN ? 1 : 0);
			clampMapCamera(currentMap(), mWorldBuilderCameraX, mWorldBuilderCameraY);
			return;
		}
		if (key >= SDLK_1 && key <= SDLK_9)
		{
			mWorldBuilderTab = WorldBuilderTab::Tiles;
			mWorldBuilderTile = TILE_TYPES[key - SDLK_1];
			return;
		}
		if (key == SDLK_t) mWorldBuilderTab = WorldBuilderTab::Tiles;
		else if (key == SDLK_n) mWorldBuilderTab = WorldBuilderTab::Npcs;
		else if (key == SDLK_s) mWorldBuilderTab = WorldBuilderTab::Shards;
		else if (key == SDLK_PAGEUP)
		{
			mCurrentWorldArea = (mCurrentWorldArea + (int)mWorldAreas.size() - 1) % (int)mWorldAreas.size();
			mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
		}
		else if (key == SDLK_PAGEDOWN)
		{
			mCurrentWorldArea = (mCurrentWorldArea + 1) % (int)mWorldAreas.size();
			mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
		}
		else return;
		mWorldBuilderListScroll = 0;
		return;
	}
	if (event.type == SDL_MOUSEWHEEL && mWorldBuilderTab != WorldBuilderTab::Tiles)
	{
		int count = mWorldBuilderTab == WorldBuilderTab::Npcs ? (int)mNpcs.size() :
			(int)mMercerStock.shards.size();
		int maximum = std::max(0, count - BUILDER_LIST_ROWS);
		mWorldBuilderListScroll = std::max(0,
			std::min(maximum, mWorldBuilderListScroll - event.wheel.y));
		return;
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		mWorldBuilderPainting = false;
		mWorldBuilderDragging = false;
		return;
	}
	if (event.type == SDL_MOUSEMOTION)
	{
		int x, y;
		logicalMouse(event.motion.x, event.motion.y, x, y);
		int cellX, cellY;
		if (mapCellAt(x, y, currentMap(), mWorldBuilderCameraX,
			mWorldBuilderCameraY, cellX, cellY))
		{
			if (mWorldBuilderPainting) paintWorldBuilderTile(cellX, cellY);
			else if (mWorldBuilderDragging) placeWorldBuilderSelection(cellX, cellY);
		}
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(BUILDER_SAVE, x, y))
	{
		std::string error;
		if (saveWorldBuilder(error)) showWorldBuilderNotice("World saved to Lua.");
		else showWorldBuilderNotice("Save failed: " + error, true);
		return;
	}
	if (contains(BUILDER_PREVIOUS_MAP, x, y))
	{
		mCurrentWorldArea = (mCurrentWorldArea + (int)mWorldAreas.size() - 1) % (int)mWorldAreas.size();
		mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
		return;
	}
	if (contains(BUILDER_NEXT_MAP, x, y))
	{
		mCurrentWorldArea = (mCurrentWorldArea + 1) % (int)mWorldAreas.size();
		mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
		return;
	}
	if (contains(BUILDER_TILES_TAB, x, y))
		mWorldBuilderTab = WorldBuilderTab::Tiles;
	else if (contains(BUILDER_NPCS_TAB, x, y))
		mWorldBuilderTab = WorldBuilderTab::Npcs;
	else if (contains(BUILDER_SHARDS_TAB, x, y))
		mWorldBuilderTab = WorldBuilderTab::Shards;
	else
	{
		if (mWorldBuilderTab == WorldBuilderTab::Tiles)
		{
			for (int i = 0; i < TILE_TYPE_COUNT; ++i)
				if (contains(paletteRect(i), x, y))
				{
					mWorldBuilderTile = TILE_TYPES[i];
					return;
				}
		}
		else if (x >= 1022 && x < 1250 && y >= BUILDER_LIST_Y &&
			y < BUILDER_LIST_Y + BUILDER_LIST_ROWS * BUILDER_LIST_ROW)
		{
			int selected = mWorldBuilderListScroll + (y - BUILDER_LIST_Y) / BUILDER_LIST_ROW;
			if (mWorldBuilderTab == WorldBuilderTab::Npcs && selected < (int)mNpcs.size())
			{
				mWorldBuilderSelectedNpc = selected;
				int map = worldAreaIndex(mNpcs[selected].mapId);
				if (map >= 0)
				{
					mCurrentWorldArea = map;
					centerMapCamera(currentMap(), mNpcs[selected].x, mNpcs[selected].y,
						mWorldBuilderCameraX, mWorldBuilderCameraY);
				}
			}
			else if (mWorldBuilderTab == WorldBuilderTab::Shards &&
				selected < (int)mMercerStock.shards.size())
			{
				mWorldBuilderSelectedShard = selected;
				int map = worldAreaIndex(mMercerStock.shards[selected].mapId);
				if (map >= 0)
				{
					mCurrentWorldArea = map;
					centerMapCamera(currentMap(), mMercerStock.shards[selected].x,
						mMercerStock.shards[selected].y, mWorldBuilderCameraX,
						mWorldBuilderCameraY);
				}
			}
			return;
		}
		int cellX, cellY;
		if (!mapCellAt(x, y, currentMap(), mWorldBuilderCameraX,
			mWorldBuilderCameraY, cellX, cellY)) return;
		if (mWorldBuilderTab == WorldBuilderTab::Tiles)
		{
			mWorldBuilderPainting = true;
			paintWorldBuilderTile(cellX, cellY);
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Npcs)
		{
			int hit = -1;
			for (size_t i = 0; i < mNpcs.size(); ++i)
				if (mNpcs[i].mapId == currentMapId() && mNpcs[i].x == cellX &&
					mNpcs[i].y == cellY) hit = (int)i;
			if (hit >= 0) mWorldBuilderSelectedNpc = hit;
			else placeWorldBuilderSelection(cellX, cellY);
		}
		else
		{
			int hit = -1;
			for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
				if (mMercerStock.shards[i].mapId == currentMapId() &&
					mMercerStock.shards[i].x == cellX && mMercerStock.shards[i].y == cellY) hit = (int)i;
			if (hit >= 0) mWorldBuilderSelectedShard = hit;
			else placeWorldBuilderSelection(cellX, cellY);
		}
		mWorldBuilderDragging = true;
		return;
	}
	mWorldBuilderListScroll = 0;
}

void Application::renderWorldBuilder()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 12, 18, 29);
	drawText("WORLD BUILDER", 32, 13, color(244, 207, 103), 25);
	drawText(mWorldAreas[mCurrentWorldArea].name, 320, 17, color(189, 207, 232), 18, 360);
	drawText(mWorldBuilderDirty ? "UNSAVED CHANGES" : "SAVED", 795, 19,
		mWorldBuilderDirty ? color(244, 139, 88) : color(105, 218, 139), 14);
	const std::vector<std::string>& map = currentMap();
	int mapX = mapOriginX((int)map[0].size()) - mWorldBuilderCameraX * TILE;
	int mapY = mapOriginY((int)map.size()) - mWorldBuilderCameraY * TILE;
	SDL_Rect mapViewport = { MAP_X, MAP_Y, MAP_VIEW_WIDTH, MAP_VIEW_HEIGHT };
	SDL_RenderSetClipRect(mRenderer, &mapViewport);
	for (size_t row = 0; row < map.size(); ++row)
	{
		for (size_t column = 0; column < map[row].size(); ++column)
		{
			SDL_Rect tile = { mapX + (int)column * TILE, mapY + (int)row * TILE, TILE, TILE };
			char type = map[row][column];
			if (type == '=') fillRect(tile, 162, 132, 76);
			else if (type == '~') fillRect(tile, 25, 111, 157);
			else if (type == 'H') fillRect(tile, 126, 65, 43);
			else if (type == '#' || type == 'T') fillRect(tile, 26, 75, 33);
			else if (type == 'S') fillRect(tile, 188, 151, 87);
			else if (type == 'M') fillRect(tile, 198, 204, 207);
			else if (type == 'B') fillRect(tile, 83, 64, 42);
			else if (type == 'A') fillRect(tile, 61, 139, 61);
			else if (type == 'E') fillRect(tile, 137, 91, 49);
			else if (type == 'W' || type == 'D' || type == 'F' || type == 'C')
				fillRect(tile, type == 'F' ? 137 : 91, type == 'F' ? 91 : 53, type == 'F' ? 49 : 31);
			else fillRect(tile, 61, 139, 61);
			if (type == '~')
				fillRect({ tile.x + 7, tile.y + 17, 28, 3 }, 92, 189, 210, 190);
			else if (type == '#' || type == 'T')
			{
				fillRect({ tile.x + 19, tile.y + 27, 10, 18 }, 85, 48, 26);
				fillRect({ tile.x + 6, tile.y + 5, 36, 30 }, 41, 116, 49);
			}
			else if (type == 'H')
			{
				fillRect({ tile.x + 3, tile.y + 4, 42, 16 }, 186, 76, 46);
				fillRect({ tile.x + 18, tile.y + 23, 13, 25 }, 54, 31, 24);
			}
			else if (type == 'W')
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 124, 72, 38);
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tile.x + 3, tile.y + 8 + plank * 10, 42, 2 }, 76, 42, 27);
				fillRect({ tile.x + 5, tile.y + 2, 5, 46 }, 67, 38, 26);
				fillRect({ tile.x + 38, tile.y + 2, 5, 46 }, 67, 38, 26);
			}
			else if (type == 'D')
			{
				fillRect({ tile.x + 7, tile.y + 2, 34, 46 }, 104, 57, 31);
				fillRect({ tile.x + 11, tile.y + 6, 26, 38 }, 139, 78, 39);
				fillRect({ tile.x + 30, tile.y + 24, 4, 4 }, 231, 184, 73);
			}
			else if (type == 'F')
			{
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tile.x, tile.y + plank * 12, 48, 2 }, 96, 58, 36);
			}
			else if (type == 'C')
			{
				fillRect({ tile.x + 2, tile.y + 10, 44, 32 }, 112, 62, 34);
				fillRect({ tile.x, tile.y + 7, 48, 7 }, 166, 105, 53);
			}
			else if (type == 'B')
			{
				fillRect({ tile.x + 5, tile.y + 32, 38, 8 }, 74, 70, 65);
				fillRect({ tile.x + 10, tile.y + 29, 28, 10 }, 111, 100, 83);
				fillRect({ tile.x + 17, tile.y + 13, 15, 22 }, 221, 69, 32);
				fillRect({ tile.x + 20, tile.y + 8, 10, 23 }, 249, 137, 36);
				fillRect({ tile.x + 23, tile.y + 16, 6, 15 }, 255, 222, 89);
			}
			else if (type == 'A')
			{
				fillRect({ tile.x + 6, tile.y + 6, 36, 5 }, 93, 52, 29);
				fillRect({ tile.x + 6, tile.y + 37, 36, 5 }, 93, 52, 29);
				fillRect({ tile.x + 4, tile.y + 15, 40, 18 }, 139, 82, 39);
				fillRect({ tile.x + 12, tile.y + 23, 7, 6 }, 232, 208, 151);
				fillRect({ tile.x + 29, tile.y + 22, 8, 7 }, 175, 49, 37);
			}
			else if (type == 'S')
			{
				fillRect({ tile.x + 7, tile.y + 10, 5, 3 }, 157, 121, 69);
				fillRect({ tile.x + 34, tile.y + 31, 6, 3 }, 211, 177, 109);
			}
			else if (type == 'M')
			{
				fillRect({ tile.x + 2, tile.y + 2, 44, 44 }, 220, 224, 224);
				fillRect({ tile.x + 3, tile.y + 22, 42, 2 }, 162, 173, 179);
				fillRect({ tile.x + 17, tile.y + 3, 2, 19 }, 177, 186, 190);
				fillRect({ tile.x + 31, tile.y + 24, 2, 21 }, 177, 186, 190);
			}
			else if (type == 'E')
			{
				fillRect({ tile.x + 3, tile.y + 24, 42, 17 }, 104, 58, 32);
				fillRect({ tile.x + 5, tile.y + 20, 38, 6 }, 171, 108, 51);
				fillRect({ tile.x + 9, tile.y + 10, 7, 11 }, 54, 151, 193);
				fillRect({ tile.x + 20, tile.y + 7, 6, 14 }, 151, 71, 183);
				fillRect({ tile.x + 31, tile.y + 12, 8, 9 }, 217, 158, 48);
			}
			outlineRect(tile, 10, 20, 27, 100, 1);
		}
	}

	if (currentMapId() == mWorldStartMap)
	{
		fillRect({ mapX + mWorldStartX * TILE + 13, mapY + mWorldStartY * TILE + 12,
			23, 25 }, 24, 66, 137, 235);
		drawText("P", mapX + mWorldStartX * TILE + 19, mapY + mWorldStartY * TILE + 16,
			color(215, 232, 255), 14);
	}
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
		if (mWorldPortals[i].fromMap == currentMapId())
			outlineRect({ mapX + mWorldPortals[i].fromX * TILE + 4,
				mapY + mWorldPortals[i].fromY * TILE + 4, TILE - 8, TILE - 8 },
				91, 222, 232, 255, 3);
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		if (shard.mapId != currentMapId()) continue;
		int x = mapX + shard.x * TILE;
		int y = mapY + shard.y * TILE;
		fillRect({ x + 18, y + 9, 14, 30 }, 47, 25, 71, 230);
		fillRect({ x + 13, y + 15, 24, 18 }, 146, 87, 211, 250);
		fillRect({ x + 19, y + 19, 12, 10 }, 231, 193, 255, 255);
		if ((int)i == mWorldBuilderSelectedShard && mWorldBuilderTab == WorldBuilderTab::Shards)
			outlineRect({ x + 2, y + 2, TILE - 4, TILE - 4 }, 246, 211, 99, 255, 3);
	}
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (mNpcs[i].mapId != currentMapId()) continue;
		drawCharacter((float)mNpcs[i].x, (float)mNpcs[i].y, mNpcs[i].appearance, false, false);
		if ((int)i == mWorldBuilderSelectedNpc && mWorldBuilderTab == WorldBuilderTab::Npcs)
			outlineRect({ mapX + mNpcs[i].x * TILE + 2, mapY + mNpcs[i].y * TILE + 2,
				TILE - 4, TILE - 4 }, 246, 211, 99, 255, 3);
	}
	SDL_RenderSetClipRect(mRenderer, NULL);

	fillRect(BUILDER_PANEL, 20, 28, 44, 248);
	outlineRect(BUILDER_PANEL, 184, 140, 60, 255, 2);
	drawText("EDIT WORLD", 1027, 32, color(240, 205, 108), 21);
	fillRect(BUILDER_PREVIOUS_MAP, 37, 47, 67, 245);
	fillRect(BUILDER_NEXT_MAP, 37, 47, 67, 245);
	outlineRect(BUILDER_PREVIOUS_MAP, 113, 139, 176, 255, 2);
	outlineRect(BUILDER_NEXT_MAP, 113, 139, 176, 255, 2);
	drawText("<", 1033, 68, color(229, 235, 245), 14);
	drawText(">", 1229, 68, color(229, 235, 245), 14);
	drawText(mWorldAreas[mCurrentWorldArea].name, 1061, 69, color(214, 222, 236), 12, 150);
	auto tab = [this](const SDL_Rect& rect, const std::string& label, bool active)
	{
		fillRect(rect, active ? 82 : 38, active ? 67 : 46, active ? 39 : 65, 245);
		outlineRect(rect, active ? 235 : 109, active ? 184 : 120, active ? 80 : 143, 255, 2);
		drawText(label, rect.x + 9, rect.y + 9, color(235, 238, 245), 12);
	};
	tab(BUILDER_TILES_TAB, "TILES", mWorldBuilderTab == WorldBuilderTab::Tiles);
	tab(BUILDER_NPCS_TAB, "NPCS", mWorldBuilderTab == WorldBuilderTab::Npcs);
	tab(BUILDER_SHARDS_TAB, "SHARDS", mWorldBuilderTab == WorldBuilderTab::Shards);

	if (mWorldBuilderTab == WorldBuilderTab::Tiles)
	{
		for (int i = 0; i < TILE_TYPE_COUNT; ++i)
		{
			SDL_Rect button = paletteRect(i);
			bool selected = mWorldBuilderTile == TILE_TYPES[i];
			fillRect(button, selected ? 79 : 38, selected ? 68 : 47, selected ? 43 : 64, 245);
			outlineRect(button, selected ? 241 : 110, selected ? 190 : 125,
				selected ? 87 : 147, 255, 2);
			drawText((i < 9 ? std::to_string(i + 1) + "  " : "   ") + TILE_NAMES[i], button.x + 9,
				button.y + 15, color(236, 239, 246), 13);
		}
		drawText("Click and drag across the map to paint.", 1022, 605,
			color(179, 195, 218), 13, 220);
	}
	else
	{
		int count = mWorldBuilderTab == WorldBuilderTab::Npcs ? (int)mNpcs.size() :
			(int)mMercerStock.shards.size();
		int selected = mWorldBuilderTab == WorldBuilderTab::Npcs ? mWorldBuilderSelectedNpc :
			mWorldBuilderSelectedShard;
		for (int row = 0; row < BUILDER_LIST_ROWS; ++row)
		{
			int index = mWorldBuilderListScroll + row;
			if (index >= count) break;
			SDL_Rect item = { 1022, BUILDER_LIST_Y + row * BUILDER_LIST_ROW, 228, 34 };
			fillRect(item, index == selected ? 74 : (row % 2 ? 31 : 37),
				index == selected ? 61 : 42, index == selected ? 42 : 59, 238);
			if (index == selected) outlineRect(item, 233, 184, 82, 255, 2);
			const std::string& name = mWorldBuilderTab == WorldBuilderTab::Npcs ?
				mNpcs[index].name : mMercerStock.shards[index].name;
			int entityX = mWorldBuilderTab == WorldBuilderTab::Npcs ? mNpcs[index].x :
				mMercerStock.shards[index].x;
			int entityY = mWorldBuilderTab == WorldBuilderTab::Npcs ? mNpcs[index].y :
				mMercerStock.shards[index].y;
			drawText(name, item.x + 7, item.y + 5, color(232, 236, 244), 12, 166);
			drawText(std::to_string(entityX) + "," + std::to_string(entityY), item.x + 181,
				item.y + 5, color(173, 193, 220), 11);
		}
		if (count > BUILDER_LIST_ROWS)
			drawText("Mouse wheel scrolls", 1052, 682, color(166, 184, 211), 12);
	}

	fillRect(BUILDER_SAVE, mWorldBuilderDirty ? 74 : 45, mWorldBuilderDirty ? 76 : 55,
		mWorldBuilderDirty ? 43 : 67, 250);
	outlineRect(BUILDER_SAVE, 207, 161, 66, 255, 2);
	drawText("SAVE TO LUA   Ctrl+S", BUILDER_SAVE.x + 29, BUILDER_SAVE.y + 12,
		color(245, 226, 181), 14);
	drawText("T/N/S: tabs  •  Arrows: pan  •  Tiles: paint  •  Entities: select and place",
		32, 650, color(180, 196, 219), 14, 930);
	drawText("P marks the player start; cyan outlines mark portals. PageUp/PageDown changes maps.",
		32, 676, color(142, 173, 217), 13);
	if (!mWorldBuilderNotice.empty() && SDL_GetTicks() < mWorldBuilderNoticeUntil)
	{
		fillRect({ 32, 716, 930, 42 }, mWorldBuilderNoticeError ? 71 : 25,
			mWorldBuilderNoticeError ? 30 : 62, mWorldBuilderNoticeError ? 31 : 43, 235);
		drawText(mWorldBuilderNotice, 45, 727,
			mWorldBuilderNoticeError ? color(255, 176, 166) : color(132, 234, 156), 15, 900);
	}
}
