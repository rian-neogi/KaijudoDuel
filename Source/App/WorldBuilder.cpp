#include "Application.h"

#include "AppSupport.h"
#include "LuaInclude.h"
#include "WorldTileRenderer.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
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
	const SDL_Rect BUILDER_OBJECTS_TAB = { 1170, 105, 80, 35 };
	const SDL_Rect BUILDER_SAVE = { 1022, 724, 228, 44 };
	const int BUILDER_LIST_Y = 151;
	const int BUILDER_LIST_ROW = 39;
	const int BUILDER_LIST_ROWS = 13;
	const int BUILDER_CATEGORY_Y = 151;
	const int BUILDER_TILE_Y = 223;
	const int BUILDER_TILE_ROWS = 10;
	const int BUILDER_VISIBLE_TILES = BUILDER_TILE_ROWS * 2;
	const Uint32 BUILDER_PAN_INTERVAL = 80;
	const int BUILDER_ZOOM_LEVELS[] = { 24, 32, 48, 64, 96 };
	const int BUILDER_ZOOM_LEVEL_COUNT = sizeof(BUILDER_ZOOM_LEVELS) /
		sizeof(BUILDER_ZOOM_LEVELS[0]);
	const WorldTileId TILE_TYPES[] = { WorldTiles::Grass, WorldTiles::Path,
		WorldTiles::Water, WorldTiles::House, WorldTiles::Tree, WorldTiles::Forest,
		WorldTiles::WoodWall, WorldTiles::Door, WorldTiles::WoodFloor, WorldTiles::Counter,
		WorldTiles::Bonfire, WorldTiles::FeastTable, WorldTiles::DuelSand,
		WorldTiles::Marble, WorldTiles::MarbleRoof, WorldTiles::WorkshopTools,
		WorldTiles::Rail, WorldTiles::RailCrossing, WorldTiles::MetalGrate,
		WorldTiles::IndustrialBrick, WorldTiles::Machinery, WorldTiles::Furnace,
		WorldTiles::TimberRoof, WorldTiles::IndustrialRoof, WorldTiles::TimberBridge,
		WorldTiles::RockyCliff, WorldTiles::OldRoadPath, WorldTiles::OldRoadWaystone,
		WorldTiles::CinderrailGround, WorldTiles::CinderrailPath,
		WorldTiles::CinderrailRubble, WorldTiles::CinderrailDuelSand,
		WorldTiles::CinderrailDoor, WorldTiles::WatershedGround,
		WorldTiles::WatershedPath, WorldTiles::WatershedMarker,
		WorldTiles::GlasswaterGround, WorldTiles::GlasswaterPaving,
		WorldTiles::GlasswaterRoof, WorldTiles::GlasswaterDock,
		WorldTiles::GlasswaterWall, WorldTiles::GlasswaterDoor,
		WorldTiles::GlasswaterArena, WorldTiles::GlasswaterMarker,
		WorldTiles::RootmazeGround, WorldTiles::RootmazePath,
		WorldTiles::RootmazeRoot, WorldTiles::RootmazeBridge,
		WorldTiles::RootmazeRoof, WorldTiles::RootmazeWall,
		WorldTiles::RootmazeDoor, WorldTiles::RootmazeArena,
		WorldTiles::RootmazeMarker, WorldTiles::Rocks, WorldTiles::Bush,
		WorldTiles::Shrub, WorldTiles::CaveEntrance, WorldTiles::TreeStump,
		WorldTiles::BlackstoneGround, WorldTiles::BlackstonePath,
		WorldTiles::BlackstoneWall, WorldTiles::BlackstoneGate };
	const int TILE_TYPE_COUNT = sizeof(TILE_TYPES) / sizeof(TILE_TYPES[0]);
	const int TILE_CATEGORY_COUNT = 4;
	const char* TILE_CATEGORY_NAMES[TILE_CATEGORY_COUNT] = {
		"GROUND", "BUILDINGS", "NATURE", "SPECIAL"
	};

	enum TileCategory
	{
		GroundTiles,
		BuildingTiles,
		NatureTiles,
		SpecialTiles
	};

	SDL_Rect tileCategoryRect(int index)
	{
		return { 1022 + (index % 2) * 116, BUILDER_CATEGORY_Y + (index / 2) * 34,
			108, 29 };
	}

	SDL_Rect paletteRect(int index)
	{
		return { 1022 + (index % 2) * 116, BUILDER_TILE_Y + (index / 2) * 39, 108, 33 };
	}

	int tileCategory(WorldTileId type)
	{
		if (type == WorldTiles::Grass || type == WorldTiles::Path ||
			type == WorldTiles::Water || type == WorldTiles::DuelSand ||
			type == WorldTiles::Marble || type == WorldTiles::RailCrossing ||
			type == WorldTiles::MetalGrate || type == WorldTiles::TimberBridge ||
			type == WorldTiles::OldRoadPath || type == WorldTiles::CinderrailGround ||
			type == WorldTiles::CinderrailPath || type == WorldTiles::CinderrailDuelSand ||
			type == WorldTiles::WatershedGround || type == WorldTiles::WatershedPath ||
			type == WorldTiles::GlasswaterGround || type == WorldTiles::GlasswaterPaving ||
			type == WorldTiles::GlasswaterDock || type == WorldTiles::GlasswaterArena ||
			type == WorldTiles::RootmazeGround || type == WorldTiles::RootmazePath ||
			type == WorldTiles::RootmazeBridge || type == WorldTiles::RootmazeArena ||
			type == WorldTiles::BlackstoneGround || type == WorldTiles::BlackstonePath)
			return GroundTiles;
		if (type == WorldTiles::House || type == WorldTiles::WoodWall ||
			type == WorldTiles::Door || type == WorldTiles::WoodFloor ||
			type == WorldTiles::Counter || type == WorldTiles::MarbleRoof ||
			type == WorldTiles::IndustrialBrick || type == WorldTiles::TimberRoof ||
			type == WorldTiles::IndustrialRoof || type == WorldTiles::CinderrailDoor ||
			type == WorldTiles::GlasswaterRoof || type == WorldTiles::GlasswaterWall ||
			type == WorldTiles::GlasswaterDoor || type == WorldTiles::RootmazeRoof ||
			type == WorldTiles::RootmazeWall || type == WorldTiles::RootmazeDoor ||
			type == WorldTiles::BlackstoneWall)
			return BuildingTiles;
		if (type == WorldTiles::Tree || type == WorldTiles::Forest ||
			type == WorldTiles::RockyCliff || type == WorldTiles::RootmazeRoot ||
			type == WorldTiles::Rocks || type == WorldTiles::Bush ||
			type == WorldTiles::Shrub || type == WorldTiles::CaveEntrance ||
			type == WorldTiles::TreeStump)
			return NatureTiles;
		return SpecialTiles;
	}

	std::vector<WorldTileId> categoryTiles(int category)
	{
		std::vector<WorldTileId> tiles;
		for (int i = 0; i < TILE_TYPE_COUNT; ++i)
			if (tileCategory(TILE_TYPES[i]) == category) tiles.push_back(TILE_TYPES[i]);
		return tiles;
	}

	const char* tileName(WorldTileId type)
	{
		if (type == WorldTiles::Grass) return "Grass";
		if (type == WorldTiles::Path) return "Path";
		if (type == WorldTiles::Water) return "Water";
		if (type == WorldTiles::House) return "House";
		if (type == WorldTiles::Tree) return "Tree";
		if (type == WorldTiles::Forest) return "Dense Forest";
		if (type == WorldTiles::WoodWall) return "Wood Wall";
		if (type == WorldTiles::Door) return "Wooden Door";
		if (type == WorldTiles::WoodFloor) return "Wood Floor";
		if (type == WorldTiles::Counter) return "Counter";
		if (type == WorldTiles::Bonfire) return "Bonfire";
		if (type == WorldTiles::FeastTable) return "Feast Table";
		if (type == WorldTiles::DuelSand) return "Dueling Sand";
		if (type == WorldTiles::Marble) return "Marble Floor";
		if (type == WorldTiles::MarbleRoof) return "Marble Roof";
		if (type == WorldTiles::WorkshopTools) return "Workshop Tools";
		if (type == WorldTiles::Rail) return "Rail";
		if (type == WorldTiles::RailCrossing) return "Rail Crossing";
		if (type == WorldTiles::MetalGrate) return "Metal Grate";
		if (type == WorldTiles::IndustrialBrick) return "Industrial Brick";
		if (type == WorldTiles::Machinery) return "Machinery";
		if (type == WorldTiles::Furnace) return "Furnace";
		if (type == WorldTiles::TimberRoof) return "Timber Roof";
		if (type == WorldTiles::IndustrialRoof) return "Industrial Roof";
		if (type == WorldTiles::TimberBridge) return "Timber Bridge";
		if (type == WorldTiles::RockyCliff) return "Rocky Cliff";
		if (type == WorldTiles::OldRoadPath) return "Old Road Path";
		if (type == WorldTiles::OldRoadWaystone) return "Waystone";
		if (type == WorldTiles::CinderrailGround) return "Cinderrail Ground";
		if (type == WorldTiles::CinderrailPath) return "Cinderrail Path";
		if (type == WorldTiles::CinderrailRubble) return "Cinderrail Rubble";
		if (type == WorldTiles::CinderrailDuelSand) return "Cinderrail Dueling Sand";
		if (type == WorldTiles::CinderrailDoor) return "Cinderrail Door";
		if (type == WorldTiles::WatershedGround) return "Watershed Ground";
		if (type == WorldTiles::WatershedPath) return "Watershed Path";
		if (type == WorldTiles::WatershedMarker) return "Watershed Route Marker";
		if (type == WorldTiles::GlasswaterGround) return "Glasswater Ground";
		if (type == WorldTiles::GlasswaterPaving) return "Tideglass Paving";
		if (type == WorldTiles::GlasswaterRoof) return "Glasswater Roof";
		if (type == WorldTiles::GlasswaterDock) return "Dock";
		if (type == WorldTiles::GlasswaterWall) return "Glasswater Wall";
		if (type == WorldTiles::GlasswaterDoor) return "Glasswater Door";
		if (type == WorldTiles::GlasswaterArena) return "Tidal Arena Floor";
		if (type == WorldTiles::GlasswaterMarker) return "Harbor Marker";
		if (type == WorldTiles::RootmazeGround) return "Rootmaze Ground";
		if (type == WorldTiles::RootmazePath) return "Rootmaze Path";
		if (type == WorldTiles::RootmazeRoot) return "Living Root";
		if (type == WorldTiles::RootmazeBridge) return "Root Bridge";
		if (type == WorldTiles::RootmazeRoof) return "Living Roof";
		if (type == WorldTiles::RootmazeWall) return "Root Wall";
		if (type == WorldTiles::RootmazeDoor) return "Rootmaze Door";
		if (type == WorldTiles::RootmazeArena) return "Meadow Arena Floor";
		if (type == WorldTiles::RootmazeMarker) return "Leaf Marker";
		if (type == WorldTiles::Rocks) return "Rocks";
		if (type == WorldTiles::Bush) return "Bush";
		if (type == WorldTiles::Shrub) return "Shrub";
		if (type == WorldTiles::CaveEntrance) return "Cave Entrance";
		if (type == WorldTiles::TreeStump) return "Tree Stump";
		if (type == WorldTiles::BlackstoneGround) return "Blackstone Ground";
		if (type == WorldTiles::BlackstonePath) return "Blackstone Road";
		if (type == WorldTiles::BlackstoneWall) return "Blackstone Retaining Wall";
		if (type == WorldTiles::BlackstoneGate) return "Blackstone Relay Gate";
		return "Unknown Tile";
	}

	bool worldBuilderMovementKey(SDL_Keycode key, int& dx, int& dy)
	{
		dx = 0;
		dy = 0;
		if (key == SDLK_a || key == SDLK_LEFT) dx = -1;
		else if (key == SDLK_d || key == SDLK_RIGHT) dx = 1;
		else if (key == SDLK_w || key == SDLK_UP) dy = -1;
		else if (key == SDLK_s || key == SDLK_DOWN) dy = 1;
		else return false;
		return true;
	}

	int builderViewportColumns(int tileSize)
	{
		return MAP_VIEW_WIDTH / tileSize;
	}

	int builderViewportRows(int tileSize)
	{
		return MAP_VIEW_HEIGHT / tileSize;
	}

	int builderMapOriginX(int columns, int tileSize)
	{
		return MAP_X + std::max(0, MAP_VIEW_WIDTH - columns * tileSize) / 2;
	}

	int builderMapOriginY(int rows, int tileSize)
	{
		return MAP_Y + std::max(0, MAP_VIEW_HEIGHT - rows * tileSize) / 2;
	}

	void clampMapCamera(const std::vector<std::string>& map, int& cameraX, int& cameraY,
		int tileSize)
	{
		cameraX = std::max(0, std::min(std::max(0,
			(int)map[0].size() - builderViewportColumns(tileSize)), cameraX));
		cameraY = std::max(0, std::min(std::max(0,
			(int)map.size() - builderViewportRows(tileSize)), cameraY));
	}

	void centerMapCamera(const std::vector<std::string>& map, int x, int y,
		int& cameraX, int& cameraY, int tileSize)
	{
		cameraX = x - builderViewportColumns(tileSize) / 2;
		cameraY = y - builderViewportRows(tileSize) / 2;
		clampMapCamera(map, cameraX, cameraY, tileSize);
	}

	bool mapCellAt(int x, int y, const std::vector<std::string>& map,
		int cameraX, int cameraY, int tileSize, int& cellX, int& cellY)
	{
		if (map.empty() || map[0].empty()) return false;
		if (x < MAP_X || x >= MAP_X + MAP_VIEW_WIDTH || y < MAP_Y ||
			y >= MAP_Y + MAP_VIEW_HEIGHT) return false;
		int originX = builderMapOriginX((int)map[0].size(), tileSize) - cameraX * tileSize;
		int originY = builderMapOriginY((int)map.size(), tileSize) - cameraY * tileSize;
		if (x < originX || y < originY) return false;
		cellX = (x - originX) / tileSize;
		cellY = (y - originY) / tileSize;
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
		return WorldTiles::isWalkable(WorldTiles::fromGlyph(tile));
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
			bool validTiles = true;
			for (size_t column = 0; column < value.size(); ++column)
				validTiles = validTiles && WorldTiles::isValid(WorldTiles::fromGlyph(value[column]));
			if (columnCount == 0 || columnCount > WORLD_MAX_COLUMNS ||
				value.size() != columnCount || !validTiles)
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

	std::vector<WorldRegion> regions;
	std::set<std::string> regionIds;
	lua_getfield(state, root, "regions");
	if (lua_istable(state, -1))
	{
		for (size_t index = 1; index <= lua_rawlen(state, -1); ++index)
		{
			lua_rawgeti(state, -1, (lua_Integer)index);
			const int regionTable = lua_gettop(state);
			WorldRegion region;
			region.id = stringField(regionTable, "id");
			region.name = stringField(regionTable, "name");
			region.mapId = stringField(regionTable, "map");
			std::string regionKind = stringField(regionTable, "kind");
			region.connector = regionKind == "connector";
			region.x = intField(regionTable, "x");
			region.y = intField(regionTable, "y");
			region.width = intField(regionTable, "width");
			region.height = intField(regionTable, "height");
			lua_pop(state, 1);
			int mapIndex = areaIndex(region.mapId);
			if (region.id.empty() || region.name.empty() ||
				(regionKind != "town" && regionKind != "connector") ||
				!regionIds.insert(region.id).second || mapIndex < 0 ||
				region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 ||
				region.x + region.width > (int)areas[mapIndex].tiles[0].size() ||
				region.y + region.height > (int)areas[mapIndex].tiles.size())
			{
				error = "region " + std::to_string(index) + " has invalid bounds or identity";
				lua_close(state);
				return false;
			}
			for (size_t existing = 0; existing < regions.size(); ++existing)
			{
				const WorldRegion& other = regions[existing];
				bool overlaps = other.mapId == region.mapId &&
					region.x < other.x + other.width && region.x + region.width > other.x &&
					region.y < other.y + other.height && region.y + region.height > other.y;
				if (overlaps)
				{
					error = "region '" + region.id + "' overlaps region '" + other.id + "'";
					lua_close(state);
					return false;
				}
			}
			regions.push_back(region);
		}
	}
	lua_pop(state, 1);

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
	std::vector<LoadedPosition> objectPositions(mWorldObjects.size());
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
	std::vector<std::string> objectIds;
	for (size_t i = 0; i < mWorldObjects.size(); ++i) objectIds.push_back(mWorldObjects[i].id);
	std::vector<std::string> shardIds;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i) shardIds.push_back(mMercerStock.shards[i].id);
	if (!readPositions("npcs", npcIds, npcPositions) ||
		!readPositions("objects", objectIds, objectPositions) ||
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
	if (!placeMissing(npcPositions) || !placeMissing(objectPositions) ||
		!placeMissing(shardPositions))
	{
		lua_close(state);
		return false;
	}
	lua_close(state);
	mWorldAreas.swap(areas);
	mWorldRegions.swap(regions);
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
		mWorldBuilderCameraY, mWorldBuilderTileSize);
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		mNpcs[i].mapId = npcPositions[i].map;
		mNpcs[i].setPosition(npcPositions[i].x, npcPositions[i].y);
	}
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
	{
		mWorldObjects[i].mapId = objectPositions[i].map;
		mWorldObjects[i].x = objectPositions[i].x;
		mWorldObjects[i].y = objectPositions[i].y;
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

const Application::WorldRegion* Application::worldRegionAt(const std::string& mapId,
	int x, int y) const
{
	for (size_t i = 0; i < mWorldRegions.size(); ++i)
	{
		const WorldRegion& region = mWorldRegions[i];
		if (region.mapId == mapId && x >= region.x && y >= region.y &&
			x < region.x + region.width && y < region.y + region.height)
			return &region;
	}
	return NULL;
}

const Application::WorldRegion* Application::currentWorldRegion() const
{
	return worldRegionAt(currentMapId(), (int)std::round(mVisualX),
		(int)std::round(mVisualY));
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
		mDialogueObject = -1;
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
		mDialogueObject = -1;
		mNotice = mWorldAreas[destination].indoor ? "Entered " + mWorldAreas[destination].name + "." :
			(portal.toMap == mWorldStartMap ? "Returned to " : "Arrived at ") +
			mWorldAreas[destination].name + ".";
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

bool Application::worldBuilderCanPlace(int x, int y, int ignoredNpc, int ignoredObject) const
{
	if (!isWalkable(x, y) ||
		(currentMapId() == mWorldStartMap && x == mWorldStartX && y == mWorldStartY) ||
		isPortalAt(currentMapId(), x, y)) return false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if ((int)i != ignoredNpc && mNpcs[i].mapId == currentMapId() &&
			mNpcs[i].x == x && mNpcs[i].y == y) return false;
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
		if ((int)i != ignoredObject && mWorldObjects[i].mapId == currentMapId() &&
			mWorldObjects[i].x == x && mWorldObjects[i].y == y) return false;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		if ((int)(mWorldObjects.size() + i) != ignoredObject &&
			mMercerStock.shards[i].mapId == currentMapId() &&
			mMercerStock.shards[i].x == x &&
			mMercerStock.shards[i].y == y) return false;
	return true;
}

void Application::paintWorldBuilderTile(int x, int y)
{
	std::vector<std::string>& map = currentMap();
	if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size()) return;
	bool walkable = WorldTiles::isWalkable(mWorldBuilderTile);
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
				showWorldBuilderNotice("Move the object before blocking this tile.", true);
				return;
			}
		for (size_t i = 0; i < mWorldObjects.size(); ++i)
			if (mWorldObjects[i].mapId == currentMapId() &&
				mWorldObjects[i].x == x && mWorldObjects[i].y == y)
			{
				showWorldBuilderNotice("Move the object before blocking this tile.", true);
				return;
			}
	}
	if (WorldTiles::fromGlyph(map[y][x]) == mWorldBuilderTile) return;
	map[y][x] = WorldTiles::glyph(mWorldBuilderTile);
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
	else if (mWorldBuilderTab == WorldBuilderTab::Objects && mWorldBuilderSelectedObject >= 0 &&
		mWorldBuilderSelectedObject < (int)(mWorldObjects.size() + mMercerStock.shards.size()))
	{
		if (!worldBuilderCanPlace(x, y, -1, mWorldBuilderSelectedObject))
		{
			showWorldBuilderNotice("Objects require an empty walkable tile.", true);
			return;
		}
		if (mWorldBuilderSelectedObject < (int)mWorldObjects.size())
		{
			WorldObject& object = mWorldObjects[mWorldBuilderSelectedObject];
			if (object.mapId == currentMapId() && object.x == x && object.y == y) return;
			object.mapId = currentMapId();
			object.x = x;
			object.y = y;
		}
		else
		{
			int shardIndex = mWorldBuilderSelectedObject - (int)mWorldObjects.size();
			MercerShard& shard = mMercerStock.shards[shardIndex];
			if (shard.mapId == currentMapId() && shard.x == x && shard.y == y) return;
			shard.mapId = currentMapId();
			shard.x = x;
			shard.y = y;
		}
		mWorldBuilderDirty = true;
	}
}

bool Application::saveWorldBuilder(std::string& error)
{
	std::ostringstream world;
	world << "-- World Builder data. This file is entirely maintained by the World Builder.\n"
		<< "-- Tile IDs are stable one-byte serialization codes defined in Source/App/WorldTile.h.\n"
		<< "-- Tile legend: . grass, = path, ~ water, H house, T tree, # forest,\n"
		<< "-- W wooden wall, K timber roof, D door, F wooden floor, C counter, B bonfire,\n"
		<< "-- A feast table, S dueling sand, M marble, Q marble roof, E workshop tools,\n"
		<< "-- R rail, X walkable rail crossing, G metal grate, I industrial brick,\n"
		<< "-- P machinery, V furnace, J industrial roof, U timber bridge, O rocky cliff.\n"
		<< "-- Explicit regional IDs: 1 old-road path, 2 waystone, 3 Cinderrail ground,\n"
		<< "-- 4 Cinderrail path, 5 rubble, 6 Cinderrail sand, 7 Cinderrail door,\n"
		<< "-- 8 Watershed ground, 9 Watershed path, 0 Watershed route marker.\n"
		<< "-- Glasswater IDs: a ground, b paving, c roof, d dock, e wall,\n"
		<< "-- f door, g arena floor, h harbor marker.\n"
		<< "-- Rootmaze IDs: i ground, j path, k root, l bridge, m roof, n wall,\n"
		<< "-- o door, p arena floor, q leaf marker. Natural objects: r-v.\n"
		<< "-- Blackstone IDs: w ground, x road, y retaining wall, z relay gate.\n"
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
	world << "\t},\n\tregions = {\n";
	for (size_t i = 0; i < mWorldRegions.size(); ++i)
	{
		const WorldRegion& region = mWorldRegions[i];
		world << "\t\t{ id = \"" << region.id << "\", name = \"" << region.name <<
			"\", map = \"" << region.mapId << "\", kind = \"" <<
			(region.connector ? "connector" : "town") << "\", x = " << region.x << ", y = " <<
			region.y << ", width = " << region.width << ", height = " << region.height <<
			" }" << (i + 1 == mWorldRegions.size() ? "\n" : ",\n");
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
	world << "\t},\n\tobjects = {\n";
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
		world << "\t\t[\"" << mWorldObjects[i].id << "\"] = { map = \"" <<
			mWorldObjects[i].mapId << "\", x = " << mWorldObjects[i].x <<
			", y = " << mWorldObjects[i].y << " },\n";
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

void Application::panWorldBuilder(int dx, int dy)
{
	mWorldBuilderCameraX += dx;
	mWorldBuilderCameraY += dy;
	clampMapCamera(currentMap(), mWorldBuilderCameraX, mWorldBuilderCameraY,
		mWorldBuilderTileSize);
}

void Application::zoomWorldBuilder(int direction, int anchorX, int anchorY)
{
	int currentLevel = 0;
	for (int level = 0; level < BUILDER_ZOOM_LEVEL_COUNT; ++level)
		if (BUILDER_ZOOM_LEVELS[level] == mWorldBuilderTileSize) currentLevel = level;
	int nextLevel = std::max(0, std::min(BUILDER_ZOOM_LEVEL_COUNT - 1,
		currentLevel + direction));
	if (nextLevel == currentLevel) return;

	const std::vector<std::string>& map = currentMap();
	if (anchorX < MAP_X || anchorX >= MAP_X + MAP_VIEW_WIDTH ||
		anchorY < MAP_Y || anchorY >= MAP_Y + MAP_VIEW_HEIGHT)
	{
		anchorX = MAP_X + MAP_VIEW_WIDTH / 2;
		anchorY = MAP_Y + MAP_VIEW_HEIGHT / 2;
	}
	int oldOriginX = builderMapOriginX((int)map[0].size(), mWorldBuilderTileSize) -
		mWorldBuilderCameraX * mWorldBuilderTileSize;
	int oldOriginY = builderMapOriginY((int)map.size(), mWorldBuilderTileSize) -
		mWorldBuilderCameraY * mWorldBuilderTileSize;
	float worldX = (anchorX - oldOriginX) / (float)mWorldBuilderTileSize;
	float worldY = (anchorY - oldOriginY) / (float)mWorldBuilderTileSize;
	mWorldBuilderTileSize = BUILDER_ZOOM_LEVELS[nextLevel];
	int newBaseX = builderMapOriginX((int)map[0].size(), mWorldBuilderTileSize);
	int newBaseY = builderMapOriginY((int)map.size(), mWorldBuilderTileSize);
	mWorldBuilderCameraX = (int)std::round(worldX -
		(anchorX - newBaseX) / (float)mWorldBuilderTileSize);
	mWorldBuilderCameraY = (int)std::round(worldY -
		(anchorY - newBaseY) / (float)mWorldBuilderTileSize);
	clampMapCamera(map, mWorldBuilderCameraX, mWorldBuilderCameraY,
		mWorldBuilderTileSize);
	mWorldBuilderPainting = false;
	mWorldBuilderDragging = false;
}

void Application::updateWorldBuilder(Uint32 deltaTime)
{
	int dx = (mWorldBuilderMoveRight ? 1 : 0) - (mWorldBuilderMoveLeft ? 1 : 0);
	int dy = (mWorldBuilderMoveDown ? 1 : 0) - (mWorldBuilderMoveUp ? 1 : 0);
	if (dx == 0 && dy == 0)
	{
		mWorldBuilderPanAccumulator = 0;
		return;
	}
	mWorldBuilderPanAccumulator += deltaTime;
	while (mWorldBuilderPanAccumulator >= BUILDER_PAN_INTERVAL)
	{
		panWorldBuilder(dx, dy);
		mWorldBuilderPanAccumulator -= BUILDER_PAN_INTERVAL;
	}
}

void Application::handleWorldBuilderEvent(const SDL_Event& event)
{
	if (event.type == SDL_WINDOWEVENT &&
		event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
	{
		mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
		mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
		mWorldBuilderPanAccumulator = 0;
		return;
	}
	if (event.type == SDL_KEYUP)
	{
		int dx, dy;
		SDL_Keycode key = event.key.keysym.sym;
		if (!worldBuilderMovementKey(key, dx, dy)) return;
		if (key == SDLK_w || key == SDLK_UP) mWorldBuilderMoveUp = false;
		else if (key == SDLK_s || key == SDLK_DOWN) mWorldBuilderMoveDown = false;
		else if (key == SDLK_a || key == SDLK_LEFT) mWorldBuilderMoveLeft = false;
		else mWorldBuilderMoveRight = false;
		return;
	}
	if (event.type == SDL_KEYDOWN)
	{
		if (event.key.repeat) return;
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
		if (key == SDLK_EQUALS || key == SDLK_KP_PLUS)
		{
			zoomWorldBuilder(1, MAP_X + MAP_VIEW_WIDTH / 2,
				MAP_Y + MAP_VIEW_HEIGHT / 2);
			return;
		}
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS)
		{
			zoomWorldBuilder(-1, MAP_X + MAP_VIEW_WIDTH / 2,
				MAP_Y + MAP_VIEW_HEIGHT / 2);
			return;
		}
		int dx, dy;
		if (worldBuilderMovementKey(key, dx, dy))
		{
			if (key == SDLK_w || key == SDLK_UP) mWorldBuilderMoveUp = true;
			else if (key == SDLK_s || key == SDLK_DOWN) mWorldBuilderMoveDown = true;
			else if (key == SDLK_a || key == SDLK_LEFT) mWorldBuilderMoveLeft = true;
			else mWorldBuilderMoveRight = true;
			mWorldBuilderPanAccumulator = 0;
			panWorldBuilder(dx, dy);
			return;
		}
		if (key >= SDLK_1 && key <= SDLK_9)
		{
			mWorldBuilderTab = WorldBuilderTab::Tiles;
			std::vector<WorldTileId> tiles = categoryTiles(mWorldBuilderTileCategory);
			int index = key - SDLK_1;
			if (index < (int)tiles.size()) mWorldBuilderTile = tiles[index];
			return;
		}
		if (key == SDLK_t) mWorldBuilderTab = WorldBuilderTab::Tiles;
		else if (key == SDLK_n) mWorldBuilderTab = WorldBuilderTab::Npcs;
		else if (key == SDLK_o) mWorldBuilderTab = WorldBuilderTab::Objects;
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
	if (event.type == SDL_MOUSEWHEEL)
	{
		if (mMouseX >= MAP_X && mMouseX < MAP_X + MAP_VIEW_WIDTH &&
			mMouseY >= MAP_Y && mMouseY < MAP_Y + MAP_VIEW_HEIGHT)
		{
			if (event.wheel.y != 0)
				zoomWorldBuilder(event.wheel.y > 0 ? 1 : -1, mMouseX, mMouseY);
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Tiles)
		{
			int maximum = std::max(0,
				(int)categoryTiles(mWorldBuilderTileCategory).size() - BUILDER_VISIBLE_TILES);
			mWorldBuilderListScroll = std::max(0,
				std::min(maximum, mWorldBuilderListScroll - event.wheel.y * 2));
			return;
		}
		int count = mWorldBuilderTab == WorldBuilderTab::Npcs ? (int)mNpcs.size() :
			(int)(mWorldObjects.size() + mMercerStock.shards.size());
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
		mMouseX = x;
		mMouseY = y;
		if (mapCellAt(x, y, currentMap(), mWorldBuilderCameraX,
			mWorldBuilderCameraY, mWorldBuilderTileSize, cellX, cellY))
		{
			if (mWorldBuilderPainting) paintWorldBuilderTile(cellX, cellY);
			else if (mWorldBuilderDragging) placeWorldBuilderSelection(cellX, cellY);
		}
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	mMouseX = x;
	mMouseY = y;
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
	{
		mWorldBuilderTab = WorldBuilderTab::Tiles;
		mWorldBuilderListScroll = 0;
	}
	else if (contains(BUILDER_NPCS_TAB, x, y))
	{
		mWorldBuilderTab = WorldBuilderTab::Npcs;
		mWorldBuilderListScroll = 0;
	}
	else if (contains(BUILDER_OBJECTS_TAB, x, y))
	{
		mWorldBuilderTab = WorldBuilderTab::Objects;
		mWorldBuilderListScroll = 0;
	}
	else
	{
		if (mWorldBuilderTab == WorldBuilderTab::Tiles)
		{
			for (int category = 0; category < TILE_CATEGORY_COUNT; ++category)
			{
				if (!contains(tileCategoryRect(category), x, y)) continue;
				mWorldBuilderTileCategory = category;
				mWorldBuilderListScroll = 0;
				return;
			}
			std::vector<WorldTileId> tiles = categoryTiles(mWorldBuilderTileCategory);
			for (int slot = 0; slot < BUILDER_VISIBLE_TILES; ++slot)
			{
				int i = mWorldBuilderListScroll + slot;
				if (i < (int)tiles.size() && contains(paletteRect(slot), x, y))
				{
					mWorldBuilderTile = tiles[i];
					return;
				}
			}
		}
		else if (x >= 1022 && x < 1250 && y >= BUILDER_LIST_Y &&
			y < BUILDER_LIST_Y + BUILDER_LIST_ROWS * BUILDER_LIST_ROW)
		{
			int selected = mWorldBuilderListScroll + (y - BUILDER_LIST_Y) / BUILDER_LIST_ROW;
			if (mWorldBuilderTab == WorldBuilderTab::Npcs && selected < (int)mNpcs.size())
			{
				mWorldBuilderSelectedNpc = selected;
				if (event.button.clicks >= 2)
				{
					int map = worldAreaIndex(mNpcs[selected].mapId);
					if (map >= 0)
					{
						mCurrentWorldArea = map;
						centerMapCamera(currentMap(), mNpcs[selected].x, mNpcs[selected].y,
							mWorldBuilderCameraX, mWorldBuilderCameraY, mWorldBuilderTileSize);
					}
				}
			}
			else if (mWorldBuilderTab == WorldBuilderTab::Objects &&
				selected < (int)(mWorldObjects.size() + mMercerStock.shards.size()))
			{
				mWorldBuilderSelectedObject = selected;
				if (event.button.clicks >= 2)
				{
					bool regularObject = selected < (int)mWorldObjects.size();
					int shardIndex = selected - (int)mWorldObjects.size();
					const std::string& mapId = regularObject ? mWorldObjects[selected].mapId :
						mMercerStock.shards[shardIndex].mapId;
					int objectX = regularObject ? mWorldObjects[selected].x :
						mMercerStock.shards[shardIndex].x;
					int objectY = regularObject ? mWorldObjects[selected].y :
						mMercerStock.shards[shardIndex].y;
					int map = worldAreaIndex(mapId);
					if (map >= 0)
					{
						mCurrentWorldArea = map;
						centerMapCamera(currentMap(), objectX, objectY, mWorldBuilderCameraX,
							mWorldBuilderCameraY, mWorldBuilderTileSize);
					}
				}
			}
			return;
		}
		int cellX, cellY;
		if (!mapCellAt(x, y, currentMap(), mWorldBuilderCameraX,
			mWorldBuilderCameraY, mWorldBuilderTileSize, cellX, cellY)) return;
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
			for (size_t i = 0; i < mWorldObjects.size(); ++i)
				if (mWorldObjects[i].mapId == currentMapId() &&
					mWorldObjects[i].x == cellX && mWorldObjects[i].y == cellY) hit = (int)i;
			for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
				if (mMercerStock.shards[i].mapId == currentMapId() &&
					mMercerStock.shards[i].x == cellX && mMercerStock.shards[i].y == cellY)
					hit = (int)mWorldObjects.size() + (int)i;
			if (hit >= 0) mWorldBuilderSelectedObject = hit;
			else placeWorldBuilderSelection(cellX, cellY);
		}
		mWorldBuilderDragging = true;
		return;
	}
	mWorldBuilderListScroll = 0;
}

void Application::drawWorldBuilderTileIcon(WorldTileId type, const SDL_Rect& rect)
{
	if (type == WorldTiles::Path) fillRect(rect, 162, 132, 76);
	else if (type == WorldTiles::OldRoadPath) fillRect(rect, 124, 112, 88);
	else if (type == WorldTiles::CinderrailPath) fillRect(rect, 116, 91, 58);
	else if (type == WorldTiles::WatershedPath) fillRect(rect, 148, 139, 105);
	else if (type == WorldTiles::GlasswaterPaving) fillRect(rect, 165, 199, 202);
	else if (type == WorldTiles::RootmazePath) fillRect(rect, 119, 134, 75);
	else if (type == WorldTiles::BlackstonePath) fillRect(rect, 111, 106, 96);
	else if (type == WorldTiles::Water || type == WorldTiles::TimberBridge)
		fillRect(rect, 25, 111, 157);
	else if (type == WorldTiles::House) fillRect(rect, 126, 65, 43);
	else if (type == WorldTiles::Forest || type == WorldTiles::Tree)
		fillRect(rect, 26, 75, 33);
	else if (type == WorldTiles::CinderrailRubble) fillRect(rect, 55, 45, 43);
	else if (type == WorldTiles::DuelSand) fillRect(rect, 188, 151, 87);
	else if (type == WorldTiles::CinderrailDuelSand) fillRect(rect, 118, 72, 48);
	else if (type == WorldTiles::Marble) fillRect(rect, 198, 204, 207);
	else if (type == WorldTiles::OldRoadWaystone) fillRect(rect, 79, 72, 65);
	else if (type == WorldTiles::MarbleRoof) fillRect(rect, 201, 196, 177);
	else if (type == WorldTiles::Bonfire) fillRect(rect, 83, 64, 42);
	else if (type == WorldTiles::FeastTable || type == WorldTiles::Grass)
		fillRect(rect, 61, 139, 61);
	else if (type == WorldTiles::WorkshopTools) fillRect(rect, 137, 91, 49);
	else if (type == WorldTiles::Rail || type == WorldTiles::RailCrossing)
		fillRect(rect, type == WorldTiles::RailCrossing ? 104 : 47,
			type == WorldTiles::RailCrossing ? 83 : 45,
			type == WorldTiles::RailCrossing ? 57 : 43);
	else if (type == WorldTiles::MetalGrate) fillRect(rect, 73, 80, 86);
	else if (type == WorldTiles::IndustrialBrick) fillRect(rect, 112, 49, 38);
	else if (type == WorldTiles::Machinery) fillRect(rect, 63, 66, 67);
	else if (type == WorldTiles::Furnace) fillRect(rect, 64, 47, 43);
	else if (type == WorldTiles::TimberRoof) fillRect(rect, 91, 48, 37);
	else if (type == WorldTiles::IndustrialRoof) fillRect(rect, 67, 68, 70);
	else if (type == WorldTiles::RockyCliff) fillRect(rect, 73, 65, 59);
	else if (type == WorldTiles::CinderrailGround) fillRect(rect, 82, 76, 59);
	else if (type == WorldTiles::WatershedGround || type == WorldTiles::WatershedMarker)
		fillRect(rect, 56, 124, 74);
	else if (type == WorldTiles::GlasswaterGround || type == WorldTiles::GlasswaterMarker)
		fillRect(rect, 91, 151, 153);
	else if (type == WorldTiles::GlasswaterRoof) fillRect(rect, 42, 91, 134);
	else if (type == WorldTiles::GlasswaterDock) fillRect(rect, 106, 76, 48);
	else if (type == WorldTiles::GlasswaterWall) fillRect(rect, 157, 190, 191);
	else if (type == WorldTiles::GlasswaterDoor) fillRect(rect, 57, 111, 137);
	else if (type == WorldTiles::GlasswaterArena) fillRect(rect, 104, 164, 188);
	else if (type == WorldTiles::RootmazeGround || type == WorldTiles::RootmazeMarker)
		fillRect(rect, 72, 126, 63);
	else if (type == WorldTiles::RootmazeRoot) fillRect(rect, 76, 55, 35);
	else if (type == WorldTiles::RootmazeBridge) fillRect(rect, 130, 92, 53);
	else if (type == WorldTiles::RootmazeRoof) fillRect(rect, 74, 116, 51);
	else if (type == WorldTiles::RootmazeWall) fillRect(rect, 109, 78, 47);
	else if (type == WorldTiles::RootmazeDoor) fillRect(rect, 126, 89, 50);
	else if (type == WorldTiles::RootmazeArena) fillRect(rect, 105, 154, 76);
	else if (type == WorldTiles::BlackstoneGround) fillRect(rect, 61, 59, 55);
	else if (type == WorldTiles::BlackstoneWall) fillRect(rect, 38, 37, 40);
	else if (type == WorldTiles::BlackstoneGate) fillRect(rect, 72, 67, 58);
	else if (type == WorldTiles::Rocks) fillRect(rect, 61, 139, 61);
	else if (type == WorldTiles::Bush) fillRect(rect, 49, 126, 54);
	else if (type == WorldTiles::Shrub) fillRect(rect, 61, 139, 61);
	else if (type == WorldTiles::CaveEntrance) fillRect(rect, 78, 70, 62);
	else if (type == WorldTiles::TreeStump) fillRect(rect, 61, 139, 61);
	else if (type == WorldTiles::WoodWall || type == WorldTiles::Door ||
		type == WorldTiles::CinderrailDoor || type == WorldTiles::WoodFloor ||
		type == WorldTiles::Counter)
		fillRect(rect, type == WorldTiles::WoodFloor ? 137 : 91,
			type == WorldTiles::WoodFloor ? 91 : 53,
			type == WorldTiles::WoodFloor ? 49 : 31);
	else fillRect(rect, 61, 139, 61);
	mWorldTileRenderer->drawTerrain(type, rect);

	const int x = rect.x;
	const int y = rect.y;
	const int w = rect.w;
	const int h = rect.h;
	if (type == WorldTiles::Grass)
	{
		fillRect({ x + 6, y + 18, 2, 8 }, 108, 184, 65);
		fillRect({ x + 14, y + 13, 2, 12 }, 119, 194, 70);
		fillRect({ x + 23, y + 19, 2, 7 }, 92, 172, 57);
	}
	else if (type == WorldTiles::Path || type == WorldTiles::OldRoadPath ||
		type == WorldTiles::BlackstonePath)
	{
		fillRect({ x + 3, y + 5, 11, 7 }, type == WorldTiles::Path ? 190 : 151,
			type == WorldTiles::Path ? 158 : 136, type == WorldTiles::Path ? 93 : 104);
		fillRect({ x + 16, y + 16, 11, 8 }, type == WorldTiles::Path ? 133 : 100,
			type == WorldTiles::Path ? 106 : 94, type == WorldTiles::Path ? 63 : 80);
	}
	else if (type == WorldTiles::Water)
	{
		fillRect({ x + 3, y + 8, w - 8, 3 }, 91, 188, 210);
		fillRect({ x + 8, y + 19, w - 11, 3 }, 63, 161, 194);
	}
	else if (type == WorldTiles::House)
	{
		fillRect({ x + 3, y + 4, w - 6, 8 }, 190, 78, 47);
		fillRect({ x + 7, y + 12, w - 14, h - 14 }, 150, 92, 53);
		fillRect({ x + 12, y + 17, 7, h - 19 }, 55, 32, 24);
	}
	else if (type == WorldTiles::Tree || type == WorldTiles::Forest)
	{
		fillRect({ x + 13, y + 14, 5, 13 }, 91, 51, 27);
		fillRect({ x + 5, y + 3, 20, 17 }, 43, 123, 51);
		if (type == WorldTiles::Forest)
		{
			fillRect({ x + 4, y + 13, 4, 12 }, 78, 44, 25);
			fillRect({ x + 1, y + 8, 13, 11 }, 30, 99, 40);
		}
	}
	else if (type == WorldTiles::WoodWall || type == WorldTiles::WoodFloor)
	{
		for (int line = 5; line < h; line += 7)
			fillRect({ x + 2, y + line, w - 4, 2 }, 75, 43, 28);
		if (type == WorldTiles::WoodWall)
		{
			fillRect({ x + 5, y + 1, 3, h - 2 }, 62, 37, 26);
			fillRect({ x + w - 8, y + 1, 3, h - 2 }, 62, 37, 26);
		}
	}
	else if (type == WorldTiles::Door || type == WorldTiles::CinderrailDoor)
	{
		fillRect({ x + 6, y + 2, w - 12, h - 2 }, type == WorldTiles::Door ? 138 : 107,
			type == WorldTiles::Door ? 78 : 111, type == WorldTiles::Door ? 39 : 112);
		fillRect({ x + w - 10, y + h / 2, 3, 3 }, 231, 184, 73);
	}
	else if (type == WorldTiles::Counter || type == WorldTiles::FeastTable)
	{
		fillRect({ x + 3, y + (type == WorldTiles::Counter ? 8 : 11), w - 6,
			type == WorldTiles::Counter ? 6 : 9 }, 166, 105, 53);
		fillRect({ x + 6, y + 20, 4, 7 }, 85, 48, 28);
		fillRect({ x + w - 10, y + 20, 4, 7 }, 85, 48, 28);
	}
	else if (type == WorldTiles::Bonfire || type == WorldTiles::Furnace)
	{
		if (type == WorldTiles::Furnace)
			fillRect({ x + 4, y + 3, w - 8, h - 5 }, 39, 32, 31);
		fillRect({ x + 9, y + 18, w - 18, 8 }, 225, 70, 28);
		fillRect({ x + 12, y + 9, w - 24, 15 }, 250, 142, 38);
		fillRect({ x + 14, y + 15, w - 28, 8 }, 255, 222, 89);
	}
	else if (type == WorldTiles::DuelSand || type == WorldTiles::CinderrailDuelSand)
	{
		fillRect({ x + 5, y + 7, 4, 3 }, 148, 107, 65);
		fillRect({ x + 20, y + 19, 5, 3 }, 220, 174, 103);
		if (type == WorldTiles::CinderrailDuelSand)
			outlineRect({ x + 2, y + 2, w - 4, h - 4 }, 225, 178, 71, 255, 2);
	}
	else if (type == WorldTiles::Marble)
	{
		fillRect({ x + w / 2, y + 1, 2, h - 2 }, 158, 171, 178);
		fillRect({ x + 1, y + h / 2, w - 2, 2 }, 158, 171, 178);
	}
	else if (type == WorldTiles::MarbleRoof || type == WorldTiles::TimberRoof ||
		type == WorldTiles::IndustrialRoof)
	{
		for (int course = 5; course < h; course += 7)
			fillRect({ x + 1, y + course, w - 2, 2 },
				type == WorldTiles::MarbleRoof ? 157 : 53,
				type == WorldTiles::MarbleRoof ? 164 : 39,
				type == WorldTiles::MarbleRoof ? 164 : 34);
		fillRect({ x, y + h - 5, w, 5 }, type == WorldTiles::MarbleRoof ? 174 : 44,
			type == WorldTiles::MarbleRoof ? 139 : 35,
			type == WorldTiles::MarbleRoof ? 67 : 33);
	}
	else if (type == WorldTiles::WorkshopTools || type == WorldTiles::Machinery)
	{
		fillRect({ x + 4, y + 18, w - 8, 7 }, 104, 59, 33);
		fillRect({ x + 7, y + 7, 5, 11 }, 54, 151, 193);
		fillRect({ x + 16, y + 4, 5, 14 }, 169, 178, 175);
		fillRect({ x + 21, y + 10, 5, 8 }, 214, 111, 48);
	}
	else if (type == WorldTiles::Rail || type == WorldTiles::RailCrossing)
	{
		for (int tie = 2; tie < w; tie += 7)
			fillRect({ x + tie, y + 3, 3, h - 6 }, 106, 75, 49);
		fillRect({ x, y + 6, w, 3 }, 165, 172, 173);
		fillRect({ x, y + h - 9, w, 3 }, 165, 172, 173);
		if (type == WorldTiles::RailCrossing)
		{
			fillRect({ x + 6, y, 3, h }, 190, 194, 191);
			fillRect({ x + w - 9, y, 3, h }, 190, 194, 191);
		}
	}
	else if (type == WorldTiles::MetalGrate)
	{
		for (int line = 5; line < w; line += 6)
		{
			fillRect({ x + line, y + 2, 2, h - 4 }, 42, 49, 53);
			fillRect({ x + 2, y + line, w - 4, 2 }, 42, 49, 53);
		}
	}
	else if (type == WorldTiles::IndustrialBrick)
	{
		for (int row = 6; row < h; row += 7)
			fillRect({ x + 1, y + row, w - 2, 2 }, 68, 38, 36);
		fillRect({ x + w / 2, y + 1, 2, 6 }, 71, 40, 37);
	}
	else if (type == WorldTiles::TimberBridge)
	{
		for (int plank = 3; plank < w; plank += 6)
			fillRect({ x + plank, y + 4, 5, h - 8 }, 132, 83, 45);
		fillRect({ x, y + 3, w, 3 }, 65, 45, 32);
		fillRect({ x, y + h - 6, w, 3 }, 65, 45, 32);
	}
	else if (type == WorldTiles::RockyCliff || type == WorldTiles::CinderrailRubble)
	{
		fillRect({ x + 3, y + 4, 12, 9 }, 112, 94, 75);
		fillRect({ x + 15, y + 12, 11, 10 }, 64, 58, 55);
		fillRect({ x + 5, y + 20, 14, 6 }, 88, 75, 65);
	}
	else if (type == WorldTiles::Rocks)
	{
		fillRect({ x + 4, y + 15, 12, 10 }, 96, 91, 82);
		fillRect({ x + 12, y + 8, 13, 15 }, 125, 117, 103);
		fillRect({ x + 15, y + 9, 7, 3 }, 163, 153, 134);
	}
	else if (type == WorldTiles::Bush || type == WorldTiles::Shrub)
	{
		int top = type == WorldTiles::Bush ? 6 : 14;
		int height = type == WorldTiles::Bush ? 19 : 11;
		fillRect({ x + 4, y + top + 5, 21, height - 5 },
			type == WorldTiles::Bush ? 34 : 52, type == WorldTiles::Bush ? 105 : 137, 48);
		fillRect({ x + 8, y + top, 10, 9 }, 71, 153, 65);
		fillRect({ x + 17, y + top + 3, 8, 8 }, 45, 124, 53);
	}
	else if (type == WorldTiles::CaveEntrance)
	{
		fillRect({ x + 3, y + 5, w - 6, h - 7 }, 112, 99, 82);
		fillRect({ x + 8, y + 10, w - 16, h - 10 }, 30, 31, 33);
		fillRect({ x + 5, y + 5, 7, 6 }, 151, 135, 108);
	}
	else if (type == WorldTiles::TreeStump)
	{
		fillRect({ x + 9, y + 13, 13, 13 }, 105, 65, 36);
		fillRect({ x + 7, y + 10, 17, 7 }, 155, 103, 53);
		fillRect({ x + 11, y + 12, 9, 3 }, 91, 57, 34);
		fillRect({ x + 4, y + 23, 8, 3 }, 75, 52, 31);
		fillRect({ x + 20, y + 22, 6, 3 }, 75, 52, 31);
	}
	else if (type == WorldTiles::OldRoadWaystone)
	{
		fillRect({ x + 8, y + 3, w - 16, h - 5 }, 127, 110, 84);
		fillRect({ x + 11, y + 9, w - 22, 3 }, 58, 83, 69);
		fillRect({ x + 13, y + 15, w - 26, 8 }, 62, 91, 75);
	}
	else if (type == WorldTiles::CinderrailGround)
	{
		fillRect({ x + 5, y + 20, 5, 3 }, 117, 94, 61);
		fillRect({ x + 20, y + 8, 3, 3 }, 54, 54, 51);
	}
	else if (type == WorldTiles::CinderrailPath)
		fillRect({ x, y + 4, w, 3 }, 190, 145, 55);
	else if (type == WorldTiles::WatershedGround)
	{
		fillRect({ x + 5, y + 19, 3, 7 }, 92, 170, 92);
		fillRect({ x + 19, y + 10, 3, 9 }, 72, 151, 86);
		fillRect({ x + 12, y + 23, 8, 2 }, 76, 142, 91);
	}
	else if (type == WorldTiles::WatershedPath)
	{
		fillRect({ x + 3, y + 5, 10, 6 }, 174, 163, 122);
		fillRect({ x + 17, y + 17, 9, 6 }, 118, 111, 88);
	}
	else if (type == WorldTiles::WatershedMarker)
	{
		fillRect({ x + 13, y + 7, 4, 19 }, 82, 54, 32);
		fillRect({ x + 5, y + 4, 20, 7 }, 203, 171, 69);
		fillRect({ x + 7, y + 6, 5, 3 }, 58, 133, 193);
		fillRect({ x + 13, y + 6, 5, 3 }, 66, 157, 79);
		fillRect({ x + 19, y + 6, 4, 3 }, 215, 169, 57);
	}
	else if (type == WorldTiles::GlasswaterGround)
	{
		fillRect({ x + 3, y + 13, w - 6, 2 }, 128, 181, 181);
		fillRect({ x + 10, y + 3, 2, 10 }, 116, 174, 176);
	}
	else if (type == WorldTiles::GlasswaterPaving)
	{
		fillRect({ x + 2, y + 2, w - 4, h - 4 }, 184, 211, 211);
		fillRect({ x + 2, y + h / 2, w - 4, 2 }, 102, 164, 177);
		fillRect({ x + w / 2, y + 2, 2, h - 4 }, 112, 174, 184);
	}
	else if (type == WorldTiles::GlasswaterRoof)
	{
		for (int wave = 5; wave < h; wave += 8)
			fillRect({ x + ((wave / 8) % 2) * 4, y + wave, w - 4, 4 }, 49, 117, 159);
		fillRect({ x, y + h - 4, w, 4 }, 111, 75, 143);
	}
	else if (type == WorldTiles::GlasswaterDock)
	{
		for (int plank = 3; plank < w; plank += 7)
			fillRect({ x + plank, y + 3, 2, h - 6 }, 68, 49, 38);
		fillRect({ x, y + 4, w, 3 }, 73, 156, 180);
		fillRect({ x, y + h - 7, w, 3 }, 73, 156, 180);
	}
	else if (type == WorldTiles::GlasswaterWall)
	{
		for (int course = 7; course < h; course += 8)
			fillRect({ x + 2, y + course, w - 4, 2 }, 98, 151, 163);
		fillRect({ x + 5, y + 10, 7, 8 }, 47, 118, 157);
		fillRect({ x + w - 12, y + 10, 7, 8 }, 47, 118, 157);
	}
	else if (type == WorldTiles::GlasswaterDoor)
	{
		fillRect({ x + 5, y + 2, w - 10, h - 2 }, 177, 207, 205);
		fillRect({ x + 9, y + 7, w - 18, h - 7 }, 49, 129, 158);
		fillRect({ x + w - 13, y + h / 2, 3, 3 }, 230, 199, 87);
	}
	else if (type == WorldTiles::GlasswaterArena)
	{
		outlineRect({ x + 3, y + 3, w - 6, h - 6 }, 220, 231, 222, 255, 2);
		fillRect({ x + w / 2 - 1, y + 5, 3, h - 10 }, 69, 119, 166);
		fillRect({ x + 5, y + h / 2 - 1, w - 10, 3 }, 111, 75, 143);
	}
	else if (type == WorldTiles::GlasswaterMarker)
	{
		fillRect({ x + w / 2 - 2, y + 7, 4, h - 9 }, 49, 74, 91);
		fillRect({ x + 6, y + 5, w - 12, 7 }, 211, 222, 205);
		fillRect({ x + 9, y + 7, 6, 3 }, 52, 142, 184);
		fillRect({ x + w - 15, y + 7, 6, 3 }, 82, 171, 132);
	}
	else if (type == WorldTiles::RootmazeGround)
	{
		fillRect({ x + 5, y + 18, 3, 8 }, 111, 174, 73);
		fillRect({ x + 19, y + 10, 3, 10 }, 91, 157, 66);
	}
	else if (type == WorldTiles::RootmazePath)
	{
		fillRect({ x + 3, y + 5, 11, 8 }, 151, 146, 91);
		fillRect({ x + 17, y + 17, 10, 7 }, 89, 103, 66);
	}
	else if (type == WorldTiles::RootmazeRoot)
	{
		fillRect({ x + 4, y + 2, 7, h - 4 }, 112, 76, 43);
		fillRect({ x + 16, y + 1, 5, h - 2 }, 63, 48, 34);
		fillRect({ x + 23, y + 8, 4, h - 10 }, 101, 72, 42);
	}
	else if (type == WorldTiles::RootmazeBridge)
	{
		for (int slat = 2; slat < w; slat += 7)
			fillRect({ x + slat, y + 4, 5, h - 8 }, 151, 108, 61);
		fillRect({ x, y + 3, w, 3 }, 61, 95, 47);
		fillRect({ x, y + h - 6, w, 3 }, 61, 95, 47);
	}
	else if (type == WorldTiles::RootmazeRoof)
	{
		for (int row = 6; row < h; row += 8)
			fillRect({ x + 1, y + row, w - 2, 2 }, 45, 86, 42);
		fillRect({ x + 5, y + 4, 8, 5 }, 121, 170, 73);
		fillRect({ x, y + h - 4, w, 4 }, 91, 58, 37);
	}
	else if (type == WorldTiles::RootmazeWall)
	{
		for (int beam = 5; beam < w; beam += 9)
			fillRect({ x + beam, y + 2, 4, h - 4 }, 70, 50, 35);
		fillRect({ x + 7, y + 10, 7, 8 }, 79, 139, 106);
		fillRect({ x + w - 14, y + 10, 7, 8 }, 79, 139, 106);
	}
	else if (type == WorldTiles::RootmazeDoor)
	{
		fillRect({ x + 5, y + 2, w - 10, h - 2 }, 82, 58, 39);
		fillRect({ x + 9, y + 7, w - 18, h - 7 }, 101, 142, 67);
		fillRect({ x + w - 13, y + h / 2, 3, 3 }, 229, 187, 76);
	}
	else if (type == WorldTiles::RootmazeArena)
	{
		outlineRect({ x + 3, y + 3, w - 6, h - 6 }, 213, 210, 136, 255, 2);
		fillRect({ x + 5, y + h / 2 - 1, w - 10, 3 }, 53, 111, 67);
		fillRect({ x + w / 2 - 1, y + 5, 3, h - 10 }, 78, 127, 60);
	}
	else if (type == WorldTiles::RootmazeMarker)
	{
		fillRect({ x + w / 2 - 2, y + 7, 4, h - 9 }, 91, 61, 37);
		fillRect({ x + 6, y + 5, w - 12, 8 }, 147, 126, 65);
		fillRect({ x + 9, y + 7, 5, 3 }, 62, 146, 77);
		fillRect({ x + w - 14, y + 7, 5, 3 }, 202, 151, 63);
	}
	else if (type == WorldTiles::BlackstoneGround)
	{
		fillRect({ x + 5, y + 8, 4, 3 }, 133, 126, 107);
		fillRect({ x + 19, y + 21, 6, 3 }, 42, 41, 39);
	}
	else if (type == WorldTiles::BlackstoneWall)
	{
		for (int course = 6; course < h; course += 8)
			fillRect({ x + 2, y + course, w - 4, 2 }, 20, 20, 22);
		fillRect({ x + w - 9, y + 5, 4, 4 }, 192, 149, 52);
	}
	else if (type == WorldTiles::BlackstoneGate)
	{
		fillRect({ x + 3, y + 2, 5, h - 4 }, 27, 27, 29);
		fillRect({ x + w - 8, y + 2, 5, h - 4 }, 27, 27, 29);
		for (int bar = 10; bar < w - 7; bar += 7)
			fillRect({ x + bar, y + 3, 3, h - 6 }, 202, 158, 57);
	}

	mWorldTileRenderer->drawDecorationTile(type, rect);
	outlineRect(rect, 8, 14, 22, 255, 1);
}

void Application::drawWorldBuilderScaledTileDetail(WorldTileId type, const SDL_Rect& rect)
{
	const int unit = std::max(1, rect.w / 12);
	const int inset = std::max(2, rect.w / 10);
	const int centerX = rect.x + rect.w / 2;
	const int centerY = rect.y + rect.h / 2;
	if (type == WorldTiles::Water)
	{
		fillRect({ rect.x + inset, rect.y + rect.h / 3, rect.w - inset * 2,
			unit }, 92, 189, 210, 210);
		fillRect({ rect.x + inset * 2, rect.y + rect.h * 2 / 3,
			std::max(unit, rect.w - inset * 3), unit }, 63, 161, 194, 210);
	}
	else if (type == WorldTiles::Path || type == WorldTiles::OldRoadPath ||
		type == WorldTiles::CinderrailPath || type == WorldTiles::WatershedPath ||
		type == WorldTiles::GlasswaterPaving || type == WorldTiles::RootmazePath ||
		type == WorldTiles::BlackstonePath)
	{
		fillRect({ rect.x + inset, rect.y + inset, rect.w / 3, rect.h / 4 },
			190, 165, 110, 170);
		fillRect({ centerX, centerY, rect.w / 3, rect.h / 4 }, 89, 94, 76, 130);
	}
	else if (type == WorldTiles::Tree || type == WorldTiles::Forest)
	{
		fillRect({ centerX - unit, centerY, unit * 2, rect.h / 2 - inset }, 85, 48, 26);
		fillRect({ rect.x + inset, rect.y + inset / 2, rect.w - inset * 2,
			rect.h * 2 / 3 }, 41, 116, 49);
	}
	else if (type == WorldTiles::Rocks)
	{
		fillRect({ rect.x + inset, centerY, rect.w / 2, rect.h / 3 }, 96, 91, 82);
		fillRect({ centerX - unit, rect.y + inset, rect.w / 2,
			rect.h / 2 }, 125, 117, 103);
	}
	else if (type == WorldTiles::Bush || type == WorldTiles::Shrub)
	{
		int top = type == WorldTiles::Bush ? rect.y + inset : centerY;
		fillRect({ rect.x + inset, top, rect.w - inset * 2,
			rect.y + rect.h - top - inset }, type == WorldTiles::Bush ? 34 : 52,
			type == WorldTiles::Bush ? 105 : 137, 48);
		fillRect({ centerX - unit * 2, top - unit, unit * 4, unit * 3 }, 71, 153, 65);
	}
	else if (type == WorldTiles::CaveEntrance)
	{
		fillRect({ rect.x + inset / 2, rect.y + inset, rect.w - inset,
			rect.h - inset }, 112, 99, 82);
		fillRect({ rect.x + inset * 2, rect.y + inset * 2,
			rect.w - inset * 4, rect.h - inset * 2 }, 30, 31, 33);
	}
	else if (type == WorldTiles::TreeStump)
	{
		fillRect({ centerX - unit * 2, centerY - unit,
			unit * 4, rect.h / 2 }, 105, 65, 36);
		fillRect({ centerX - unit * 3, centerY - unit * 2,
			unit * 6, unit * 2 }, 155, 103, 53);
	}
	else if (type == WorldTiles::TimberBridge || type == WorldTiles::GlasswaterDock ||
		type == WorldTiles::RootmazeBridge)
	{
		for (int slat = rect.x + unit; slat < rect.x + rect.w; slat += unit * 3)
			fillRect({ slat, rect.y + inset, unit * 2, rect.h - inset * 2 }, 132, 83, 45);
		fillRect({ rect.x, rect.y + inset, rect.w, unit }, 65, 70, 43);
		fillRect({ rect.x, rect.y + rect.h - inset - unit, rect.w, unit }, 65, 70, 43);
	}
	else if (type == WorldTiles::Rail || type == WorldTiles::RailCrossing)
	{
		for (int tie = rect.x + unit; tie < rect.x + rect.w; tie += unit * 3)
			fillRect({ tie, rect.y + inset, unit, rect.h - inset * 2 }, 106, 75, 49);
		fillRect({ rect.x, rect.y + rect.h / 4, rect.w, unit }, 165, 172, 173);
		fillRect({ rect.x, rect.y + rect.h * 3 / 4, rect.w, unit }, 165, 172, 173);
	}
	else if (type == WorldTiles::TimberRoof || type == WorldTiles::IndustrialRoof ||
		type == WorldTiles::MarbleRoof || type == WorldTiles::GlasswaterRoof ||
		type == WorldTiles::RootmazeRoof)
	{
		for (int course = rect.y + inset; course < rect.y + rect.h; course += unit * 3)
			fillRect({ rect.x + unit, course, rect.w - unit * 2, unit }, 50, 48, 42, 180);
		fillRect({ rect.x, rect.y + rect.h - unit * 2, rect.w, unit * 2 }, 55, 44, 38);
	}
	else if (type == WorldTiles::WoodWall || type == WorldTiles::IndustrialBrick ||
		type == WorldTiles::GlasswaterWall || type == WorldTiles::RootmazeWall ||
		type == WorldTiles::BlackstoneWall || type == WorldTiles::House)
	{
		outlineRect({ rect.x + inset, rect.y + inset, rect.w - inset * 2,
			rect.h - inset * 2 }, 66, 48, 39, 220, unit);
		fillRect({ rect.x + rect.w / 4, centerY - unit, rect.w / 6, unit * 3 },
			72, 132, 151);
		fillRect({ rect.x + rect.w * 3 / 5, centerY - unit, rect.w / 6, unit * 3 },
			72, 132, 151);
	}
	else if (type == WorldTiles::Door || type == WorldTiles::CinderrailDoor ||
		type == WorldTiles::GlasswaterDoor || type == WorldTiles::RootmazeDoor)
	{
		fillRect({ rect.x + rect.w / 4, rect.y + inset, rect.w / 2,
			rect.h - inset }, 74, 61, 43);
		fillRect({ rect.x + rect.w * 2 / 3, centerY, unit, unit }, 231, 184, 73);
	}
	else if (type == WorldTiles::BlackstoneGate)
	{
		fillRect({ rect.x + inset / 2, rect.y + inset / 2, unit * 2,
			rect.h - inset }, 27, 27, 29);
		fillRect({ rect.x + rect.w - inset / 2 - unit * 2, rect.y + inset / 2,
			unit * 2, rect.h - inset }, 27, 27, 29);
		for (int bar = rect.x + inset * 2; bar < rect.x + rect.w - inset; bar += unit * 3)
			fillRect({ bar, rect.y + inset, unit, rect.h - inset * 2 }, 202, 158, 57);
	}
	else if (type == WorldTiles::DuelSand || type == WorldTiles::CinderrailDuelSand ||
		type == WorldTiles::GlasswaterArena || type == WorldTiles::RootmazeArena)
	{
		outlineRect({ rect.x + inset, rect.y + inset, rect.w - inset * 2,
			rect.h - inset * 2 }, 225, 210, 132, 230, unit);
		fillRect({ centerX - unit / 2, rect.y + inset * 2, unit,
			rect.h - inset * 4 }, 93, 112, 95, 180);
	}
	else if (type == WorldTiles::RootmazeRoot)
	{
		fillRect({ rect.x + inset, rect.y + unit, unit * 2, rect.h - unit * 2 }, 112, 76, 43);
		fillRect({ centerX, rect.y, unit * 2, rect.h }, 63, 48, 34);
		fillRect({ rect.x + rect.w - inset - unit, rect.y + inset, unit,
			rect.h - inset }, 101, 72, 42);
	}
	else if (type == WorldTiles::OldRoadWaystone || type == WorldTiles::WatershedMarker ||
		type == WorldTiles::GlasswaterMarker || type == WorldTiles::RootmazeMarker)
	{
		fillRect({ centerX - unit, rect.y + inset, unit * 2, rect.h - inset * 2 }, 73, 61, 48);
		fillRect({ rect.x + inset, rect.y + inset, rect.w - inset * 2, unit * 3 },
			203, 171, 69);
	}
	else if (type == WorldTiles::Bonfire || type == WorldTiles::Furnace)
	{
		fillRect({ centerX - unit * 2, centerY, unit * 4, rect.h / 3 }, 225, 70, 28);
		fillRect({ centerX - unit, centerY - unit * 2, unit * 2, unit * 4 }, 255, 177, 48);
	}
	else if (type == WorldTiles::MetalGrate || type == WorldTiles::Marble)
	{
		fillRect({ centerX - unit / 2, rect.y + inset, unit, rect.h - inset * 2 }, 70, 81, 85, 160);
		fillRect({ rect.x + inset, centerY - unit / 2, rect.w - inset * 2, unit }, 70, 81, 85, 160);
	}
	else
	{
		fillRect({ rect.x + rect.w / 4, rect.y + rect.h * 2 / 3, unit, unit * 2 },
			105, 174, 72, 180);
		fillRect({ rect.x + rect.w * 2 / 3, rect.y + rect.h / 3, unit, unit * 2 },
			88, 155, 69, 180);
	}
}

void Application::drawWorldBuilderNpcPortrait(const Npc& npc, const SDL_Rect& rect)
{
	bool savedScaleActive = mWorldBuilderTileScaleActive;
	SDL_Rect savedScaleDestination = mWorldBuilderTileScaleDestination;
	mWorldBuilderTileScaleActive = true;
	mWorldBuilderTileScaleDestination = rect;
	drawCharacterSprite(rect.x, rect.y, npc.appearance, false, false,
		npc.facingX, npc.facingY);
	mWorldBuilderTileScaleActive = savedScaleActive;
	mWorldBuilderTileScaleDestination = savedScaleDestination;
	outlineRect(rect, 104, 123, 153, 255, 1);
}

int Application::worldBuilderHoveredNpc() const
{
	int cellX = -1;
	int cellY = -1;
	if (!mapCellAt(mMouseX, mMouseY, currentMap(), mWorldBuilderCameraX,
		mWorldBuilderCameraY, mWorldBuilderTileSize, cellX, cellY)) return -1;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].mapId == currentMapId() && mNpcs[i].x == cellX &&
			mNpcs[i].y == cellY) return (int)i;
	return -1;
}

void Application::renderWorldBuilder()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 12, 18, 29);
	drawText("WORLD BUILDER", 32, 13, color(244, 207, 103), 25);
	drawText(mWorldAreas[mCurrentWorldArea].name, 320, 17, color(189, 207, 232), 18, 360);
	drawText("ZOOM " + std::to_string(mWorldBuilderTileSize * 100 / TILE) + "%",
		690, 19, color(143, 189, 231), 14);
	drawText(mWorldBuilderDirty ? "UNSAVED CHANGES" : "SAVED", 810, 19,
		mWorldBuilderDirty ? color(244, 139, 88) : color(105, 218, 139), 14);
	const std::vector<std::string>& map = currentMap();
	const int tileSize = mWorldBuilderTileSize;
	const int viewportColumns = builderViewportColumns(tileSize);
	const int viewportRows = builderViewportRows(tileSize);
	int mapX = builderMapOriginX((int)map[0].size(), tileSize) -
		mWorldBuilderCameraX * tileSize;
	int mapY = builderMapOriginY((int)map.size(), tileSize) -
		mWorldBuilderCameraY * tileSize;
	TileBounds visibleTiles = visibleTileBounds((float)mWorldBuilderCameraX,
		(float)mWorldBuilderCameraY, (int)map[0].size(), (int)map.size(),
		viewportColumns, viewportRows);
	SDL_Rect mapViewport = { MAP_X, MAP_Y, MAP_VIEW_WIDTH, MAP_VIEW_HEIGHT };
	SDL_RenderSetClipRect(mRenderer, &mapViewport);
	for (int row = visibleTiles.top; row < visibleTiles.bottom; ++row)
	{
		for (int column = visibleTiles.left; column < visibleTiles.right; ++column)
		{
			SDL_Rect tile = { mapX + (int)column * tileSize,
				mapY + (int)row * tileSize, tileSize, tileSize };
			WorldTileId type = WorldTiles::fromGlyph(map[row][column]);
			if (type == WorldTiles::Path) fillRect(tile, 162, 132, 76);
			else if (type == WorldTiles::OldRoadPath) fillRect(tile, 124, 112, 88);
			else if (type == WorldTiles::CinderrailPath) fillRect(tile, 116, 91, 58);
			else if (type == WorldTiles::WatershedPath) fillRect(tile, 148, 139, 105);
			else if (type == WorldTiles::GlasswaterPaving) fillRect(tile, 165, 199, 202);
			else if (type == WorldTiles::RootmazePath) fillRect(tile, 119, 134, 75);
			else if (type == WorldTiles::BlackstonePath) fillRect(tile, 111, 106, 96);
			else if (type == WorldTiles::Water) fillRect(tile, 25, 111, 157);
			else if (type == WorldTiles::House) fillRect(tile, 126, 65, 43);
			else if (type == WorldTiles::Forest || type == WorldTiles::Tree)
				fillRect(tile, 26, 75, 33);
			else if (type == WorldTiles::CinderrailRubble) fillRect(tile, 55, 45, 43);
			else if (type == WorldTiles::DuelSand) fillRect(tile, 188, 151, 87);
			else if (type == WorldTiles::CinderrailDuelSand) fillRect(tile, 118, 72, 48);
			else if (type == WorldTiles::Marble) fillRect(tile, 198, 204, 207);
			else if (type == WorldTiles::OldRoadWaystone) fillRect(tile, 79, 72, 65);
			else if (type == WorldTiles::MarbleRoof) fillRect(tile, 201, 196, 177);
			else if (type == WorldTiles::Bonfire) fillRect(tile, 83, 64, 42);
			else if (type == WorldTiles::FeastTable) fillRect(tile, 61, 139, 61);
			else if (type == WorldTiles::WorkshopTools) fillRect(tile, 137, 91, 49);
			else if (type == WorldTiles::Rail || type == WorldTiles::RailCrossing)
				fillRect(tile, type == WorldTiles::RailCrossing ? 104 : 47,
					type == WorldTiles::RailCrossing ? 83 : 45,
					type == WorldTiles::RailCrossing ? 57 : 43);
			else if (type == WorldTiles::MetalGrate) fillRect(tile, 73, 80, 86);
			else if (type == WorldTiles::IndustrialBrick) fillRect(tile, 112, 49, 38);
			else if (type == WorldTiles::Machinery) fillRect(tile, 63, 66, 67);
			else if (type == WorldTiles::Furnace) fillRect(tile, 64, 47, 43);
			else if (type == WorldTiles::TimberRoof) fillRect(tile, 91, 48, 37);
			else if (type == WorldTiles::IndustrialRoof) fillRect(tile, 67, 68, 70);
			else if (type == WorldTiles::TimberBridge) fillRect(tile, 31, 99, 132);
			else if (type == WorldTiles::RockyCliff) fillRect(tile, 73, 65, 59);
			else if (type == WorldTiles::WoodWall || type == WorldTiles::Door ||
				type == WorldTiles::CinderrailDoor || type == WorldTiles::WoodFloor ||
				type == WorldTiles::Counter)
				fillRect(tile, type == WorldTiles::WoodFloor ? 137 : 91,
					type == WorldTiles::WoodFloor ? 91 : 53,
					type == WorldTiles::WoodFloor ? 49 : 31);
			else if (type == WorldTiles::CinderrailGround) fillRect(tile, 82, 76, 59);
			else if (type == WorldTiles::WatershedGround ||
				type == WorldTiles::WatershedMarker) fillRect(tile, 56, 124, 74);
			else if (type == WorldTiles::GlasswaterGround ||
				type == WorldTiles::GlasswaterMarker) fillRect(tile, 91, 151, 153);
			else if (type == WorldTiles::GlasswaterRoof) fillRect(tile, 42, 91, 134);
			else if (type == WorldTiles::GlasswaterDock) fillRect(tile, 106, 76, 48);
			else if (type == WorldTiles::GlasswaterWall) fillRect(tile, 157, 190, 191);
			else if (type == WorldTiles::GlasswaterDoor) fillRect(tile, 57, 111, 137);
			else if (type == WorldTiles::GlasswaterArena) fillRect(tile, 104, 164, 188);
			else if (type == WorldTiles::RootmazeGround ||
				type == WorldTiles::RootmazeMarker) fillRect(tile, 72, 126, 63);
			else if (type == WorldTiles::RootmazeRoot) fillRect(tile, 76, 55, 35);
			else if (type == WorldTiles::RootmazeBridge) fillRect(tile, 130, 92, 53);
			else if (type == WorldTiles::RootmazeRoof) fillRect(tile, 74, 116, 51);
			else if (type == WorldTiles::RootmazeWall) fillRect(tile, 109, 78, 47);
			else if (type == WorldTiles::RootmazeDoor) fillRect(tile, 126, 89, 50);
			else if (type == WorldTiles::RootmazeArena) fillRect(tile, 105, 154, 76);
			else if (type == WorldTiles::BlackstoneGround) fillRect(tile, 61, 59, 55);
			else if (type == WorldTiles::BlackstoneWall) fillRect(tile, 38, 37, 40);
			else if (type == WorldTiles::BlackstoneGate) fillRect(tile, 72, 67, 58);
			else if (type == WorldTiles::Rocks) fillRect(tile, 61, 139, 61);
			else if (type == WorldTiles::Bush) fillRect(tile, 49, 126, 54);
			else if (type == WorldTiles::Shrub) fillRect(tile, 61, 139, 61);
			else if (type == WorldTiles::CaveEntrance) fillRect(tile, 78, 70, 62);
			else if (type == WorldTiles::TreeStump) fillRect(tile, 61, 139, 61);
			else fillRect(tile, 61, 139, 61);
			mWorldTileRenderer->drawTerrain(type, tile);
			SDL_Rect displayedTile = tile;
			mWorldBuilderTileScaleActive = tileSize != TILE;
			mWorldBuilderTileScaleDestination = displayedTile;
			if (mWorldBuilderTileScaleActive)
			{
				tile.w = TILE;
				tile.h = TILE;
			}
			{
			if (type == WorldTiles::Water)
				fillRect({ tile.x + 7, tile.y + 17, 28, 3 }, 92, 189, 210, 190);
			else if (type == WorldTiles::CinderrailRubble)
			{
				fillRect({ tile.x + 3, tile.y + 5, 42, 38 }, 72, 58, 54);
				fillRect({ tile.x + 7, tile.y + 11, 17, 4 }, 91, 70, 61);
				fillRect({ tile.x + 27, tile.y + 25, 14, 4 }, 39, 35, 37);
			}
			else if (type == WorldTiles::Forest || type == WorldTiles::Tree)
			{
				fillRect({ tile.x + 19, tile.y + 27, 10, 18 }, 85, 48, 26);
				fillRect({ tile.x + 6, tile.y + 5, 36, 30 }, 41, 116, 49);
			}
			else if (type == WorldTiles::House)
			{
				fillRect({ tile.x + 3, tile.y + 4, 42, 16 }, 186, 76, 46);
				fillRect({ tile.x + 18, tile.y + 23, 13, 25 }, 54, 31, 24);
			}
			else if (type == WorldTiles::TimberBridge)
			{
				fillRect({ tile.x, tile.y + 6, 48, 36 }, 31, 99, 132);
				for (int plank = 0; plank < 48; plank += 9)
					fillRect({ tile.x + plank, tile.y + 8, 8, 32 }, 129, 82, 45);
				fillRect({ tile.x, tile.y + 5, 48, 5 }, 66, 45, 32);
				fillRect({ tile.x, tile.y + 39, 48, 5 }, 66, 45, 32);
			}
			else if (type == WorldTiles::RockyCliff)
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 91, 80, 69);
				fillRect({ tile.x + 5, tile.y + 8, 19, 8 }, 116, 99, 78);
				fillRect({ tile.x + 27, tile.y + 18, 15, 9 }, 58, 55, 54);
				fillRect({ tile.x + 10, tile.y + 27, 24, 4 }, 66, 60, 56);
			}
			else if (type == WorldTiles::Rocks)
			{
				fillRect({ tile.x + 5, tile.y + 26, 21, 16 }, 96, 91, 82);
				fillRect({ tile.x + 17, tile.y + 13, 24, 27 }, 125, 117, 103);
				fillRect({ tile.x + 21, tile.y + 16, 14, 5 }, 163, 153, 134);
			}
			else if (type == WorldTiles::Bush || type == WorldTiles::Shrub)
			{
				int top = type == WorldTiles::Bush ? 9 : 24;
				int height = type == WorldTiles::Bush ? 34 : 18;
				fillRect({ tile.x + 5, tile.y + top + 7, 38, height - 7 },
					type == WorldTiles::Bush ? 34 : 52,
					type == WorldTiles::Bush ? 105 : 137, 48);
				fillRect({ tile.x + 10, tile.y + top, 18, 15 }, 71, 153, 65);
				fillRect({ tile.x + 25, tile.y + top + 5, 15, 14 }, 45, 124, 53);
			}
			else if (type == WorldTiles::CaveEntrance)
			{
				fillRect({ tile.x + 3, tile.y + 7, 42, 41 }, 112, 99, 82);
				fillRect({ tile.x + 10, tile.y + 15, 28, 33 }, 30, 31, 33);
				fillRect({ tile.x + 6, tile.y + 8, 13, 8 }, 151, 135, 108);
				fillRect({ tile.x + 15, tile.y + 41, 18, 4 }, 49, 44, 40);
			}
			else if (type == WorldTiles::TreeStump)
			{
				fillRect({ tile.x + 15, tile.y + 21, 20, 21 }, 105, 65, 36);
				fillRect({ tile.x + 11, tile.y + 16, 28, 11 }, 155, 103, 53);
				fillRect({ tile.x + 17, tile.y + 19, 16, 5 }, 91, 57, 34);
				fillRect({ tile.x + 7, tile.y + 39, 13, 5 }, 75, 52, 31);
				fillRect({ tile.x + 31, tile.y + 38, 11, 5 }, 75, 52, 31);
			}
			else if (type == WorldTiles::TimberRoof)
			{
				fillRect({ tile.x, tile.y + 3, 48, 42 }, 111, 56, 40);
				fillRect({ tile.x, tile.y + 2, 48, 7 }, 63, 39, 34);
				for (int shingle = 0; shingle < 4; ++shingle)
				{
					int shingleY = tile.y + 11 + shingle * 9;
					fillRect({ tile.x, shingleY, 48, 3 }, 67, 38, 33);
				}
				fillRect({ tile.x, tile.y + 42, 48, 5 }, 50, 34, 31);
			}
			else if (type == WorldTiles::MarbleRoof)
			{
				fillRect({ tile.x + 1, tile.y + 3, 46, 42 }, 216, 213, 199);
				fillRect({ tile.x + 1, tile.y + 3, 46, 6 }, 244, 241, 222);
				for (int course = 0; course < 4; ++course)
					fillRect({ tile.x + 2, tile.y + 11 + course * 9, 44, 2 }, 157, 164, 164);
				fillRect({ tile.x, tile.y + 42, 48, 5 }, 174, 139, 67);
			}
			else if (type == WorldTiles::WoodWall)
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 124, 72, 38);
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tile.x + 3, tile.y + 8 + plank * 10, 42, 2 }, 76, 42, 27);
				fillRect({ tile.x + 5, tile.y + 2, 5, 46 }, 67, 38, 26);
				fillRect({ tile.x + 38, tile.y + 2, 5, 46 }, 67, 38, 26);
			}
			else if (type == WorldTiles::Door || type == WorldTiles::CinderrailDoor)
			{
				bool cinderDoor = type == WorldTiles::CinderrailDoor;
				fillRect({ tile.x + 7, tile.y + 2, 34, 46 },
					cinderDoor ? 75 : 104, cinderDoor ? 71 : 57, cinderDoor ? 68 : 31);
				fillRect({ tile.x + 11, tile.y + 6, 26, 38 },
					cinderDoor ? 107 : 139, cinderDoor ? 111 : 78, cinderDoor ? 112 : 39);
				fillRect({ tile.x + 30, tile.y + 24, 4, 4 }, 231, 184, 73);
			}
			else if (type == WorldTiles::WoodFloor)
			{
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tile.x, tile.y + plank * 12, 48, 2 }, 96, 58, 36);
			}
			else if (type == WorldTiles::Counter)
			{
				fillRect({ tile.x + 2, tile.y + 10, 44, 32 }, 112, 62, 34);
				fillRect({ tile.x, tile.y + 7, 48, 7 }, 166, 105, 53);
			}
			else if (type == WorldTiles::Bonfire)
			{
				fillRect({ tile.x + 5, tile.y + 32, 38, 8 }, 74, 70, 65);
				fillRect({ tile.x + 10, tile.y + 29, 28, 10 }, 111, 100, 83);
				fillRect({ tile.x + 17, tile.y + 13, 15, 22 }, 221, 69, 32);
				fillRect({ tile.x + 20, tile.y + 8, 10, 23 }, 249, 137, 36);
				fillRect({ tile.x + 23, tile.y + 16, 6, 15 }, 255, 222, 89);
			}
			else if (type == WorldTiles::FeastTable)
			{
				fillRect({ tile.x + 6, tile.y + 6, 36, 5 }, 93, 52, 29);
				fillRect({ tile.x + 6, tile.y + 37, 36, 5 }, 93, 52, 29);
				fillRect({ tile.x + 4, tile.y + 15, 40, 18 }, 139, 82, 39);
				fillRect({ tile.x + 12, tile.y + 23, 7, 6 }, 232, 208, 151);
				fillRect({ tile.x + 29, tile.y + 22, 8, 7 }, 175, 49, 37);
			}
			else if (type == WorldTiles::DuelSand || type == WorldTiles::CinderrailDuelSand)
			{
				if (type == WorldTiles::CinderrailDuelSand)
				{
					fillRect({ tile.x + 3, tile.y + 3, 42, 42 }, 132, 76, 48);
					fillRect({ tile.x + 3, tile.y + 5, 42, 3 }, 225, 178, 71);
				}
				else
				{
					fillRect({ tile.x + 7, tile.y + 10, 5, 3 }, 157, 121, 69);
					fillRect({ tile.x + 34, tile.y + 31, 6, 3 }, 211, 177, 109);
				}
			}
			else if (type == WorldTiles::Marble || type == WorldTiles::OldRoadWaystone)
			{
				if (type == WorldTiles::OldRoadWaystone)
				{
					fillRect({ tile.x + 11, tile.y + 5, 27, 39 }, 102, 91, 76);
					fillRect({ tile.x + 14, tile.y + 8, 21, 31 }, 127, 110, 84);
					fillRect({ tile.x + 17, tile.y + 13, 15, 4 }, 58, 83, 69);
					fillRect({ tile.x + 21, tile.y + 20, 7, 12 }, 62, 91, 75);
				}
				else
				{
					fillRect({ tile.x + 2, tile.y + 2, 44, 44 }, 220, 224, 224);
					fillRect({ tile.x + 3, tile.y + 22, 42, 2 }, 162, 173, 179);
					fillRect({ tile.x + 17, tile.y + 3, 2, 19 }, 177, 186, 190);
					fillRect({ tile.x + 31, tile.y + 24, 2, 21 }, 177, 186, 190);
				}
			}
			else if (type == WorldTiles::WorkshopTools)
			{
				fillRect({ tile.x + 3, tile.y + 24, 42, 17 }, 104, 58, 32);
				fillRect({ tile.x + 5, tile.y + 20, 38, 6 }, 171, 108, 51);
				fillRect({ tile.x + 9, tile.y + 10, 7, 11 }, 54, 151, 193);
				fillRect({ tile.x + 20, tile.y + 7, 6, 14 }, 151, 71, 183);
				fillRect({ tile.x + 31, tile.y + 12, 8, 9 }, 217, 158, 48);
			}
			else if (type == WorldTiles::Rail || type == WorldTiles::RailCrossing)
			{
				for (int tie = 2; tie < 48; tie += 10)
					fillRect({ tile.x + tie, tile.y + 5, 5, 38 },
						type == WorldTiles::RailCrossing ? 155 : 100,
						type == WorldTiles::RailCrossing ? 112 : 73,
						type == WorldTiles::RailCrossing ? 55 : 48);
				fillRect({ tile.x, tile.y + 9, 48, 5 }, 151, 158, 158);
				fillRect({ tile.x, tile.y + 34, 48, 5 }, 151, 158, 158);
			}
			else if (type == WorldTiles::MetalGrate)
			{
				fillRect({ tile.x + 2, tile.y + 2, 44, 44 }, 88, 96, 101);
				for (int grate = 7; grate < 44; grate += 9)
					fillRect({ tile.x + grate, tile.y + 4, 3, 40 }, 45, 50, 54);
				fillRect({ tile.x + 2, tile.y + 3, 44, 3 }, 222, 168, 57);
			}
			else if (type == WorldTiles::IndustrialRoof)
			{
				fillRect({ tile.x + 1, tile.y + 3, 46, 42 }, 75, 76, 77);
				fillRect({ tile.x, tile.y + 40, 48, 7 }, 43, 45, 47);
				for (int panel = 0; panel < 48; panel += 16)
				{
					fillRect({ tile.x + panel, tile.y + 3, 3, 38 }, 43, 47, 49);
					fillRect({ tile.x + panel + 3, tile.y + 7, 12, 4 }, 142, 77, 51);
					fillRect({ tile.x + panel + 6, tile.y + 11, 9, 4 }, 125, 72, 51);
				}
				fillRect({ tile.x + 4, tile.y + 24, 40, 7 }, 155, 170, 171);
				fillRect({ tile.x + 7, tile.y + 26, 34, 3 }, 104, 189, 202);
			}
			else if (type == WorldTiles::IndustrialBrick)
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 135, 55, 40);
				for (int brick = 10; brick < 43; brick += 11)
					fillRect({ tile.x + 3, tile.y + brick, 42, 2 }, 73, 39, 37);
			}
			else if (type == WorldTiles::Machinery)
			{
				fillRect({ tile.x + 5, tile.y + 6, 38, 37 }, 82, 88, 89);
				fillRect({ tile.x + 8, tile.y + 10, 32, 6 }, 176, 75, 43);
				fillRect({ tile.x + 21, tile.y + 16, 8, 23 }, 169, 178, 175);
				fillRect({ tile.x + 14, tile.y + 23, 22, 8 }, 169, 178, 175);
			}
			else if (type == WorldTiles::Furnace)
			{
				fillRect({ tile.x + 5, tile.y + 3, 38, 43 }, 69, 55, 52);
				fillRect({ tile.x + 11, tile.y + 20, 26, 22 }, 34, 29, 29);
				fillRect({ tile.x + 14, tile.y + 28, 20, 12 }, 235, 71, 29);
				fillRect({ tile.x + 19, tile.y + 24, 10, 15 }, 255, 177, 48);
			}
			else if (type == WorldTiles::CinderrailGround)
			{
				fillRect({ tile.x + 7, tile.y + 35, 5, 3 }, 117, 94, 61);
				fillRect({ tile.x + 32, tile.y + 13, 3, 3 }, 58, 57, 52);
			}
			if (type == WorldTiles::OldRoadPath)
			{
				fillRect({ tile.x + 2, tile.y + 5, 18, 15 }, 143, 130, 101);
				fillRect({ tile.x + 24, tile.y + 7, 21, 13 }, 105, 98, 82);
				fillRect({ tile.x + 6, tile.y + 25, 25, 14 }, 104, 98, 82);
				fillRect({ tile.x + 34, tile.y + 26, 12, 12 }, 148, 131, 95);
			}
			else if (type == WorldTiles::CinderrailPath)
				fillRect({ tile.x, tile.y + 4, 48, 3 }, 190, 145, 55);
			else if (type == WorldTiles::WatershedGround)
			{
				fillRect({ tile.x + 7, tile.y + 34, 3, 8 }, 99, 181, 96);
				fillRect({ tile.x + 31, tile.y + 16, 3, 10 }, 77, 158, 88);
				fillRect({ tile.x + 18, tile.y + 39, 12, 3 }, 74, 143, 91);
			}
			else if (type == WorldTiles::WatershedPath)
			{
				fillRect({ tile.x + 4, tile.y + 7, 18, 12 }, 174, 163, 122);
				fillRect({ tile.x + 27, tile.y + 28, 15, 10 }, 118, 111, 88);
			}
			else if (type == WorldTiles::WatershedMarker)
			{
				fillRect({ tile.x + 21, tile.y + 13, 6, 31 }, 82, 54, 32);
				fillRect({ tile.x + 7, tile.y + 8, 34, 12 }, 203, 171, 69);
				fillRect({ tile.x + 10, tile.y + 12, 8, 4 }, 58, 133, 193);
				fillRect({ tile.x + 20, tile.y + 12, 8, 4 }, 66, 157, 79);
				fillRect({ tile.x + 30, tile.y + 12, 7, 4 }, 215, 169, 57);
			}
			else if (type == WorldTiles::GlasswaterGround)
			{
				fillRect({ tile.x + 3, tile.y + 22, 42, 2 }, 128, 181, 181);
				fillRect({ tile.x + 17, tile.y + 3, 2, 19 }, 116, 174, 176);
				fillRect({ tile.x + 32, tile.y + 25, 2, 20 }, 105, 164, 168);
			}
			else if (type == WorldTiles::GlasswaterPaving)
			{
				fillRect({ tile.x + 2, tile.y + 2, 44, 44 }, 184, 211, 211);
				fillRect({ tile.x + 3, tile.y + 22, 42, 2 }, 102, 164, 177);
				fillRect({ tile.x + 22, tile.y + 3, 2, 42 }, 112, 174, 184);
			}
			else if (type == WorldTiles::GlasswaterRoof)
			{
				for (int wave = 0; wave < 4; ++wave)
				{
					int waveY = tile.y + 7 + wave * 10;
					fillRect({ tile.x + (wave % 2 == 0 ? 0 : 7), waveY, 41, 5 },
						49, 117, 159);
					fillRect({ tile.x + (wave % 2 == 0 ? 8 : 0), waveY + 4, 40, 3 },
						27, 70, 116);
				}
				fillRect({ tile.x, tile.y + 43, 48, 4 }, 111, 75, 143);
			}
			else if (type == WorldTiles::GlasswaterDock)
			{
				for (int plank = 3; plank < 48; plank += 9)
					fillRect({ tile.x + plank, tile.y + 3, 3, 42 }, 68, 49, 38);
				fillRect({ tile.x, tile.y + 5, 48, 4 }, 73, 156, 180);
				fillRect({ tile.x, tile.y + 39, 48, 4 }, 73, 156, 180);
			}
			else if (type == WorldTiles::GlasswaterWall)
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 174, 207, 207);
				for (int course = 10; course < 43; course += 11)
					fillRect({ tile.x + 3, tile.y + course, 42, 2 }, 98, 151, 163);
				fillRect({ tile.x + 7, tile.y + 14, 12, 13 }, 47, 118, 157);
				fillRect({ tile.x + 29, tile.y + 14, 12, 13 }, 47, 118, 157);
			}
			else if (type == WorldTiles::GlasswaterDoor)
			{
				fillRect({ tile.x + 5, tile.y + 2, 38, 46 }, 177, 207, 205);
				fillRect({ tile.x + 10, tile.y + 8, 28, 40 }, 39, 98, 133);
				fillRect({ tile.x + 14, tile.y + 12, 20, 32 }, 49, 129, 158);
				fillRect({ tile.x + 29, tile.y + 27, 4, 4 }, 230, 199, 87);
			}
			else if (type == WorldTiles::GlasswaterArena)
			{
				outlineRect({ tile.x + 4, tile.y + 4, 40, 40 }, 220, 231, 222, 255, 3);
				fillRect({ tile.x + 22, tile.y + 6, 4, 36 }, 69, 119, 166);
				fillRect({ tile.x + 6, tile.y + 22, 36, 4 }, 111, 75, 143);
			}
			else if (type == WorldTiles::GlasswaterMarker)
			{
				fillRect({ tile.x + 21, tile.y + 9, 6, 34 }, 49, 74, 91);
				fillRect({ tile.x + 10, tile.y + 7, 28, 9 }, 211, 222, 205);
				fillRect({ tile.x + 13, tile.y + 10, 9, 3 }, 52, 142, 184);
				fillRect({ tile.x + 26, tile.y + 10, 8, 3 }, 82, 171, 132);
			}
			else if (type == WorldTiles::RootmazeGround)
			{
				fillRect({ tile.x + 7, tile.y + 33, 4, 10 }, 111, 174, 73);
				fillRect({ tile.x + 29, tile.y + 19, 3, 13 }, 91, 157, 66);
				fillRect({ tile.x + 18, tile.y + 39, 12, 3 }, 52, 105, 52);
			}
			else if (type == WorldTiles::RootmazePath)
			{
				fillRect({ tile.x + 3, tile.y + 7, 19, 12 }, 151, 146, 91);
				fillRect({ tile.x + 26, tile.y + 27, 18, 11 }, 89, 103, 66);
			}
			else if (type == WorldTiles::RootmazeRoot)
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 87, 62, 38);
				fillRect({ tile.x + 6, tile.y + 7, 9, 36 }, 112, 76, 43);
				fillRect({ tile.x + 24, tile.y + 2, 7, 43 }, 63, 48, 34);
				fillRect({ tile.x + 35, tile.y + 12, 7, 31 }, 101, 72, 42);
			}
			else if (type == WorldTiles::RootmazeBridge)
			{
				for (int slat = 2; slat < 48; slat += 8)
					fillRect({ tile.x + slat, tile.y + 5, 6, 38 }, 151, 108, 61);
				fillRect({ tile.x, tile.y + 4, 48, 5 }, 61, 95, 47);
				fillRect({ tile.x, tile.y + 39, 48, 5 }, 61, 95, 47);
			}
			else if (type == WorldTiles::RootmazeRoof)
			{
				fillRect({ tile.x + 1, tile.y + 3, 46, 42 }, 78, 124, 54);
				for (int row = 8; row < 42; row += 9)
					fillRect({ tile.x + 2, tile.y + row, 44, 3 }, 45, 86, 42);
				fillRect({ tile.x + 7, tile.y + 5, 9, 6 }, 121, 170, 73);
				fillRect({ tile.x, tile.y + 42, 48, 5 }, 91, 58, 37);
			}
			else if (type == WorldTiles::RootmazeWall)
			{
				fillRect({ tile.x + 2, tile.y + 3, 44, 42 }, 122, 87, 52);
				for (int beam = 7; beam < 45; beam += 12)
					fillRect({ tile.x + beam, tile.y + 3, 5, 42 }, 70, 50, 35);
				fillRect({ tile.x + 10, tile.y + 13, 12, 13 }, 79, 139, 106);
				fillRect({ tile.x + 29, tile.y + 13, 10, 13 }, 79, 139, 106);
			}
			else if (type == WorldTiles::RootmazeDoor)
			{
				fillRect({ tile.x + 5, tile.y + 2, 38, 46 }, 82, 58, 39);
				fillRect({ tile.x + 10, tile.y + 8, 28, 40 }, 138, 99, 55);
				fillRect({ tile.x + 14, tile.y + 13, 20, 31 }, 101, 142, 67);
				fillRect({ tile.x + 29, tile.y + 27, 4, 4 }, 229, 187, 76);
			}
			else if (type == WorldTiles::RootmazeArena)
			{
				outlineRect({ tile.x + 4, tile.y + 4, 40, 40 }, 213, 210, 136, 255, 3);
				fillRect({ tile.x + 7, tile.y + 22, 34, 4 }, 53, 111, 67);
				fillRect({ tile.x + 22, tile.y + 7, 4, 34 }, 78, 127, 60);
			}
			else if (type == WorldTiles::RootmazeMarker)
			{
				fillRect({ tile.x + 21, tile.y + 8, 6, 35 }, 91, 61, 37);
				fillRect({ tile.x + 7, tile.y + 7, 34, 11 }, 147, 126, 65);
				fillRect({ tile.x + 10, tile.y + 10, 7, 4 }, 62, 146, 77);
				fillRect({ tile.x + 30, tile.y + 10, 7, 4 }, 202, 151, 63);
			}
			else if (type == WorldTiles::BlackstoneGround)
			{
				fillRect({ tile.x + 7, tile.y + 11, 6, 4 }, 133, 126, 107);
				fillRect({ tile.x + 31, tile.y + 34, 8, 4 }, 42, 41, 39);
			}
			else if (type == WorldTiles::BlackstonePath)
			{
				fillRect({ tile.x + 3, tile.y + 6, 19, 13 }, 142, 136, 121);
				fillRect({ tile.x + 27, tile.y + 28, 17, 11 }, 77, 75, 70);
			}
			else if (type == WorldTiles::BlackstoneWall)
			{
				for (int course = 9; course < 45; course += 11)
					fillRect({ tile.x + 2, tile.y + course, 44, 3 }, 20, 20, 22);
				fillRect({ tile.x + 35, tile.y + 7, 6, 6 }, 192, 149, 52);
			}
			else if (type == WorldTiles::BlackstoneGate)
			{
				fillRect({ tile.x + 3, tile.y + 2, 7, 44 }, 27, 27, 29);
				fillRect({ tile.x + 38, tile.y + 2, 7, 44 }, 27, 27, 29);
				for (int bar = 13; bar < 38; bar += 8)
					fillRect({ tile.x + bar, tile.y + 4, 4, 40 }, 202, 158, 57);
			}
			}
			mWorldBuilderTileScaleActive = false;
			mWorldTileRenderer->drawDecorationTile(type, displayedTile);
			outlineRect(displayedTile, 10, 20, 27, 100, 1);
		}
	}

	if (currentMapId() == mWorldStartMap &&
		visibleTiles.contains(mWorldStartX, mWorldStartY))
	{
		int startX = mapX + mWorldStartX * tileSize;
		int startY = mapY + mWorldStartY * tileSize;
		int inset = std::max(2, tileSize / 4);
		fillRect({ startX + inset, startY + inset, tileSize - inset * 2,
			tileSize - inset * 2 }, 24, 66, 137, 235);
		if (tileSize >= 32)
			drawText("P", startX + tileSize / 2 - 5, startY + tileSize / 2 - 9,
				color(215, 232, 255), std::min(16, tileSize / 3));
	}
	for (size_t i = 0; i < mWorldPortals.size(); ++i)
		if (mWorldPortals[i].fromMap == currentMapId() &&
			visibleTiles.contains(mWorldPortals[i].fromX, mWorldPortals[i].fromY))
		{
			int inset = std::max(2, tileSize / 12);
			outlineRect({ mapX + mWorldPortals[i].fromX * tileSize + inset,
				mapY + mWorldPortals[i].fromY * tileSize + inset,
				tileSize - inset * 2, tileSize - inset * 2 },
				91, 222, 232, 255, std::max(2, tileSize / 24));
		}
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
	{
		const WorldObject& object = mWorldObjects[i];
		if (object.mapId != currentMapId() || !visibleTiles.contains(object.x, object.y))
			continue;
		int x = mapX + object.x * tileSize;
		int y = mapY + object.y * tileSize;
		if (object.kind == WorldObjectKind::Signpost)
		{
			int postWidth = std::max(2, tileSize / 9);
			fillRect({ x + tileSize / 2 - postWidth / 2, y + tileSize / 3,
				postWidth, tileSize * 3 / 5 }, 91, 58, 32, 255);
			fillRect({ x + tileSize / 7, y + tileSize / 6,
				tileSize * 5 / 7, tileSize / 3 }, 178, 125, 59, 255);
			outlineRect({ x + tileSize / 7, y + tileSize / 6,
				tileSize * 5 / 7, tileSize / 3 }, 70, 43, 27, 255, 1);
		}
		else
		{
			int inset = std::max(2, tileSize / 7);
			fillRect({ x + inset, y + tileSize * 2 / 5,
				tileSize - inset * 2, tileSize / 2 }, 121, 69, 32, 255);
			fillRect({ x + inset, y + tileSize / 4,
				tileSize - inset * 2, tileSize / 4 }, 166, 97, 39, 255);
			outlineRect({ x + inset, y + tileSize / 4,
				tileSize - inset * 2, tileSize * 13 / 20 }, 58, 35, 23, 255, 1);
			fillRect({ x + tileSize * 9 / 20, y + tileSize / 2,
				std::max(2, tileSize / 8), tileSize / 5 }, 224, 174, 65, 255);
		}
		if ((int)i == mWorldBuilderSelectedObject &&
			mWorldBuilderTab == WorldBuilderTab::Objects)
			outlineRect({ x + 2, y + 2, tileSize - 4, tileSize - 4 },
				246, 211, 99, 255, std::max(2, tileSize / 16));
	}
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		if (shard.mapId != currentMapId()) continue;
		if (!visibleTiles.contains(shard.x, shard.y)) continue;
		int x = mapX + shard.x * tileSize;
		int y = mapY + shard.y * tileSize;
		int unit = std::max(1, tileSize / 12);
		fillRect({ x + tileSize / 2 - unit * 2, y + tileSize / 5,
			unit * 4, tileSize * 3 / 5 }, 47, 25, 71, 230);
		fillRect({ x + tileSize / 4, y + tileSize / 3,
			tileSize / 2, tileSize / 3 }, 146, 87, 211, 250);
		fillRect({ x + tileSize * 2 / 5, y + tileSize * 2 / 5,
			tileSize / 5, tileSize / 5 }, 231, 193, 255, 255);
		if ((int)(mWorldObjects.size() + i) == mWorldBuilderSelectedObject &&
			mWorldBuilderTab == WorldBuilderTab::Objects)
			outlineRect({ x + 2, y + 2, tileSize - 4, tileSize - 4 },
				246, 211, 99, 255, std::max(2, tileSize / 16));
	}
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (mNpcs[i].mapId != currentMapId()) continue;
		if (!visibleTiles.contains(mNpcs[i].x, mNpcs[i].y)) continue;
		int npcX = mapX + mNpcs[i].x * tileSize;
		int npcY = mapY + mNpcs[i].y * tileSize;
		if (tileSize >= TILE)
			drawCharacter((float)mNpcs[i].x, (float)mNpcs[i].y,
				mNpcs[i].appearance, false, false, mNpcs[i].facingX, mNpcs[i].facingY);
		else
		{
			int inset = std::max(3, tileSize / 5);
			fillRect({ npcX + inset, npcY + inset, tileSize - inset * 2,
				tileSize - inset * 2 }, 213, 137, 78, 255);
			outlineRect({ npcX + inset, npcY + inset, tileSize - inset * 2,
				tileSize - inset * 2 }, 62, 35, 31, 255, 1);
		}
		if ((int)i == mWorldBuilderSelectedNpc && mWorldBuilderTab == WorldBuilderTab::Npcs)
			outlineRect({ npcX + 2, npcY + 2, tileSize - 4, tileSize - 4 },
				246, 211, 99, 255, std::max(2, tileSize / 16));
	}
	int hoveredNpc = worldBuilderHoveredNpc();
	if (hoveredNpc >= 0)
	{
		const Npc& npc = mNpcs[hoveredNpc];
		int npcX = mapX + npc.x * tileSize;
		int npcY = mapY + npc.y * tileSize;
		int labelWidth = std::max(54, std::min(230, 18 + (int)npc.id.size() * 8));
		int labelX = std::max(MAP_X + 3, std::min(MAP_X + MAP_VIEW_WIDTH - labelWidth - 3,
			npcX + tileSize / 2 - labelWidth / 2));
		int labelY = npcY - 27;
		if (labelY < MAP_Y + 3) labelY = npcY + tileSize + 3;
		fillRect({ labelX, labelY, labelWidth, 24 }, 12, 20, 34, 244);
		outlineRect({ labelX, labelY, labelWidth, 24 }, 238, 188, 79, 255, 2);
		drawText(npc.id, labelX + 8, labelY + 5, color(247, 221, 151), 12,
			labelWidth - 16);
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
	tab(BUILDER_OBJECTS_TAB, "OBJECTS", mWorldBuilderTab == WorldBuilderTab::Objects);

	if (mWorldBuilderTab == WorldBuilderTab::Tiles)
	{
		for (int category = 0; category < TILE_CATEGORY_COUNT; ++category)
		{
			SDL_Rect button = tileCategoryRect(category);
			bool active = category == mWorldBuilderTileCategory;
			fillRect(button, active ? 68 : 32, active ? 76 : 43,
				active ? 58 : 60, 245);
			outlineRect(button, active ? 226 : 94, active ? 183 : 113,
				active ? 84 : 137, 255, 2);
			drawText(TILE_CATEGORY_NAMES[category], button.x + 7, button.y + 8,
				color(232, 236, 244), category == BuildingTiles ? 9 : 10, button.w - 12);
		}

		std::vector<WorldTileId> tiles = categoryTiles(mWorldBuilderTileCategory);
		mWorldBuilderHoveredTileName.clear();
		for (int slot = 0; slot < BUILDER_VISIBLE_TILES; ++slot)
		{
			int i = mWorldBuilderListScroll + slot;
			if (i >= (int)tiles.size()) break;
			SDL_Rect button = paletteRect(slot);
			bool selected = mWorldBuilderTile == tiles[i];
			bool hovered = contains(button, mMouseX, mMouseY);
			fillRect(button, selected ? 79 : 38, selected ? 68 : 47, selected ? 43 : 64, 245);
			outlineRect(button, selected ? 241 : (hovered ? 174 : 110),
				selected ? 190 : (hovered ? 188 : 125),
				selected ? 87 : (hovered ? 206 : 147), 255, 2);
			drawWorldBuilderTileIcon(tiles[i],
				{ button.x + (button.w - 29) / 2, button.y + 2, 29, 29 });
			if (hovered) mWorldBuilderHoveredTileName = tileName(tiles[i]);
		}
		fillRect({ 1022, 623, 228, 34 }, 27, 37, 53, 245);
		outlineRect({ 1022, 623, 228, 34 }, 88, 112, 143, 255, 1);
		std::string tileLabel = mWorldBuilderHoveredTileName.empty() ?
			"Selected: " + std::string(tileName(mWorldBuilderTile)) :
			mWorldBuilderHoveredTileName;
		drawText(tileLabel, 1032, 632, color(224, 232, 243), 12, 208);
		drawText("Hover: name  •  Wheel: tiles", 1022, 673,
			color(179, 195, 218), 12, 220);
		drawText("Click/drag map to paint", 1022, 699,
			color(153, 174, 205), 11, 220);
	}
	else
	{
		mWorldBuilderHoveredTileName.clear();
		int count = mWorldBuilderTab == WorldBuilderTab::Npcs ? (int)mNpcs.size() :
			(int)(mWorldObjects.size() + mMercerStock.shards.size());
		int selected = mWorldBuilderTab == WorldBuilderTab::Npcs ? mWorldBuilderSelectedNpc :
			mWorldBuilderSelectedObject;
		for (int row = 0; row < BUILDER_LIST_ROWS; ++row)
		{
			int index = mWorldBuilderListScroll + row;
			if (index >= count) break;
			SDL_Rect item = { 1022, BUILDER_LIST_Y + row * BUILDER_LIST_ROW, 228, 34 };
			fillRect(item, index == selected ? 74 : (row % 2 ? 31 : 37),
				index == selected ? 61 : 42, index == selected ? 42 : 59, 238);
			if (index == selected) outlineRect(item, 233, 184, 82, 255, 2);
			if (mWorldBuilderTab == WorldBuilderTab::Npcs)
			{
				int entityX = mNpcs[index].x;
				int entityY = mNpcs[index].y;
				drawWorldBuilderNpcPortrait(mNpcs[index],
					{ item.x + 2, item.y + 2, 30, 30 });
				std::string label = mNpcs[index].id + " (" + mNpcs[index].name + ")";
				drawText(label, item.x + 38, item.y + 3, color(232, 236, 244), 8, 188);
				drawText(std::to_string(entityX) + "," + std::to_string(entityY),
					item.x + 38, item.y + 18, color(173, 193, 220), 9, 100);
			}
			else
			{
				bool regularObject = index < (int)mWorldObjects.size();
				int shardIndex = index - (int)mWorldObjects.size();
				const std::string& name = regularObject ? mWorldObjects[index].name :
					mMercerStock.shards[shardIndex].name;
				int entityX = regularObject ? mWorldObjects[index].x :
					mMercerStock.shards[shardIndex].x;
				int entityY = regularObject ? mWorldObjects[index].y :
					mMercerStock.shards[shardIndex].y;
				if (regularObject)
				{
					if (mWorldObjects[index].kind == WorldObjectKind::Signpost)
					{
						fillRect({ item.x + 8, item.y + 17, 4, 13 }, 91, 58, 32, 255);
						fillRect({ item.x + 3, item.y + 7, 18, 11 }, 178, 125, 59, 255);
					}
					else
					{
						fillRect({ item.x + 3, item.y + 13, 20, 16 }, 117, 67, 31, 255);
						fillRect({ item.x + 3, item.y + 8, 20, 8 }, 165, 96, 39, 255);
						fillRect({ item.x + 11, item.y + 16, 5, 7 }, 225, 176, 66, 255);
					}
				}
				else
				{
					fillRect({ item.x + 5, item.y + 7, 16, 20 }, 123, 73, 185, 255);
					fillRect({ item.x + 9, item.y + 12, 8, 10 }, 226, 184, 255, 255);
				}
				drawText(name, item.x + 27, item.y + 3,
					color(232, 236, 244), 9, 148);
				std::string kindLabel = regularObject ?
					(mWorldObjects[index].kind == WorldObjectKind::DeckChest ?
						"Deck Chest" : "Signpost") : "Shard";
				drawText(kindLabel, item.x + 27, item.y + 18,
					color(171, 192, 218), 8, 80);
				drawText(std::to_string(entityX) + "," + std::to_string(entityY), item.x + 181,
					item.y + 10, color(173, 193, 220), 9);
			}
		}
		if (count > BUILDER_LIST_ROWS)
			drawText("Mouse wheel scrolls", 1052, 682, color(166, 184, 211), 12);
		drawText("Click: select  •  Double-click: locate", 1022, 701,
			color(166, 184, 211), 10, 224);
	}

	fillRect(BUILDER_SAVE, mWorldBuilderDirty ? 74 : 45, mWorldBuilderDirty ? 76 : 55,
		mWorldBuilderDirty ? 43 : 67, 250);
	outlineRect(BUILDER_SAVE, 207, 161, 66, 255, 2);
	drawText("SAVE TO LUA   Ctrl+S", BUILDER_SAVE.x + 29, BUILDER_SAVE.y + 12,
		color(245, 226, 181), 14);
	drawText("T/N/O: tabs  •  Arrows/WASD: pan  •  Wheel over map or +/-: zoom",
		32, 650, color(180, 196, 219), 14, 930);
	drawText("Tiles: paint  •  Entities: place  •  P: start  •  PageUp/PageDown: maps",
		32, 676, color(142, 173, 217), 13);
	if (!mWorldBuilderNotice.empty() && SDL_GetTicks() < mWorldBuilderNoticeUntil)
	{
		fillRect({ 32, 716, 930, 42 }, mWorldBuilderNoticeError ? 71 : 25,
			mWorldBuilderNoticeError ? 30 : 62, mWorldBuilderNoticeError ? 31 : 43, 235);
		drawText(mWorldBuilderNotice, 45, 727,
			mWorldBuilderNoticeError ? color(255, 176, 166) : color(132, 234, 156), 15, 900);
	}
}
