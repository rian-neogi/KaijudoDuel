#include "Application.h"

#include "AppSupport.h"
#include "AssetManager.h"
#include "CatalogMapStorage.h"
#include "LuaInclude.h"
#include "WorldStorage.h"
#include "WorldTileRenderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <set>
#include <tuple>

using namespace AppSupport;

namespace
{
	const SDL_Rect BUILDER_PANEL = { 1008, 18, 256, 764 };
	const SDL_Rect BUILDER_PREVIOUS_MAP = { 1022, 61, 32, 34 };
	const SDL_Rect BUILDER_NEW_MAP = { 1178, 61, 36, 34 };
	const SDL_Rect BUILDER_NEXT_MAP = { 1218, 61, 32, 34 };
	const SDL_Rect BUILDER_GRID = { 1162, 27, 88, 25 };
	const SDL_Rect BUILDER_TILES_TAB = { 1022, 105, 40, 35 };
	const SDL_Rect BUILDER_NPCS_TAB = { 1064, 105, 38, 35 };
	const SDL_Rect BUILDER_OBJECTS_TAB = { 1104, 105, 48, 35 };
	const SDL_Rect BUILDER_REGIONS_TAB = { 1154, 105, 50, 35 };
	const SDL_Rect BUILDER_PORTALS_TAB = { 1206, 105, 44, 35 };
	const SDL_Rect BUILDER_OBJECT_ADD = { 1022, 663, 108, 28 };
	const SDL_Rect BUILDER_OBJECT_PLACED = { 1142, 663, 108, 28 };
	const SDL_Rect BUILDER_PORTAL_NEW = { 1022, 663, 70, 28 };
	const SDL_Rect BUILDER_PORTAL_FROM = { 1096, 663, 70, 28 };
	const SDL_Rect BUILDER_PORTAL_TO = { 1170, 663, 80, 28 };
	const SDL_Rect BUILDER_PORTAL_APPEARANCE_PREVIOUS = { 1022, 623, 28, 32 };
	const SDL_Rect BUILDER_PORTAL_APPEARANCE = { 1054, 623, 164, 32 };
	const SDL_Rect BUILDER_PORTAL_APPEARANCE_NEXT = { 1222, 623, 28, 32 };
	const SDL_Rect BUILDER_REGION_NEW = { 1022, 590, 68, 28 };
	const SDL_Rect BUILDER_REGION_BOUNDS = { 1094, 590, 78, 28 };
	const SDL_Rect BUILDER_REGION_DELETE = { 1176, 590, 74, 28 };
	const SDL_Rect BUILDER_REGION_KIND = { 1022, 623, 111, 28 };
	const SDL_Rect BUILDER_REGION_WEATHER = { 1137, 623, 113, 28 };
	const SDL_Rect BUILDER_REGION_NAME = { 1022, 656, 228, 35 };
	const SDL_Rect BUILDER_UNDO = { 1022, 724, 91, 44 };
	const SDL_Rect BUILDER_SAVE = { 1119, 724, 131, 44 };
	const int BUILDER_LIST_Y = 151;
	const int BUILDER_LIST_ROW = 39;
	const int BUILDER_LIST_ROWS = 13;
	const int BUILDER_REGION_LIST_ROWS = 11;
	const int BUILDER_PORTAL_LIST_ROWS = 12;
	const int BUILDER_OBJECT_LIST_ROWS = 12;
	const int BUILDER_CATEGORY_Y = 151;
	const int BUILDER_SHEET_Y = 219;
	const SDL_Rect BUILDER_TILESET_VIEW = { 1022, 257, 228, 388 };
	const SDL_Rect BUILDER_BRUSH_DECREASE = { 1022, 651, 34, 34 };
	const SDL_Rect BUILDER_BRUSH_LABEL = { 1060, 651, 152, 34 };
	const SDL_Rect BUILDER_BRUSH_INCREASE = { 1216, 651, 34, 34 };
	const SDL_Rect BUILDER_TILE_INFO = { 1022, 690, 228, 25 };
	const SDL_Rect BUILDER_MAP_DIALOG = { 326, 164, 628, 444 };
	const SDL_Rect BUILDER_MAP_ID = { 504, 226, 356, 38 };
	const SDL_Rect BUILDER_MAP_NAME = { 504, 282, 356, 38 };
	const SDL_Rect BUILDER_MAP_WIDTH = { 504, 338, 150, 38 };
	const SDL_Rect BUILDER_MAP_HEIGHT = { 710, 338, 150, 38 };
	const SDL_Rect BUILDER_MAP_INDOOR = { 504, 394, 188, 38 };
	const SDL_Rect BUILDER_MAP_CANCEL = { 566, 526, 128, 44 };
	const SDL_Rect BUILDER_MAP_CREATE = { 710, 526, 150, 44 };
	const Uint32 BUILDER_PAN_INTERVAL = 80;
	const int BUILDER_MAX_BRUSH_SIZE = 9;
	const int BUILDER_MAX_UNDO_ACTIONS = 100;
	const int BUILDER_UNDO_NPC = 1;
	const int BUILDER_UNDO_OBJECT = 2;
	const int BUILDER_UNDO_SHARD = 3;
	const int BUILDER_UNDO_OBJECT_CREATED = 4;
	const int BUILDER_UNDO_OBJECT_DELETED = 5;
	const int BUILDER_UNDO_PORTAL = 6;
	const int BUILDER_UNDO_PORTAL_CREATED = 7;
	const int BUILDER_UNDO_PORTAL_DELETED = 8;
	const int BUILDER_UNDO_REGION = 9;
	const int BUILDER_UNDO_REGION_CREATED = 10;
	const int BUILDER_UNDO_REGION_DELETED = 11;
	const int BUILDER_UNDO_OBJECT_APPEARANCE = 12;
	const int BUILDER_UNDO_MAP_CREATED = 13;
	const int BUILDER_ZOOM_PERCENTAGES[] = {
		10, 20, 30, 40, 50, 60, 70, 80, 90, 100
	};
	const int BUILDER_ZOOM_LEVELS[] = {
		(TILE * 10 + 50) / 100, (TILE * 20 + 50) / 100,
		(TILE * 30 + 50) / 100, (TILE * 40 + 50) / 100,
		(TILE * 50 + 50) / 100, (TILE * 60 + 50) / 100,
		(TILE * 70 + 50) / 100, (TILE * 80 + 50) / 100,
		(TILE * 90 + 50) / 100, TILE
	};
	const int BUILDER_ZOOM_LEVEL_COUNT = sizeof(BUILDER_ZOOM_LEVELS) /
		sizeof(BUILDER_ZOOM_LEVELS[0]);
	const char* const PORTAL_APPEARANCES[] = {
		"!Door1-1", "!Door1-2", "!Door1-3", "!Door1-4",
		"!Door1-5", "!Door1-6", "!Door1-7", "!Door1-8",
		"!Door2-1", "!Door2-2", "!Door2-3", "!Door2-4",
		"!Door2-5", "!Door2-6", "!Door2-7", "!Door2-8",
		"!Door3-1", "!Door3-2", "!Door3-3", "!Door3-4",
		"!Door3-5", "!Door3-6", "!Door3-7", "!Door3-8",
		"!$Gate1-1", "!$Gate2-1", ""
	};
	const int PORTAL_APPEARANCE_COUNT = sizeof(PORTAL_APPEARANCES) /
		sizeof(PORTAL_APPEARANCES[0]);
	const char* const DEFAULT_PORTAL_APPEARANCE = "!Door3-5";
	const char* const CHEST_APPEARANCES[] = {
		"!Chest-1", "!Chest-2", "!Chest-3", "!Chest-4",
		"!Chest-5", "!Chest-6", "!Chest-7", "!Chest-8"
	};
	const int CHEST_APPEARANCE_COUNT = sizeof(CHEST_APPEARANCES) /
		sizeof(CHEST_APPEARANCES[0]);

	bool samePortal(const WorldPortal& first, const WorldPortal& second)
	{
		return first.appearance == second.appearance &&
			first.fromMap == second.fromMap && first.fromX == second.fromX &&
			first.fromY == second.fromY && first.toMap == second.toMap &&
			first.toX == second.toX && first.toY == second.toY;
	}

	std::string portalAppearanceLabel(const std::string& appearance)
	{
		return appearance.empty() ? "None (invisible)" : appearance;
	}

	bool validBuilderMapId(const std::string& id)
	{
		if (id.empty()) return false;
		for (size_t index = 0; index < id.size(); ++index)
		{
			unsigned char character = (unsigned char)id[index];
			if (!((character >= 'a' && character <= 'z') ||
				(character >= 'A' && character <= 'Z') ||
				(character >= '0' && character <= '9') || character == '_' ||
				character == '-')) return false;
		}
		return true;
	}

	bool builderMapDimension(const std::string& input, int& dimension)
	{
		if (input.empty()) return false;
		dimension = 0;
		for (size_t index = 0; index < input.size(); ++index)
		{
			if (input[index] < '0' || input[index] > '9') return false;
			dimension = dimension * 10 + input[index] - '0';
			if (dimension > 1024) return false;
		}
		return dimension >= 1;
	}

	bool sameRegion(const WorldRegion& first, const WorldRegion& second)
	{
		return first.id == second.id && first.name == second.name &&
			first.mapId == second.mapId && first.x == second.x &&
			first.y == second.y && first.width == second.width &&
			first.height == second.height && first.connector == second.connector &&
			first.snow == second.snow;
	}

	int builderZoomLevel(int tileSize)
	{
		for (int level = 0; level < BUILDER_ZOOM_LEVEL_COUNT; ++level)
			if (BUILDER_ZOOM_LEVELS[level] == tileSize) return level;
		return BUILDER_ZOOM_LEVEL_COUNT - 1;
	}
	const int TILE_CATEGORY_COUNT = 4;
	const char* TILE_CATEGORY_NAMES[TILE_CATEGORY_COUNT] = {
		"DUNGEON", "INSIDE", "OUTSIDE", "WORLD"
	};
	const SDL_Rect BUILDER_PREVIOUS_SHEET = { 1022, BUILDER_SHEET_Y, 28, 29 };
	const SDL_Rect BUILDER_SHEET_NAME = { 1054, BUILDER_SHEET_Y, 164, 29 };
	const SDL_Rect BUILDER_NEXT_SHEET = { 1222, BUILDER_SHEET_Y, 28, 29 };

	SDL_Rect tileCategoryRect(int index)
	{
		return { 1022 + (index % 2) * 116, BUILDER_CATEGORY_Y + (index / 2) * 34,
			108, 29 };
	}

	SDL_Rect catalogSheetRect(const RtpSheetDescriptor& sheet)
	{
		float scale = std::min((float)BUILDER_TILESET_VIEW.w / sheet.width,
			(float)BUILDER_TILESET_VIEW.h / sheet.height);
		int width = std::max(1, (int)std::floor(sheet.width * scale));
		int height = std::max(1, (int)std::floor(sheet.height * scale));
		return { BUILDER_TILESET_VIEW.x + (BUILDER_TILESET_VIEW.w - width) / 2,
			BUILDER_TILESET_VIEW.y, width, height };
	}

	SDL_Rect scaledCatalogTileRect(const RtpSheetDescriptor& sheet,
		const SDL_Rect& sheetDestination, const SDL_Rect& source)
	{
		int left = sheetDestination.x + source.x * sheetDestination.w / sheet.width;
		int top = sheetDestination.y + source.y * sheetDestination.h / sheet.height;
		int right = sheetDestination.x + (source.x + source.w) *
			sheetDestination.w / sheet.width;
		int bottom = sheetDestination.y + (source.y + source.h) *
			sheetDestination.h / sheet.height;
		return { left, top, std::max(1, right - left), std::max(1, bottom - top) };
	}

	int catalogTileAt(const RtpSheetDescriptor& sheet,
		const SDL_Rect& sheetDestination, int x, int y)
	{
		auto inside = [](const SDL_Rect& rect, int pointX, int pointY) -> bool
		{
			return pointX >= rect.x && pointX < rect.x + rect.w &&
				pointY >= rect.y && pointY < rect.y + rect.h;
		};
		if (!inside(sheetDestination, x, y)) return -1;
		int sourceX = (x - sheetDestination.x) * sheet.width / sheetDestination.w;
		int sourceY = (y - sheetDestination.y) * sheet.height / sheetDestination.h;
		for (int tile = 0; tile < sheet.tileCount; ++tile)
		{
			SDL_Rect source;
			if (RtpTilesetRenderer::paletteTileSource(sheet.sheet, tile, source) &&
				inside(source, sourceX, sourceY)) return tile;
		}
		return -1;
	}

	RtpTilesetFamily tilesetFamily(int category)
	{
		if (category == 0) return RtpTilesetFamily::Dungeon;
		if (category == 1) return RtpTilesetFamily::Inside;
		if (category == 3) return RtpTilesetFamily::World;
		return RtpTilesetFamily::Outside;
	}

	const char* familyName(RtpTilesetFamily family)
	{
		if (family == RtpTilesetFamily::Dungeon) return "Dungeon";
		if (family == RtpTilesetFamily::Inside) return "Inside";
		if (family == RtpTilesetFamily::World) return "World";
		return "Outside";
	}

	bool parseFamily(const std::string& name, RtpTilesetFamily& family)
	{
		for (int i = 0; i < TILE_CATEGORY_COUNT; ++i)
			if (name == familyName(tilesetFamily(i)))
			{
				family = tilesetFamily(i);
				return true;
			}
		return false;
	}

	const char* sheetName(RtpTileSheet sheet)
	{
		if (sheet == RtpTileSheet::A1) return "A1";
		if (sheet == RtpTileSheet::A2) return "A2";
		if (sheet == RtpTileSheet::A3) return "A3";
		if (sheet == RtpTileSheet::A4) return "A4";
		if (sheet == RtpTileSheet::A5) return "A5";
		if (sheet == RtpTileSheet::B) return "B";
		return "C";
	}

	bool parseSheet(const std::string& name, RtpTileSheet& sheet)
	{
		const RtpTileSheet sheets[] = { RtpTileSheet::A1, RtpTileSheet::A2,
			RtpTileSheet::A3, RtpTileSheet::A4, RtpTileSheet::A5,
			RtpTileSheet::B, RtpTileSheet::C };
		for (int i = 0; i < 7; ++i)
			if (name == sheetName(sheets[i])) { sheet = sheets[i]; return true; }
		return false;
	}

	const char* layerName(RtpRenderLayer layer)
	{
		if (layer == RtpRenderLayer::Decoration) return "decoration";
		if (layer == RtpRenderLayer::Foreground) return "foreground";
		return "ground";
	}

	bool parseLayer(const std::string& name, RtpRenderLayer& layer)
	{
		if (name == "ground") layer = RtpRenderLayer::Ground;
		else if (name == "decoration") layer = RtpRenderLayer::Decoration;
		else if (name == "foreground") layer = RtpRenderLayer::Foreground;
		else return false;
		return true;
	}

	std::vector<RtpTileSheet> familySheets(RtpTilesetFamily family)
	{
		std::vector<RtpTileSheet> result;
		std::vector<RtpSheetDescriptor> available = RtpTilesetRenderer::availableSheets();
		for (size_t i = 0; i < available.size(); ++i)
			if (available[i].family == family) result.push_back(available[i].sheet);
		return result;
	}

	std::string catalogTileName(RtpTilesetFamily family, RtpTileSheet sheet, int index)
	{
		const char* treeName = RtpTilesetRenderer::treeAutotileName(family, sheet, index);
		if (treeName != NULL) return treeName;
		static std::map<std::pair<int, int>, std::vector<std::string> > cache;
		std::pair<int, int> key((int)family, (int)sheet);
		if (cache.count(key) == 0)
		{
			std::vector<std::string> names;
			std::string error;
			RtpTilesetRenderer::loadTileNames(family, sheet, names, error);
			cache[key] = names;
		}
		const std::vector<std::string>& names = cache[key];
		if (index >= 0 && index < (int)names.size() && !names[index].empty())
			return names[index];
		return std::string(sheetName(sheet)) + " tile " + std::to_string(index);
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

	void clampMapCamera(const std::vector<std::string>& map, float& cameraX, float& cameraY,
		int tileSize)
	{
		float visibleColumns = MAP_VIEW_WIDTH / (float)tileSize;
		float visibleRows = MAP_VIEW_HEIGHT / (float)tileSize;
		cameraX = std::max(0.f, std::min(std::max(0.f,
			(float)map[0].size() - visibleColumns), cameraX));
		cameraY = std::max(0.f, std::min(std::max(0.f,
			(float)map.size() - visibleRows), cameraY));
	}

	void centerMapCamera(const std::vector<std::string>& map, int x, int y,
		float& cameraX, float& cameraY, int tileSize)
	{
		cameraX = x - MAP_VIEW_WIDTH / (2.f * tileSize);
		cameraY = y - MAP_VIEW_HEIGHT / (2.f * tileSize);
		clampMapCamera(map, cameraX, cameraY, tileSize);
	}

	bool mapCellAt(int x, int y, const std::vector<std::string>& map,
		float cameraX, float cameraY, int tileSize, int& cellX, int& cellY)
	{
		if (map.empty() || map[0].empty()) return false;
		if (x < MAP_X || x >= MAP_X + MAP_VIEW_WIDTH || y < MAP_Y ||
			y >= MAP_Y + MAP_VIEW_HEIGHT) return false;
		float originX = builderMapOriginX((int)map[0].size(), tileSize) -
			cameraX * tileSize;
		float originY = builderMapOriginY((int)map.size(), tileSize) -
			cameraY * tileSize;
		if (x < originX || y < originY) return false;
		cellX = (int)std::floor((x - originX) / tileSize);
		cellY = (int)std::floor((y - originY) / tileSize);
		return cellY >= 0 && cellY < (int)map.size() && cellX >= 0 &&
			cellX < (int)map[cellY].size();
	}

}

bool Application::loadWorldMap(const std::string& path, std::string& error,
	bool allowMissingPositions)
{
	WorldData loadedWorld;
	if (!WorldStorage::load(path, loadedWorld, error)) return false;
	std::vector<WorldObject> candidateObjects;
	std::set<std::string> objectIdsInUse;
	for (size_t index = 0; index < mWorldObjects.size(); ++index)
		if (!mWorldObjects[index].editorCreated)
		{
			candidateObjects.push_back(mWorldObjects[index]);
			objectIdsInUse.insert(mWorldObjects[index].id);
		}
	for (std::map<std::string, WorldObjectDefinition>::const_iterator definition =
		loadedWorld.objectDefinitions.begin();
		definition != loadedWorld.objectDefinitions.end(); ++definition)
	{
		const WorldObjectTemplate* objectTemplate = NULL;
		for (size_t index = 0; index < mWorldObjectTemplates.size(); ++index)
			if (mWorldObjectTemplates[index].id == definition->second.templateId)
			{
				objectTemplate = &mWorldObjectTemplates[index];
				break;
			}
		if (objectTemplate == NULL || !objectIdsInUse.insert(definition->first).second)
		{
			error = "placed object '" + definition->first +
				"' has an unknown template or duplicate ID";
			return false;
		}
		candidateObjects.push_back(createWorldObject(*objectTemplate, definition->first));
	}
	for (std::map<std::string, std::string>::const_iterator appearance =
		loadedWorld.objectAppearances.begin();
		appearance != loadedWorld.objectAppearances.end(); ++appearance)
	{
		WorldObject* object = NULL;
		for (size_t index = 0; index < candidateObjects.size(); ++index)
			if (candidateObjects[index].id == appearance->first)
			{
				object = &candidateObjects[index];
				break;
			}
		if (object == NULL || (object->kind != WorldObjectKind::Chest &&
			object->kind != WorldObjectKind::DeckChest) ||
			!setWorldObjectAppearance(*object, appearance->second))
		{
			error = "placed chest '" + appearance->first +
				"' has an invalid appearance override";
			return false;
		}
	}
	auto walkable = [this](const WorldData& world, const WorldPosition& position) -> bool
	{
		const WorldMap* map = world.map(position.mapId);
		return map != NULL && worldTileWalkable(*map, position.x, position.y);
	};
	if (!walkable(loadedWorld, loadedWorld.start))
	{
		error = "world start must reference a walkable catalog tile";
		return false;
	}
	for (size_t index = 0; index < loadedWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = loadedWorld.portals[index];
		if (!walkable(loadedWorld, { portal.fromMap, portal.fromX, portal.fromY }) ||
			!walkable(loadedWorld, { portal.toMap, portal.toX, portal.toY }))
		{
			error = "portal " + std::to_string(index + 1) +
				" must use walkable catalog tiles";
			return false;
		}
	}
	std::set<std::tuple<std::string, int, int> > occupied;
	occupied.insert(std::make_tuple(loadedWorld.start.mapId,
		loadedWorld.start.x, loadedWorld.start.y));
	for (size_t index = 0; index < loadedWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = loadedWorld.portals[index];
		occupied.insert(std::make_tuple(portal.fromMap, portal.fromX, portal.fromY));
		occupied.insert(std::make_tuple(portal.toMap, portal.toX, portal.toY));
	}
	bool placedMissing = false;
	auto resolvePositions = [&](const char* groupName, const std::vector<std::string>& ids,
		const std::map<std::string, WorldPosition>& source,
		std::map<std::string, WorldPosition>& resolved) -> bool
	{
		resolved.clear();
		for (size_t index = 0; index < ids.size(); ++index)
		{
			std::map<std::string, WorldPosition>::const_iterator found = source.find(ids[index]);
			if (found != source.end())
			{
				if (!walkable(loadedWorld, found->second) ||
					!occupied.insert(std::make_tuple(found->second.mapId,
						found->second.x, found->second.y)).second)
				{
					error = std::string("invalid or occupied ") + groupName +
						" position for '" + ids[index] + "'";
					return false;
				}
				resolved[ids[index]] = found->second;
				continue;
			}
			if (!allowMissingPositions)
			{
				error = std::string("missing ") + groupName + " position for '" +
					ids[index] + "'";
				return false;
			}
			bool placed = false;
			for (size_t mapIndex = 0; mapIndex < loadedWorld.maps.size() && !placed; ++mapIndex)
				for (int y = 0; y < loadedWorld.maps[mapIndex].height() && !placed; ++y)
					for (int x = 0; x < loadedWorld.maps[mapIndex].width() && !placed; ++x)
					{
						WorldPosition candidate = { loadedWorld.maps[mapIndex].id, x, y };
						if (walkable(loadedWorld, candidate) &&
							occupied.insert(std::make_tuple(candidate.mapId, x, y)).second)
						{
							resolved[ids[index]] = candidate;
							placed = placedMissing = true;
						}
					}
			if (!placed)
			{
				error = "no free walkable tile is available for new world entities";
				return false;
			}
		}
		return true;
	};
	std::vector<std::string> npcIds;
	for (size_t index = 0; index < mNpcs.size(); ++index) npcIds.push_back(mNpcs[index].id);
	std::vector<std::string> objectIds;
	for (size_t index = 0; index < candidateObjects.size(); ++index)
		objectIds.push_back(candidateObjects[index].id);
	std::vector<std::string> shardIds;
	for (size_t index = 0; index < mMercerStock.shards.size(); ++index)
		shardIds.push_back(mMercerStock.shards[index].id);
	std::map<std::string, WorldPosition> npcPositions;
	std::map<std::string, WorldPosition> objectPositions;
	std::map<std::string, WorldPosition> shardPositions;
	if (!resolvePositions("npc", npcIds, loadedWorld.npcPositions, npcPositions) ||
		!resolvePositions("object", objectIds, loadedWorld.objectPositions, objectPositions) ||
		!resolvePositions("shard", shardIds, loadedWorld.shardPositions, shardPositions))
		return false;
	loadedWorld.npcPositions.swap(npcPositions);
	loadedWorld.objectPositions.swap(objectPositions);
	loadedWorld.shardPositions.swap(shardPositions);
	if (!loadedWorld.validateStructure(error)) return false;
	mWorldObjects.swap(candidateObjects);
	loadedWorld.swap(mWorld);
	clearWorldBuilderUndoHistory();
	mWorldBuilderSelectedPortal = -1;
	mWorldBuilderPortalEndpoint = 0;
	mWorldBuilderPortalCreating = false;
	mWorldBuilderPortalDraftOrigin = WorldPosition();
	mWorldBuilderSelectedRegion = -1;
	mWorldBuilderRegionPlacementMode = 0;
	mWorldBuilderRegionDragging = false;
	mWorldBuilderRegionNameFocused = false;
	mWorldBuilderRegionNameIndex = -1;
	mWorldBuilderRegionNameInput.clear();
	mWorldBuilderMapDialogOpen = false;
	mWorldBuilderMapDialogField = 0;
	mWorldBuilderMapIdInput.clear();
	mWorldBuilderMapNameInput.clear();
	mWorldBuilderMapWidthInput.clear();
	mWorldBuilderMapHeightInput.clear();
	mWorldBuilderMapIndoorInput = false;
	mWorldBuilderMapDialogError.clear();
	mCurrentWorldArea = worldAreaIndex(mWorld.start.mapId);
	mPlayerX = mWorld.start.x;
	mPlayerY = mWorld.start.y;
	mVisualX = (float)mPlayerX;
	mVisualY = (float)mPlayerY;
	centerMapCamera(currentMap(), mPlayerX, mPlayerY, mWorldBuilderCameraX,
		mWorldBuilderCameraY, mWorldBuilderTileSize);
	for (size_t index = 0; index < mNpcs.size(); ++index)
	{
		const WorldPosition& position = mWorld.npcPositions[mNpcs[index].id];
		mNpcs[index].mapId = position.mapId;
		mNpcs[index].setPosition(position.x, position.y);
	}
	for (size_t index = 0; index < mWorldObjects.size(); ++index)
	{
		const WorldPosition& position = mWorld.objectPositions[mWorldObjects[index].id];
		mWorldObjects[index].mapId = position.mapId;
		mWorldObjects[index].x = position.x;
		mWorldObjects[index].y = position.y;
	}
	for (size_t index = 0; index < mMercerStock.shards.size(); ++index)
	{
		const WorldPosition& position = mWorld.shardPositions[mMercerStock.shards[index].id];
		mMercerStock.shards[index].mapId = position.mapId;
		mMercerStock.shards[index].x = position.x;
		mMercerStock.shards[index].y = position.y;
	}
	if (placedMissing)
	{
		mWorldBuilderDirty = true;
		mWorldBuilderNotice = "New metadata entities were placed automatically. Review and save.";
		mWorldBuilderNoticeError = false;
		mWorldBuilderNoticeUntil = static_cast<Uint32>(-1);
	}
	return true;
}

// Migration compatibility only. Normal gameplay and the World Builder load
// World/World.json through WorldStorage; the supported migration entry point is
// Tools/convert_legacy_world.py.
bool Application::loadDeprecatedLuaWorldMap(const std::string& path, std::string& error,
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
	auto walkable = [this](const WorldMap& area, int x, int y)
	{
		return worldTileWalkable(area, x, y);
	};
	auto inBounds = [](const WorldMap& area, int x, int y) -> bool
	{
		return area.contains(x, y);
	};

	std::vector<WorldMap> areas;
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
		WorldMap area;
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
		area.columns = (int)columnCount;
		area.rows = rowCount;
		lua_pop(state, 1);
		lua_getfield(state, mapTable, "tile_layers");
		if (!lua_isnil(state, -1) && !lua_istable(state, -1))
		{
			error = "map '" + area.id + "' tile_layers must be a table";
			lua_close(state);
			return false;
		}
		if (lua_istable(state, -1))
		{
			for (size_t layerIndex = 1; layerIndex <= lua_rawlen(state, -1); ++layerIndex)
			{
				lua_rawgeti(state, -1, (lua_Integer)layerIndex);
				int tileTable = lua_gettop(state);
				int x = intField(tileTable, "x");
				int y = intField(tileTable, "y");
				int tileIndex = intField(tileTable, "index");
				std::string familyText = stringField(tileTable, "tileset");
				std::string sheetText = stringField(tileTable, "sheet");
				std::string layerText = stringField(tileTable, "layer");
				RtpTilesetFamily family;
				RtpTileSheet sheet;
				RtpRenderLayer layer;
				const RtpSheetDescriptor* descriptor = NULL;
				bool valid = x >= 0 && y >= 0 && y < (int)area.tiles.size() &&
					x < (int)area.tiles[y].size() && parseFamily(familyText, family) &&
					parseSheet(sheetText, sheet) && parseLayer(layerText, layer);
				if (valid) descriptor = RtpTilesetRenderer::descriptor(family, sheet);
				std::tuple<int, int, int> key(y, x, (int)layer);
				if (!valid || descriptor == NULL || tileIndex < 0 ||
					tileIndex >= descriptor->tileCount || area.tileLayers.count(key) != 0)
				{
					error = "map '" + area.id + "' has invalid tile layer " +
						std::to_string(layerIndex);
					lua_close(state);
					return false;
				}
				area.tileLayers.insert(std::make_pair(key,
					RtpTileReference(family, sheet, tileIndex, layer)));
				lua_pop(state, 1);
			}
		}
		lua_pop(state, 2);
		areas.push_back(area);
	}
	lua_pop(state, 1);
	bool anyCatalogMap = false;
	bool allCatalogMaps = true;
	for (size_t index = 0; index < areas.size(); ++index)
	{
		std::ifstream catalogInput(("World/Maps/" + areas[index].id + ".json").c_str());
		anyCatalogMap = anyCatalogMap || catalogInput.good();
		allCatalogMaps = allCatalogMaps && catalogInput.good();
	}
	if (anyCatalogMap && !allCatalogMaps)
	{
		error = "World/Maps is incomplete; every Lua map needs a catalog JSON map";
		lua_close(state);
		return false;
	}
	if (allCatalogMaps && !CatalogMapStorage::loadMaps("World/Maps", areas, error))
	{
		lua_close(state);
		return false;
	}
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
		!walkable(areas[startArea], startX, startY))
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
			!walkable(areas[fromArea], portal.fromX, portal.fromY) ||
			!walkable(areas[toArea], portal.toX, portal.toY) ||
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
				!walkable(areas[mapIndex], positions[i].x, positions[i].y) ||
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
						if (walkable(areas[map], x, y) &&
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
	WorldData loadedWorld;
	loadedWorld.maps.swap(areas);
	loadedWorld.regions.swap(regions);
	loadedWorld.portals.swap(portals);
	loadedWorld.start = { startMap, startX, startY };
	if (!loadedWorld.validateStructure(error)) return false;
	mWorld.swap(loadedWorld);
	clearWorldBuilderUndoHistory();
	mWorldBuilderSelectedPortal = -1;
	mWorldBuilderPortalEndpoint = 0;
	mWorldBuilderPortalCreating = false;
	mWorldBuilderPortalDraftOrigin = WorldPosition();
	mWorldBuilderSelectedRegion = -1;
	mWorldBuilderRegionPlacementMode = 0;
	mWorldBuilderRegionDragging = false;
	mWorldBuilderRegionNameFocused = false;
	mWorldBuilderRegionNameIndex = -1;
	mWorldBuilderRegionNameInput.clear();
	mWorldBuilderMapDialogOpen = false;
	mWorldBuilderMapDialogField = 0;
	mWorldBuilderMapIdInput.clear();
	mWorldBuilderMapNameInput.clear();
	mWorldBuilderMapWidthInput.clear();
	mWorldBuilderMapHeightInput.clear();
	mWorldBuilderMapIndoorInput = false;
	mWorldBuilderMapDialogError.clear();
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
	return mWorld.maps[mCurrentWorldArea].tiles;
}

const std::vector<std::string>& Application::currentMap() const
{
	return mWorld.maps[mCurrentWorldArea].tiles;
}

const std::string& Application::currentMapId() const
{
	return mWorld.maps[mCurrentWorldArea].id;
}

int Application::worldAreaIndex(const std::string& id) const
{
	return mWorld.mapIndex(id);
}

const WorldRegion* Application::worldRegionAt(const std::string& mapId,
	int x, int y) const
{
	return mWorld.regionAt(mapId, x, y);
}

const WorldRegion* Application::currentWorldRegion() const
{
	return worldRegionAt(currentMapId(), (int)std::round(mVisualX),
		(int)std::round(mVisualY));
}

bool Application::isPortalAt(const std::string& mapId, int x, int y) const
{
	return mWorld.hasPortalEndpoint(mapId, x, y);
}

bool Application::portalOriginBlocksMovement(int x, int y) const
{
	for (size_t index = 0; index < mWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = mWorld.portals[index];
		if (portal.hasAppearance() && portal.fromMap == currentMapId() &&
			portal.fromX == x && portal.fromY == y) return true;
	}
	return false;
}

bool Application::beginPortalAt(int x, int y)
{
	for (size_t i = 0; i < mWorld.portals.size(); ++i)
	{
		const WorldPortal& portal = mWorld.portals[i];
		if (portal.fromMap != currentMapId() || portal.fromX != x || portal.fromY != y)
			continue;
		if (worldAreaIndex(portal.toMap) < 0) return false;
		if (!portal.hasAppearance()) return activatePortalAt(x, y);
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
	for (size_t i = 0; i < mWorld.portals.size(); ++i)
	{
		const WorldPortal& portal = mWorld.portals[i];
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
		mNotice = mWorld.maps[destination].indoor ? "Entered " + mWorld.maps[destination].name + "." :
			(portal.toMap == mWorld.start.mapId ? "Returned to " : "Arrived at ") +
			mWorld.maps[destination].name + ".";
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

bool Application::worldBuilderBrushResizable() const
{
	RtpTilesetFamily family = tilesetFamily(mWorldBuilderTileCategory);
	RtpTileSheet sheet = (RtpTileSheet)mWorldBuilderTileSheet;
	const RtpSheetDescriptor* descriptor = RtpTilesetRenderer::descriptor(family, sheet);
	if (descriptor == NULL || mWorldBuilderCatalogTile < 0 ||
		mWorldBuilderCatalogTile >= descriptor->tileCount) return false;
	RtpTileReference tile(family, sheet,
		RtpTilesetRenderer::canonicalTileIndex(family, sheet,
			mWorldBuilderCatalogTile));
	tile.layer = RtpTilesetRenderer::inferredLayer(tile);
	return tile.layer == RtpRenderLayer::Ground ||
		RtpTilesetRenderer::isCompositeTile(tile);
}

void Application::beginWorldBuilderUndoAction()
{
	if (mWorldBuilderUndoPending) return;
	mWorldBuilderPendingUndo = WorldBuilderUndoAction();
	mWorldBuilderPendingUndo.dirtyBefore = mWorldBuilderDirty;
	mWorldBuilderUndoPending = true;
}

void Application::recordWorldBuilderTileUndo(int x, int y, RtpRenderLayer layer)
{
	if (!mWorldBuilderUndoPending) return;
	std::tuple<int, int, int, int> undoKey(mCurrentWorldArea, y, x, (int)layer);
	if (!mWorldBuilderPendingUndo.tileKeys.insert(undoKey).second) return;
	std::map<std::tuple<int, int, int>, RtpTileReference>& layers =
		mWorld.maps[mCurrentWorldArea].tileLayers;
	std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator previous =
		layers.find(std::make_tuple(y, x, (int)layer));
	bool hadTile = previous != layers.end();
	mWorldBuilderPendingUndo.tiles.push_back({ mCurrentWorldArea, x, y, (int)layer,
		hadTile, hadTile ? previous->second : RtpTileReference(
			RtpTilesetFamily::Outside, RtpTileSheet::A1, 0, layer) });
}

void Application::recordWorldBuilderEntityUndo(int kind, int index,
	const std::string& mapId, int x, int y)
{
	if (!mWorldBuilderUndoPending || mWorldBuilderPendingUndo.entityKind != 0) return;
	mWorldBuilderPendingUndo.entityKind = kind;
	mWorldBuilderPendingUndo.entityIndex = index;
	mWorldBuilderPendingUndo.entityMapId = mapId;
	mWorldBuilderPendingUndo.entityX = x;
	mWorldBuilderPendingUndo.entityY = y;
}

void Application::recordWorldBuilderPortalUndo(int index)
{
	if (!mWorldBuilderUndoPending || mWorldBuilderPendingUndo.entityKind != 0 ||
		index < 0 || index >= (int)mWorld.portals.size()) return;
	mWorldBuilderPendingUndo.entityKind = BUILDER_UNDO_PORTAL;
	mWorldBuilderPendingUndo.entityIndex = index;
	mWorldBuilderPendingUndo.portalSnapshot = mWorld.portals[index];
	mWorldBuilderPendingUndo.hasPortalSnapshot = true;
}

void Application::commitWorldBuilderUndoAction()
{
	if (!mWorldBuilderUndoPending) return;
	auto sameTile = [](const RtpTileReference& first,
		const RtpTileReference& second) -> bool
	{
		return first.family == second.family && first.sheet == second.sheet &&
			first.index == second.index && first.layer == second.layer &&
			first.red == second.red && first.green == second.green &&
			first.blue == second.blue;
	};
	std::vector<WorldBuilderTileUndo> changedTiles;
	for (size_t index = 0; index < mWorldBuilderPendingUndo.tiles.size(); ++index)
	{
		const WorldBuilderTileUndo& undo = mWorldBuilderPendingUndo.tiles[index];
		if (undo.mapIndex < 0 || undo.mapIndex >= (int)mWorld.maps.size()) continue;
		const std::map<std::tuple<int, int, int>, RtpTileReference>& layers =
			mWorld.maps[undo.mapIndex].tileLayers;
		std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator current =
			layers.find(std::make_tuple(undo.y, undo.x, undo.layer));
		bool hasTile = current != layers.end();
		if (hasTile != undo.hadTile ||
			(hasTile && !sameTile(current->second, undo.tile)))
			changedTiles.push_back(undo);
	}
	mWorldBuilderPendingUndo.tiles.swap(changedTiles);
	mWorldBuilderPendingUndo.tileKeys.clear();
	bool entityChanged = false;
	int kind = mWorldBuilderPendingUndo.entityKind;
	int entity = mWorldBuilderPendingUndo.entityIndex;
	if (kind == BUILDER_UNDO_NPC && entity >= 0 && entity < (int)mNpcs.size())
		entityChanged = mNpcs[entity].mapId != mWorldBuilderPendingUndo.entityMapId ||
			mNpcs[entity].x != mWorldBuilderPendingUndo.entityX ||
			mNpcs[entity].y != mWorldBuilderPendingUndo.entityY;
	else if (kind == BUILDER_UNDO_OBJECT && entity >= 0 &&
		entity < (int)mWorldObjects.size())
		entityChanged = mWorldObjects[entity].mapId !=
			mWorldBuilderPendingUndo.entityMapId ||
			mWorldObjects[entity].x != mWorldBuilderPendingUndo.entityX ||
			mWorldObjects[entity].y != mWorldBuilderPendingUndo.entityY;
	else if (kind == BUILDER_UNDO_SHARD && entity >= 0 &&
		entity < (int)mMercerStock.shards.size())
		entityChanged = mMercerStock.shards[entity].mapId !=
			mWorldBuilderPendingUndo.entityMapId ||
			mMercerStock.shards[entity].x != mWorldBuilderPendingUndo.entityX ||
			mMercerStock.shards[entity].y != mWorldBuilderPendingUndo.entityY;
	else if (kind == BUILDER_UNDO_PORTAL &&
		mWorldBuilderPendingUndo.hasPortalSnapshot && entity >= 0 &&
		entity < (int)mWorld.portals.size())
		entityChanged = !samePortal(mWorld.portals[entity],
			mWorldBuilderPendingUndo.portalSnapshot);
	if (mWorldBuilderPendingUndo.tiles.empty() && !entityChanged)
	{
		mWorldBuilderPendingUndo = WorldBuilderUndoAction();
		mWorldBuilderUndoPending = false;
		return;
	}
	if (!entityChanged) mWorldBuilderPendingUndo.entityKind = 0;
	mWorldBuilderUndoHistory.push_back(mWorldBuilderPendingUndo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderPendingUndo = WorldBuilderUndoAction();
	mWorldBuilderUndoPending = false;
}

void Application::undoWorldBuilder()
{
	if (mWorldBuilderMapDialogOpen)
	{
		cancelWorldBuilderMapCreation();
		return;
	}
	if (mWorldBuilderRegionPlacementMode != 0)
	{
		cancelWorldBuilderRegionPlacement();
		return;
	}
	if (mWorldBuilderPortalCreating)
	{
		cancelWorldBuilderPortalCreation();
		return;
	}
	commitWorldBuilderUndoAction();
	mWorldBuilderPainting = false;
	mWorldBuilderErasing = false;
	mWorldBuilderLastBrushX = -1;
	mWorldBuilderLastBrushY = -1;
	mWorldBuilderDragging = false;
	if (mWorldBuilderUndoHistory.empty()) return;
	WorldBuilderUndoAction undo = mWorldBuilderUndoHistory.back();
	mWorldBuilderUndoHistory.pop_back();
	for (size_t index = 0; index < undo.tiles.size(); ++index)
	{
		const WorldBuilderTileUndo& tile = undo.tiles[index];
		if (tile.mapIndex < 0 || tile.mapIndex >= (int)mWorld.maps.size()) continue;
		std::map<std::tuple<int, int, int>, RtpTileReference>& layers =
			mWorld.maps[tile.mapIndex].tileLayers;
		std::tuple<int, int, int> key(tile.y, tile.x, tile.layer);
		layers.erase(key);
		if (tile.hadTile) layers.insert(std::make_pair(key, tile.tile));
	}
	if (undo.entityKind == BUILDER_UNDO_NPC && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mNpcs.size())
	{
		Npc& npc = mNpcs[undo.entityIndex];
		npc.mapId = undo.entityMapId;
		npc.setPosition(undo.entityX, undo.entityY);
	}
	else if (undo.entityKind == BUILDER_UNDO_OBJECT && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorldObjects.size())
	{
		WorldObject& object = mWorldObjects[undo.entityIndex];
		object.mapId = undo.entityMapId;
		object.x = undo.entityX;
		object.y = undo.entityY;
	}
	else if (undo.entityKind == BUILDER_UNDO_OBJECT_APPEARANCE &&
		undo.hasObjectSnapshot && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorldObjects.size())
	{
		mWorldObjects[undo.entityIndex] = undo.objectSnapshot;
		mWorldBuilderSelectedObject = undo.entityIndex;
	}
	else if (undo.entityKind == BUILDER_UNDO_SHARD && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mMercerStock.shards.size())
	{
		MercerShard& shard = mMercerStock.shards[undo.entityIndex];
		shard.mapId = undo.entityMapId;
		shard.x = undo.entityX;
		shard.y = undo.entityY;
	}
	else if (undo.entityKind == BUILDER_UNDO_OBJECT_CREATED &&
		undo.hasObjectSnapshot && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorldObjects.size() &&
		mWorldObjects[undo.entityIndex].id == undo.objectSnapshot.id)
	{
		mWorldObjects.erase(mWorldObjects.begin() + undo.entityIndex);
		if (mWorldBuilderSelectedObject == undo.entityIndex)
			mWorldBuilderSelectedObject = -1;
		else if (mWorldBuilderSelectedObject > undo.entityIndex)
			--mWorldBuilderSelectedObject;
	}
	else if (undo.entityKind == BUILDER_UNDO_OBJECT_DELETED &&
		undo.hasObjectSnapshot && undo.entityIndex >= 0)
	{
		int index = std::min(undo.entityIndex, (int)mWorldObjects.size());
		mWorldObjects.insert(mWorldObjects.begin() + index, undo.objectSnapshot);
		mWorldBuilderSelectedObject = index;
	}
	else if (undo.entityKind == BUILDER_UNDO_PORTAL &&
		undo.hasPortalSnapshot && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorld.portals.size())
	{
		mWorld.portals[undo.entityIndex] = undo.portalSnapshot;
		mWorldBuilderSelectedPortal = undo.entityIndex;
	}
	else if (undo.entityKind == BUILDER_UNDO_PORTAL_CREATED &&
		undo.hasPortalSnapshot && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorld.portals.size() &&
		samePortal(mWorld.portals[undo.entityIndex], undo.portalSnapshot))
	{
		mWorld.portals.erase(mWorld.portals.begin() + undo.entityIndex);
		if (mWorldBuilderSelectedPortal == undo.entityIndex)
			mWorldBuilderSelectedPortal = -1;
		else if (mWorldBuilderSelectedPortal > undo.entityIndex)
			--mWorldBuilderSelectedPortal;
	}
	else if (undo.entityKind == BUILDER_UNDO_PORTAL_DELETED &&
		undo.hasPortalSnapshot && undo.entityIndex >= 0)
	{
		int index = std::min(undo.entityIndex, (int)mWorld.portals.size());
		mWorld.portals.insert(mWorld.portals.begin() + index, undo.portalSnapshot);
		mWorldBuilderSelectedPortal = index;
	}
	else if (undo.entityKind == BUILDER_UNDO_REGION &&
		undo.hasRegionSnapshot && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorld.regions.size())
	{
		mWorld.regions[undo.entityIndex] = undo.regionSnapshot;
		mWorldBuilderSelectedRegion = undo.entityIndex;
		mWorldBuilderRegionNameInput = undo.regionSnapshot.name;
	}
	else if (undo.entityKind == BUILDER_UNDO_REGION_CREATED &&
		undo.hasRegionSnapshot && undo.entityIndex >= 0 &&
		undo.entityIndex < (int)mWorld.regions.size() &&
		sameRegion(mWorld.regions[undo.entityIndex], undo.regionSnapshot))
	{
		mWorld.regions.erase(mWorld.regions.begin() + undo.entityIndex);
		if (mWorldBuilderSelectedRegion == undo.entityIndex)
			mWorldBuilderSelectedRegion = -1;
		else if (mWorldBuilderSelectedRegion > undo.entityIndex)
			--mWorldBuilderSelectedRegion;
	}
	else if (undo.entityKind == BUILDER_UNDO_REGION_DELETED &&
		undo.hasRegionSnapshot && undo.entityIndex >= 0)
	{
		int index = std::min(undo.entityIndex, (int)mWorld.regions.size());
		mWorld.regions.insert(mWorld.regions.begin() + index, undo.regionSnapshot);
		mWorldBuilderSelectedRegion = index;
		mWorldBuilderRegionNameInput = undo.regionSnapshot.name;
	}
	else if (undo.entityKind == BUILDER_UNDO_MAP_CREATED && undo.hasMapSnapshot &&
		undo.entityIndex >= 0 && undo.entityIndex < (int)mWorld.maps.size() &&
		mWorld.maps[undo.entityIndex].id == undo.mapSnapshot.id)
	{
		const std::string& mapId = undo.mapSnapshot.id;
		bool referenced = mWorld.start.mapId == mapId;
		for (size_t index = 0; index < mWorld.regions.size() && !referenced; ++index)
			referenced = mWorld.regions[index].mapId == mapId;
		for (size_t index = 0; index < mWorld.portals.size() && !referenced; ++index)
			referenced = mWorld.portals[index].fromMap == mapId ||
				mWorld.portals[index].toMap == mapId;
		for (size_t index = 0; index < mNpcs.size() && !referenced; ++index)
			referenced = mNpcs[index].mapId == mapId;
		for (size_t index = 0; index < mWorldObjects.size() && !referenced; ++index)
			referenced = mWorldObjects[index].mapId == mapId;
		for (size_t index = 0; index < mMercerStock.shards.size() && !referenced; ++index)
			referenced = mMercerStock.shards[index].mapId == mapId;
		if (referenced)
		{
			mWorldBuilderUndoHistory.push_back(undo);
			showWorldBuilderNotice(
				"Move or remove everything that references this map before undoing it.", true);
			return;
		}
		mWorld.maps.erase(mWorld.maps.begin() + undo.entityIndex);
		mCurrentWorldArea = std::max(0, std::min(undo.selectedMapBefore,
			(int)mWorld.maps.size() - 1));
		mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
	}
	mWorldBuilderDirty = undo.dirtyBefore;
	showWorldBuilderNotice("Undid the last editor action.");
}

void Application::clearWorldBuilderUndoHistory()
{
	mWorldBuilderPendingUndo = WorldBuilderUndoAction();
	mWorldBuilderUndoPending = false;
	mWorldBuilderUndoHistory.clear();
}

void Application::applyWorldBuilderBrush(int x, int y, bool erasing)
{
	int brushSize = worldBuilderBrushResizable() ? mWorldBuilderBrushSize : 1;
	int firstX = x - (brushSize - 1) / 2;
	int firstY = y - (brushSize - 1) / 2;
	for (int brushY = firstY; brushY < firstY + brushSize; ++brushY)
		for (int brushX = firstX; brushX < firstX + brushSize; ++brushX)
		{
			if (erasing) eraseWorldBuilderTile(brushX, brushY);
			else paintWorldBuilderTile(brushX, brushY);
		}
}

bool Application::worldBuilderCanPlace(int x, int y, int ignoredNpc, int ignoredObject) const
{
	if (!isWalkable(x, y) ||
		(currentMapId() == mWorld.start.mapId && x == mWorld.start.x && y == mWorld.start.y) ||
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

bool Application::worldBuilderCanPlacePortal(int x, int y, int ignoredPortal,
	bool ignoredFromEndpoint) const
{
	if (!isWalkable(x, y) ||
		(currentMapId() == mWorld.start.mapId && x == mWorld.start.x &&
			y == mWorld.start.y)) return false;
	for (size_t index = 0; index < mWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = mWorld.portals[index];
		if (!((int)index == ignoredPortal && ignoredFromEndpoint) &&
			portal.fromMap == currentMapId() && portal.fromX == x && portal.fromY == y)
			return false;
		if (!((int)index == ignoredPortal && !ignoredFromEndpoint) &&
			portal.toMap == currentMapId() && portal.toX == x && portal.toY == y)
			return false;
	}
	for (size_t index = 0; index < mNpcs.size(); ++index)
		if (mNpcs[index].mapId == currentMapId() && mNpcs[index].x == x &&
			mNpcs[index].y == y) return false;
	for (size_t index = 0; index < mWorldObjects.size(); ++index)
		if (mWorldObjects[index].mapId == currentMapId() &&
			mWorldObjects[index].x == x && mWorldObjects[index].y == y) return false;
	for (size_t index = 0; index < mMercerStock.shards.size(); ++index)
		if (mMercerStock.shards[index].mapId == currentMapId() &&
			mMercerStock.shards[index].x == x &&
			mMercerStock.shards[index].y == y) return false;
	return true;
}

bool Application::addWorldBuilderObject(int templateIndex, int x, int y)
{
	if (templateIndex < 0 || templateIndex >= (int)mWorldObjectTemplates.size())
		return false;
	if (!worldBuilderCanPlace(x, y, -1, -1))
	{
		showWorldBuilderNotice("Objects require an empty walkable tile.", true);
		return false;
	}
	commitWorldBuilderUndoAction();
	const WorldObjectTemplate& objectTemplate = mWorldObjectTemplates[templateIndex];
	const unsigned long long createdAt = (unsigned long long)
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	const std::string idStem = objectTemplate.id + "_" + std::to_string(createdAt);
	std::string id = idStem;
	int suffix = 2;
	for (;;)
	{
		bool used = false;
		for (size_t index = 0; index < mWorldObjects.size(); ++index)
			if (mWorldObjects[index].id == id) { used = true; break; }
		if (!used) break;
		id = idStem + "_" + std::to_string(suffix++);
	}
	WorldObject object = createWorldObject(objectTemplate, id);
	object.mapId = currentMapId();
	object.x = x;
	object.y = y;
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_OBJECT_CREATED;
	undo.entityIndex = (int)mWorldObjects.size();
	undo.objectSnapshot = object;
	undo.hasObjectSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	mWorldObjects.push_back(object);
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderSelectedObject = (int)mWorldObjects.size() - 1;
	mWorldBuilderDirty = true;
	showWorldBuilderNotice("Added " + object.name + ".");
	return true;
}

void Application::deleteWorldBuilderObject()
{
	commitWorldBuilderUndoAction();
	if (mWorldBuilderSelectedObject < 0 ||
		mWorldBuilderSelectedObject >= (int)mWorldObjects.size()) return;
	const WorldObject& object = mWorldObjects[mWorldBuilderSelectedObject];
	if (!object.editorCreated)
	{
		showWorldBuilderNotice("Lua-authored objects cannot be deleted here.", true);
		return;
	}
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_OBJECT_DELETED;
	undo.entityIndex = mWorldBuilderSelectedObject;
	undo.objectSnapshot = object;
	undo.hasObjectSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	const std::string name = object.name;
	mWorldObjects.erase(mWorldObjects.begin() + mWorldBuilderSelectedObject);
	mWorldBuilderSelectedObject = -1;
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderDirty = true;
	showWorldBuilderNotice("Removed " + name + ". Ctrl+Z restores it.");
}

void Application::cycleWorldBuilderChestAppearance(int direction)
{
	if (mWorldBuilderObjectPalette || mWorldBuilderSelectedObject < 0 ||
		mWorldBuilderSelectedObject >= (int)mWorldObjects.size() || direction == 0) return;
	WorldObject& object = mWorldObjects[mWorldBuilderSelectedObject];
	if (object.kind != WorldObjectKind::Chest && object.kind != WorldObjectKind::DeckChest)
		return;
	int current = -1;
	for (int index = 0; index < CHEST_APPEARANCE_COUNT; ++index)
		if (object.appearance == CHEST_APPEARANCES[index])
		{
			current = index;
			break;
		}
	int next = current < 0 ? 0 :
		(current + (direction < 0 ? -1 : 1) + CHEST_APPEARANCE_COUNT) %
		CHEST_APPEARANCE_COUNT;
	commitWorldBuilderUndoAction();
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_OBJECT_APPEARANCE;
	undo.entityIndex = mWorldBuilderSelectedObject;
	undo.objectSnapshot = object;
	undo.hasObjectSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	if (!setWorldObjectAppearance(object, CHEST_APPEARANCES[next])) return;
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderDirty = true;
	showWorldBuilderNotice("Chest appearance set to " + object.appearance + ".");
}

void Application::beginWorldBuilderMapCreation()
{
	if (mWorldBuilderRegionPlacementMode != 0 || mWorldBuilderPortalCreating)
	{
		showWorldBuilderNotice(
			"Finish or cancel the current region or portal before adding a map.", true);
		return;
	}
	commitWorldBuilderRegionNameEdit(true);
	if (mWorldBuilderRegionNameFocused) return;
	commitWorldBuilderUndoAction();
	std::string id = "new_map";
	for (int suffix = 2; mWorld.mapIndex(id) >= 0; ++suffix)
		id = "new_map_" + std::to_string(suffix);
	mWorldBuilderMapDialogOpen = true;
	mWorldBuilderMapDialogField = 0;
	mWorldBuilderMapIdInput = id;
	mWorldBuilderMapNameInput = "New Map";
	mWorldBuilderMapWidthInput = "32";
	mWorldBuilderMapHeightInput = "24";
	mWorldBuilderMapIndoorInput = false;
	mWorldBuilderMapDialogError.clear();
	mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
	mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
	mWorldBuilderPanAccumulator = 0;
	SDL_StartTextInput();
}

void Application::cancelWorldBuilderMapCreation()
{
	if (!mWorldBuilderMapDialogOpen) return;
	mWorldBuilderMapDialogOpen = false;
	mWorldBuilderMapDialogError.clear();
	SDL_StopTextInput();
}

bool Application::createWorldBuilderMap(std::string& error)
{
	error.clear();
	if (!mWorldBuilderMapDialogOpen)
	{
		error = "the new-map dialog is not open";
		return false;
	}
	auto trim = [](const std::string& input) -> std::string
	{
		size_t first = input.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) return "";
		size_t last = input.find_last_not_of(" \t\r\n");
		return input.substr(first, last - first + 1);
	};
	const std::string id = trim(mWorldBuilderMapIdInput);
	const std::string name = trim(mWorldBuilderMapNameInput);
	int width = 0;
	int height = 0;
	if (!validBuilderMapId(id))
	{
		error = "Map IDs may contain only letters, numbers, underscores, and hyphens.";
		return false;
	}
	if (mWorld.mapIndex(id) >= 0)
	{
		error = "A map with ID '" + id + "' already exists.";
		return false;
	}
	std::ifstream existingMap(("World/Maps/" + id + ".json").c_str(),
		std::ios::binary);
	if (existingMap)
	{
		error = "World/Maps/" + id + ".json already exists outside the manifest.";
		return false;
	}
	if (name.empty())
	{
		error = "Map names cannot be empty.";
		return false;
	}
	if (!builderMapDimension(mWorldBuilderMapWidthInput, width) ||
		!builderMapDimension(mWorldBuilderMapHeightInput, height))
	{
		error = "Map width and height must be between 1 and 1024.";
		return false;
	}

	WorldMap map;
	map.id = id;
	map.name = name;
	map.indoor = mWorldBuilderMapIndoorInput;
	map.columns = width;
	map.rows = height;
	map.catalogOnly = true;
	map.tiles.assign(height, std::string(width, '.'));
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_MAP_CREATED;
	undo.entityIndex = (int)mWorld.maps.size();
	undo.mapSnapshot = map;
	undo.hasMapSnapshot = true;
	undo.selectedMapBefore = mCurrentWorldArea;
	undo.dirtyBefore = mWorldBuilderDirty;
	mWorld.maps.push_back(map);
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mCurrentWorldArea = (int)mWorld.maps.size() - 1;
	mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
	mWorldBuilderTileCategory = map.indoor ? 1 : 2;
	mWorldBuilderTileSheet = (int)RtpTileSheet::A2;
	mWorldBuilderCatalogTile = 0;
	mWorldBuilderListScroll = 0;
	mWorldBuilderDirty = true;
	mWorldBuilderMapDialogOpen = false;
	mWorldBuilderMapDialogError.clear();
	SDL_StopTextInput();
	showWorldBuilderNotice("Added map '" + name + "'. Paint walkable tiles before "
		"placing entities or portals.");
	return true;
}

bool Application::handleWorldBuilderMapDialogEvent(const SDL_Event& event)
{
	if (!mWorldBuilderMapDialogOpen) return false;
	if (event.type == SDL_TEXTINPUT)
	{
		std::string* input = mWorldBuilderMapDialogField == 0 ?
			&mWorldBuilderMapIdInput : (mWorldBuilderMapDialogField == 1 ?
			&mWorldBuilderMapNameInput : (mWorldBuilderMapDialogField == 2 ?
			&mWorldBuilderMapWidthInput : &mWorldBuilderMapHeightInput));
		const size_t limit = mWorldBuilderMapDialogField < 2 ? 64 : 4;
		for (const char* character = event.text.text; *character != '\0' &&
			input->size() < limit; ++character)
		{
			if (mWorldBuilderMapDialogField >= 2 &&
				(*character < '0' || *character > '9')) continue;
			if (mWorldBuilderMapDialogField == 0)
			{
				unsigned char value = (unsigned char)*character;
				if (!((value >= 'a' && value <= 'z') ||
					(value >= 'A' && value <= 'Z') ||
					(value >= '0' && value <= '9') || value == '_' || value == '-'))
					continue;
			}
			input->push_back(*character);
		}
		mWorldBuilderMapDialogError.clear();
		return true;
	}
	if (event.type == SDL_KEYDOWN)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			cancelWorldBuilderMapCreation();
			return true;
		}
		if (key == SDLK_TAB)
		{
			int direction = (event.key.keysym.mod & KMOD_SHIFT) ? -1 : 1;
			mWorldBuilderMapDialogField =
				(mWorldBuilderMapDialogField + direction + 4) % 4;
			return true;
		}
		if (key == SDLK_BACKSPACE)
		{
			std::string* input = mWorldBuilderMapDialogField == 0 ?
				&mWorldBuilderMapIdInput : (mWorldBuilderMapDialogField == 1 ?
				&mWorldBuilderMapNameInput : (mWorldBuilderMapDialogField == 2 ?
				&mWorldBuilderMapWidthInput : &mWorldBuilderMapHeightInput));
			if (!input->empty()) input->pop_back();
			mWorldBuilderMapDialogError.clear();
			return true;
		}
		if ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && !event.key.repeat)
		{
			std::string error;
			if (!createWorldBuilderMap(error)) mWorldBuilderMapDialogError = error;
			return true;
		}
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		int x, y;
		logicalMouse(event.button.x, event.button.y, x, y);
		const SDL_Rect fields[] = { BUILDER_MAP_ID, BUILDER_MAP_NAME,
			BUILDER_MAP_WIDTH, BUILDER_MAP_HEIGHT };
		for (int field = 0; field < 4; ++field)
			if (contains(fields[field], x, y))
			{
				mWorldBuilderMapDialogField = field;
				return true;
			}
		if (contains(BUILDER_MAP_INDOOR, x, y))
		{
			mWorldBuilderMapIndoorInput = !mWorldBuilderMapIndoorInput;
			return true;
		}
		if (contains(BUILDER_MAP_CANCEL, x, y))
		{
			cancelWorldBuilderMapCreation();
			return true;
		}
		if (contains(BUILDER_MAP_CREATE, x, y))
		{
			std::string error;
			if (!createWorldBuilderMap(error)) mWorldBuilderMapDialogError = error;
			return true;
		}
	}
	return true;
}

void Application::beginWorldBuilderPortalCreation()
{
	commitWorldBuilderUndoAction();
	mWorldBuilderPortalCreating = true;
	mWorldBuilderPortalEndpoint = 0;
	mWorldBuilderPortalDraftOrigin = WorldPosition();
	mWorldBuilderSelectedPortal = -1;
	showWorldBuilderNotice("Click an empty walkable tile for the portal origin.");
}

void Application::cancelWorldBuilderPortalCreation()
{
	mWorldBuilderPortalCreating = false;
	mWorldBuilderPortalEndpoint = 0;
	mWorldBuilderPortalDraftOrigin = WorldPosition();
	showWorldBuilderNotice("Portal creation cancelled.");
}

void Application::placeWorldBuilderPortalEndpoint(int x, int y)
{
	if (mWorldBuilderPortalCreating)
	{
		if (!worldBuilderCanPlacePortal(x, y, -1, false))
		{
			showWorldBuilderNotice("Portal endpoints require an empty walkable tile.", true);
			return;
		}
		if (mWorldBuilderPortalEndpoint == 0)
		{
			mWorldBuilderPortalDraftOrigin = { currentMapId(), x, y };
			mWorldBuilderPortalEndpoint = 1;
			showWorldBuilderNotice(
				"Origin set. Switch maps if needed, then click the destination.");
			return;
		}
		if (mWorldBuilderPortalDraftOrigin.mapId == currentMapId() &&
			mWorldBuilderPortalDraftOrigin.x == x &&
			mWorldBuilderPortalDraftOrigin.y == y)
		{
			showWorldBuilderNotice("Portal origin and destination must be distinct.", true);
			return;
		}
		WorldPortal portal;
		portal.appearance = DEFAULT_PORTAL_APPEARANCE;
		portal.fromMap = mWorldBuilderPortalDraftOrigin.mapId;
		portal.fromX = mWorldBuilderPortalDraftOrigin.x;
		portal.fromY = mWorldBuilderPortalDraftOrigin.y;
		portal.toMap = currentMapId();
		portal.toX = x;
		portal.toY = y;
		WorldBuilderUndoAction undo;
		undo.entityKind = BUILDER_UNDO_PORTAL_CREATED;
		undo.entityIndex = (int)mWorld.portals.size();
		undo.portalSnapshot = portal;
		undo.hasPortalSnapshot = true;
		undo.dirtyBefore = mWorldBuilderDirty;
		mWorld.portals.push_back(portal);
		mWorldBuilderUndoHistory.push_back(undo);
		if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
			mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
		mWorldBuilderSelectedPortal = (int)mWorld.portals.size() - 1;
		mWorldBuilderPortalCreating = false;
		mWorldBuilderPortalEndpoint = 0;
		mWorldBuilderPortalDraftOrigin = WorldPosition();
		mWorldBuilderDirty = true;
		showWorldBuilderNotice(
			"Directed portal added. Add a reverse portal separately if needed.");
		return;
	}

	if (mWorldBuilderSelectedPortal < 0 ||
		mWorldBuilderSelectedPortal >= (int)mWorld.portals.size()) return;
	bool fromEndpoint = mWorldBuilderPortalEndpoint == 0;
	if (!worldBuilderCanPlacePortal(x, y, mWorldBuilderSelectedPortal, fromEndpoint))
	{
		showWorldBuilderNotice("Portal endpoints require an empty walkable tile.", true);
		return;
	}
	WorldPortal& portal = mWorld.portals[mWorldBuilderSelectedPortal];
	std::string& mapId = fromEndpoint ? portal.fromMap : portal.toMap;
	int& portalX = fromEndpoint ? portal.fromX : portal.toX;
	int& portalY = fromEndpoint ? portal.fromY : portal.toY;
	if (mapId == currentMapId() && portalX == x && portalY == y) return;
	recordWorldBuilderPortalUndo(mWorldBuilderSelectedPortal);
	mapId = currentMapId();
	portalX = x;
	portalY = y;
	mWorldBuilderDirty = true;
}

void Application::deleteWorldBuilderPortal()
{
	if (mWorldBuilderPortalCreating)
	{
		cancelWorldBuilderPortalCreation();
		return;
	}
	commitWorldBuilderUndoAction();
	if (mWorldBuilderSelectedPortal < 0 ||
		mWorldBuilderSelectedPortal >= (int)mWorld.portals.size()) return;
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_PORTAL_DELETED;
	undo.entityIndex = mWorldBuilderSelectedPortal;
	undo.portalSnapshot = mWorld.portals[mWorldBuilderSelectedPortal];
	undo.hasPortalSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	mWorld.portals.erase(mWorld.portals.begin() + mWorldBuilderSelectedPortal);
	mWorldBuilderSelectedPortal = -1;
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderDirty = true;
	showWorldBuilderNotice("Portal removed. Ctrl+Z restores it.");
}

void Application::cycleWorldBuilderPortalAppearance(int direction)
{
	if (mWorldBuilderPortalCreating || mWorldBuilderSelectedPortal < 0 ||
		mWorldBuilderSelectedPortal >= (int)mWorld.portals.size() || direction == 0) return;
	WorldPortal& portal = mWorld.portals[mWorldBuilderSelectedPortal];
	int current = -1;
	for (int index = 0; index < PORTAL_APPEARANCE_COUNT; ++index)
		if (portal.appearance == PORTAL_APPEARANCES[index])
		{
			current = index;
			break;
		}
	int next = current < 0 ? 0 :
		(current + (direction < 0 ? -1 : 1) + PORTAL_APPEARANCE_COUNT) %
		PORTAL_APPEARANCE_COUNT;
	beginWorldBuilderUndoAction();
	recordWorldBuilderPortalUndo(mWorldBuilderSelectedPortal);
	portal.appearance = PORTAL_APPEARANCES[next];
	mWorldBuilderDirty = true;
	commitWorldBuilderUndoAction();
	showWorldBuilderNotice("Portal appearance set to " +
		portalAppearanceLabel(portal.appearance) + ".");
}

bool Application::worldBuilderCanPlaceRegion(const WorldRegion& region,
	int ignoredRegion, std::string& error) const
{
	error.clear();
	const WorldMap* map = mWorld.map(region.mapId);
	if (map == NULL || map->indoor)
	{
		error = "Regions can be drawn only on exterior maps.";
		return false;
	}
	if (region.width <= 0 || region.height <= 0 ||
		!map->contains(region.x, region.y) ||
		!map->contains(region.x + region.width - 1,
			region.y + region.height - 1))
	{
		error = "Region bounds must stay inside the current map.";
		return false;
	}
	for (size_t index = 0; index < mWorld.regions.size(); ++index)
	{
		if ((int)index == ignoredRegion) continue;
		const WorldRegion& other = mWorld.regions[index];
		bool overlaps = other.mapId == region.mapId &&
			region.x < other.x + other.width && region.x + region.width > other.x &&
			region.y < other.y + other.height && region.y + region.height > other.y;
		if (overlaps)
		{
			error = "Region bounds cannot overlap " + other.name + ".";
			return false;
		}
	}
	return true;
}

void Application::beginWorldBuilderRegionCreation()
{
	commitWorldBuilderRegionNameEdit(true);
	if (mWorld.maps[mCurrentWorldArea].indoor)
	{
		showWorldBuilderNotice("Regions can be drawn only on exterior maps.", true);
		return;
	}
	if (mWorldBuilderPortalCreating) cancelWorldBuilderPortalCreation();
	mWorldBuilderRegionPlacementMode = 1;
	mWorldBuilderRegionDragging = false;
	mWorldBuilderSelectedRegion = -1;
	mWorldBuilderRegionDraft = WorldRegion();
	showWorldBuilderNotice("Drag across the map to draw the new region rectangle.");
}

void Application::beginWorldBuilderRegionBounds()
{
	commitWorldBuilderRegionNameEdit(true);
	if (mWorldBuilderSelectedRegion < 0 ||
		mWorldBuilderSelectedRegion >= (int)mWorld.regions.size()) return;
	if (mWorld.maps[mCurrentWorldArea].indoor)
	{
		showWorldBuilderNotice("Regions can be drawn only on exterior maps.", true);
		return;
	}
	mWorldBuilderRegionPlacementMode = 2;
	mWorldBuilderRegionDragging = false;
	mWorldBuilderRegionDraft = mWorld.regions[mWorldBuilderSelectedRegion];
	showWorldBuilderNotice("Drag across the map to replace the selected region bounds.");
}

void Application::cancelWorldBuilderRegionPlacement()
{
	mWorldBuilderRegionPlacementMode = 0;
	mWorldBuilderRegionDragging = false;
	mWorldBuilderRegionDragStartX = -1;
	mWorldBuilderRegionDragStartY = -1;
	mWorldBuilderRegionDraft = WorldRegion();
	showWorldBuilderNotice("Region placement cancelled.");
}

void Application::updateWorldBuilderRegionDraft(int x, int y)
{
	if (!mWorldBuilderRegionDragging) return;
	int left = std::min(mWorldBuilderRegionDragStartX, x);
	int top = std::min(mWorldBuilderRegionDragStartY, y);
	int right = std::max(mWorldBuilderRegionDragStartX, x);
	int bottom = std::max(mWorldBuilderRegionDragStartY, y);
	mWorldBuilderRegionDraft.mapId = currentMapId();
	mWorldBuilderRegionDraft.x = left;
	mWorldBuilderRegionDraft.y = top;
	mWorldBuilderRegionDraft.width = right - left + 1;
	mWorldBuilderRegionDraft.height = bottom - top + 1;
}

void Application::finishWorldBuilderRegionPlacement()
{
	if (!mWorldBuilderRegionDragging || mWorldBuilderRegionPlacementMode == 0) return;
	mWorldBuilderRegionDragging = false;
	std::string error;
	int ignoredRegion = mWorldBuilderRegionPlacementMode == 2 ?
		mWorldBuilderSelectedRegion : -1;
	if (!worldBuilderCanPlaceRegion(mWorldBuilderRegionDraft, ignoredRegion, error))
	{
		showWorldBuilderNotice(error, true);
		return;
	}
	bool changed = false;
	if (mWorldBuilderRegionPlacementMode == 1)
	{
		int suffix = 1;
		std::string id;
		for (;; ++suffix)
		{
			id = "region_" + std::to_string(suffix);
			bool used = false;
			for (size_t index = 0; index < mWorld.regions.size(); ++index)
				if (mWorld.regions[index].id == id) { used = true; break; }
			if (!used) break;
		}
		WorldRegion region = mWorldBuilderRegionDraft;
		region.id = id;
		region.name = "New Region " + std::to_string(suffix);
		region.connector = false;
		WorldBuilderUndoAction undo;
		undo.entityKind = BUILDER_UNDO_REGION_CREATED;
		undo.entityIndex = (int)mWorld.regions.size();
		undo.regionSnapshot = region;
		undo.hasRegionSnapshot = true;
		undo.dirtyBefore = mWorldBuilderDirty;
		mWorld.regions.push_back(region);
		mWorldBuilderUndoHistory.push_back(undo);
		mWorldBuilderSelectedRegion = (int)mWorld.regions.size() - 1;
		mWorldBuilderRegionNameInput = region.name;
		changed = true;
		showWorldBuilderNotice("Region added. Click its name field to rename it.");
	}
	else if (mWorldBuilderSelectedRegion >= 0 &&
		mWorldBuilderSelectedRegion < (int)mWorld.regions.size())
	{
		WorldRegion& region = mWorld.regions[mWorldBuilderSelectedRegion];
		WorldBuilderUndoAction undo;
		undo.entityKind = BUILDER_UNDO_REGION;
		undo.entityIndex = mWorldBuilderSelectedRegion;
		undo.regionSnapshot = region;
		undo.hasRegionSnapshot = true;
		undo.dirtyBefore = mWorldBuilderDirty;
		region.mapId = mWorldBuilderRegionDraft.mapId;
		region.x = mWorldBuilderRegionDraft.x;
		region.y = mWorldBuilderRegionDraft.y;
		region.width = mWorldBuilderRegionDraft.width;
		region.height = mWorldBuilderRegionDraft.height;
		if (!sameRegion(region, undo.regionSnapshot))
		{
			mWorldBuilderUndoHistory.push_back(undo);
			changed = true;
			showWorldBuilderNotice("Region bounds updated.");
		}
		else showWorldBuilderNotice("Region bounds are unchanged.");
	}
	if (changed && (int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	if (changed) mWorldBuilderDirty = true;
	mWorldBuilderRegionPlacementMode = 0;
	mWorldBuilderRegionDraft = WorldRegion();
}

void Application::deleteWorldBuilderRegion()
{
	commitWorldBuilderRegionNameEdit(true);
	if (mWorldBuilderRegionPlacementMode != 0)
	{
		cancelWorldBuilderRegionPlacement();
		return;
	}
	if (mWorldBuilderSelectedRegion < 0 ||
		mWorldBuilderSelectedRegion >= (int)mWorld.regions.size()) return;
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_REGION_DELETED;
	undo.entityIndex = mWorldBuilderSelectedRegion;
	undo.regionSnapshot = mWorld.regions[mWorldBuilderSelectedRegion];
	undo.hasRegionSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	const std::string name = undo.regionSnapshot.name;
	mWorld.regions.erase(mWorld.regions.begin() + mWorldBuilderSelectedRegion);
	mWorldBuilderSelectedRegion = -1;
	mWorldBuilderRegionNameInput.clear();
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderDirty = true;
	showWorldBuilderNotice("Removed " + name + ". Ctrl+Z restores it.");
}

void Application::toggleWorldBuilderRegionKind()
{
	commitWorldBuilderRegionNameEdit(true);
	if (mWorldBuilderSelectedRegion < 0 ||
		mWorldBuilderSelectedRegion >= (int)mWorld.regions.size()) return;
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_REGION;
	undo.entityIndex = mWorldBuilderSelectedRegion;
	undo.regionSnapshot = mWorld.regions[mWorldBuilderSelectedRegion];
	undo.hasRegionSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	WorldRegion& region = mWorld.regions[mWorldBuilderSelectedRegion];
	region.connector = !region.connector;
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderDirty = true;
	showWorldBuilderNotice(region.name + " is now a " +
		(region.connector ? "connector region." : "town region."));
}

void Application::toggleWorldBuilderRegionWeather()
{
	commitWorldBuilderRegionNameEdit(true);
	if (mWorldBuilderSelectedRegion < 0 ||
		mWorldBuilderSelectedRegion >= (int)mWorld.regions.size()) return;
	WorldBuilderUndoAction undo;
	undo.entityKind = BUILDER_UNDO_REGION;
	undo.entityIndex = mWorldBuilderSelectedRegion;
	undo.regionSnapshot = mWorld.regions[mWorldBuilderSelectedRegion];
	undo.hasRegionSnapshot = true;
	undo.dirtyBefore = mWorldBuilderDirty;
	WorldRegion& region = mWorld.regions[mWorldBuilderSelectedRegion];
	region.snow = !region.snow;
	mWorldBuilderUndoHistory.push_back(undo);
	if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
		mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
	mWorldBuilderDirty = true;
	showWorldBuilderNotice(region.name + " precipitation changed to " +
		(region.snow ? "snow." : "rain."));
}

void Application::beginWorldBuilderRegionNameEdit()
{
	if (mWorldBuilderSelectedRegion < 0 ||
		mWorldBuilderSelectedRegion >= (int)mWorld.regions.size()) return;
	mWorldBuilderRegionNameFocused = true;
	mWorldBuilderRegionNameIndex = mWorldBuilderSelectedRegion;
	mWorldBuilderRegionNameInput = mWorld.regions[mWorldBuilderSelectedRegion].name;
	SDL_StartTextInput();
}

void Application::commitWorldBuilderRegionNameEdit(bool accept)
{
	if (!mWorldBuilderRegionNameFocused) return;
	if (accept)
	{
		size_t first = mWorldBuilderRegionNameInput.find_first_not_of(" \t\r\n");
		size_t last = mWorldBuilderRegionNameInput.find_last_not_of(" \t\r\n");
		if (first == std::string::npos)
		{
			showWorldBuilderNotice("Region names cannot be empty.", true);
			return;
		}
		mWorldBuilderRegionNameInput =
			mWorldBuilderRegionNameInput.substr(first, last - first + 1);
	}
	if (mWorldBuilderRegionNameIndex >= 0 &&
		mWorldBuilderRegionNameIndex < (int)mWorld.regions.size())
	{
		WorldRegion& region = mWorld.regions[mWorldBuilderRegionNameIndex];
		if (accept && region.name != mWorldBuilderRegionNameInput)
		{
			WorldBuilderUndoAction undo;
			undo.entityKind = BUILDER_UNDO_REGION;
			undo.entityIndex = mWorldBuilderRegionNameIndex;
			undo.regionSnapshot = region;
			undo.hasRegionSnapshot = true;
			undo.dirtyBefore = mWorldBuilderDirty;
			region.name = mWorldBuilderRegionNameInput;
			mWorldBuilderUndoHistory.push_back(undo);
			if ((int)mWorldBuilderUndoHistory.size() > BUILDER_MAX_UNDO_ACTIONS)
				mWorldBuilderUndoHistory.erase(mWorldBuilderUndoHistory.begin());
			mWorldBuilderDirty = true;
		}
		else if (!accept)
			mWorldBuilderRegionNameInput = region.name;
	}
	mWorldBuilderRegionNameFocused = false;
	mWorldBuilderRegionNameIndex = -1;
	SDL_StopTextInput();
}

void Application::paintWorldBuilderTile(int x, int y)
{
	const std::vector<std::string>& map = currentMap();
	if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size()) return;
	RtpTilesetFamily family = tilesetFamily(mWorldBuilderTileCategory);
	RtpTileSheet sheet = (RtpTileSheet)mWorldBuilderTileSheet;
	const RtpSheetDescriptor* descriptor = RtpTilesetRenderer::descriptor(family, sheet);
	if (descriptor == NULL || mWorldBuilderCatalogTile < 0 ||
		mWorldBuilderCatalogTile >= descriptor->tileCount) return;
	int canonicalIndex = RtpTilesetRenderer::canonicalTileIndex(family, sheet,
		mWorldBuilderCatalogTile);
	RtpTileReference tile(family, sheet, canonicalIndex);
	tile.layer = RtpTilesetRenderer::inferredLayer(tile);
	int footprintWidth = 0;
	int footprintHeight = 0;
	if (RtpTilesetRenderer::compositeTileFootprint(tile, footprintWidth,
		footprintHeight) &&
		(x < footprintWidth - 1 || y < footprintHeight - 1))
	{
		showWorldBuilderNotice("The complete composite tile does not fit at the map edge.",
			true);
		return;
	}
	std::tuple<int, int, int> key(y, x, (int)tile.layer);
	std::map<std::tuple<int, int, int>, RtpTileReference>& layers =
		mWorld.maps[mCurrentWorldArea].tileLayers;
	if (footprintWidth == 2)
	{
		for (int checkY = y - 1; checkY <= y + 1; ++checkY)
			for (int checkX = x - 1; checkX <= x + 1; ++checkX)
			{
				if ((checkX == x && checkY == y) || checkX < 0 || checkY < 0 ||
					checkX >= (int)map[0].size() || checkY >= (int)map.size()) continue;
				std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator other =
					layers.find(std::make_tuple(checkY, checkX,
						(int)RtpRenderLayer::Decoration));
				if (other != layers.end() &&
					RtpTilesetRenderer::largeTreeAnchorsConflict(tile, x, y,
						other->second, checkX, checkY))
				{
					showWorldBuilderNotice(
						"Large tree trunks must be at least two tiles apart horizontally.",
						true);
					return;
				}
			}
	}
	std::map<std::tuple<int, int, int>, RtpTileReference>::iterator existing =
		layers.find(key);
	if (existing != layers.end() && existing->second.family == tile.family &&
		existing->second.sheet == tile.sheet && existing->second.index == tile.index) return;
	bool hadExisting = existing != layers.end();
	RtpTileReference previous = hadExisting ? existing->second : tile;
	recordWorldBuilderTileUndo(x, y, tile.layer);
	if (existing != layers.end()) layers.erase(existing);
	layers.insert(std::make_pair(key, tile));
	if (worldBuilderRequiresWalkable(x, y) &&
		!worldTileWalkable(mWorld.maps[mCurrentWorldArea], x, y))
	{
		layers.erase(key);
		if (hadExisting) layers.insert(std::make_pair(key, previous));
		showWorldBuilderNotice("Move the entity or portal before blocking this tile.", true);
		return;
	}
	mWorldBuilderDirty = true;
}

void Application::eraseWorldBuilderTile(int x, int y)
{
	const std::vector<std::string>& map = currentMap();
	if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size()) return;
	std::map<std::tuple<int, int, int>, RtpTileReference>& layers =
		mWorld.maps[mCurrentWorldArea].tileLayers;
	RtpTilesetFamily selectedFamily = tilesetFamily(mWorldBuilderTileCategory);
	RtpTileSheet selectedSheet = (RtpTileSheet)mWorldBuilderTileSheet;
	RtpTileReference selected(selectedFamily, selectedSheet,
		RtpTilesetRenderer::canonicalTileIndex(selectedFamily, selectedSheet,
			mWorldBuilderCatalogTile));
	selected.layer = RtpTilesetRenderer::inferredLayer(selected);
	std::tuple<int, int, int> key(y, x, (int)selected.layer);
	std::map<std::tuple<int, int, int>, RtpTileReference>::iterator existing =
		layers.find(key);
	if (existing == layers.end()) return;
	RtpTileReference previous = existing->second;
	recordWorldBuilderTileUndo(x, y, selected.layer);
	layers.erase(existing);
	if (worldBuilderRequiresWalkable(x, y) &&
		!worldTileWalkable(mWorld.maps[mCurrentWorldArea], x, y))
	{
		layers.insert(std::make_pair(key, previous));
		showWorldBuilderNotice("This layer keeps an entity or portal tile walkable.", true);
		return;
	}
	mWorldBuilderDirty = true;
}

void Application::applyWorldBuilderBrushStroke(int fromX, int fromY,
	int toX, int toY, bool erasing)
{
	if (fromX < 0 || fromY < 0)
	{
		applyWorldBuilderBrush(toX, toY, erasing);
		return;
	}
	int x = fromX;
	int y = fromY;
	int dx = std::abs(toX - fromX);
	int stepX = fromX < toX ? 1 : -1;
	int dy = -std::abs(toY - fromY);
	int stepY = fromY < toY ? 1 : -1;
	int error = dx + dy;
	for (;;)
	{
		applyWorldBuilderBrush(x, y, erasing);
		if (x == toX && y == toY) break;
		int doubledError = error * 2;
		if (doubledError >= dy) { error += dy; x += stepX; }
		if (doubledError <= dx) { error += dx; y += stepY; }
	}
}

bool Application::worldBuilderRequiresWalkable(int x, int y) const
{
	if ((currentMapId() == mWorld.start.mapId && x == mWorld.start.x && y == mWorld.start.y) ||
		isPortalAt(currentMapId(), x, y)) return true;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].mapId == currentMapId() && mNpcs[i].x == x && mNpcs[i].y == y)
			return true;
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
		if (mWorldObjects[i].mapId == currentMapId() &&
			mWorldObjects[i].x == x && mWorldObjects[i].y == y) return true;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		if (mMercerStock.shards[i].mapId == currentMapId() &&
			mMercerStock.shards[i].x == x && mMercerStock.shards[i].y == y) return true;
	return false;
}

const RtpTileReference* Application::worldTileLayer(const WorldMap& area,
	int x, int y, RtpRenderLayer layer) const
{
	std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator found =
		area.tileLayers.find(std::make_tuple(y, x, (int)layer));
	return found == area.tileLayers.end() ? NULL : &found->second;
}

bool Application::worldTileWalkable(const WorldMap& area, int x, int y) const
{
	if (y < 0 || y >= (int)area.tiles.size() || x < 0 ||
		x >= (int)area.tiles[y].size()) return false;
	const RtpRenderLayer layers[] = { RtpRenderLayer::Foreground,
		RtpRenderLayer::Decoration, RtpRenderLayer::Ground };
	for (int layer = 0; layer < 3; ++layer)
	{
		const RtpTileReference* tile = worldTileLayer(area, x, y, layers[layer]);
		if (tile == NULL) continue;
		RtpTileCollision collision = RtpTilesetRenderer::collision(*tile);
		if (collision == RtpTileCollision::Walkable) return true;
		if (collision == RtpTileCollision::Blocked) return false;
	}
	if (area.catalogOnly) return false;
	return WorldTiles::isWalkable(WorldTiles::fromGlyph(area.tiles[y][x]));
}

unsigned int Application::worldTileConnections(const WorldMap& area, int x, int y,
	RtpRenderLayer layer) const
{
	const RtpTileReference* tile = worldTileLayer(area, x, y, layer);
	if (tile == NULL) return 0;
	auto matches = [&](int checkX, int checkY) -> bool
	{
		const RtpTileReference* other = worldTileLayer(area, checkX, checkY, layer);
		return other != NULL && RtpTilesetRenderer::autotileCompatible(*tile, *other);
	};
	unsigned int connections = 0;
	if (matches(x, y - 1)) connections |= RtpTilesetRenderer::North;
	if (matches(x + 1, y)) connections |= RtpTilesetRenderer::East;
	if (matches(x, y + 1)) connections |= RtpTilesetRenderer::South;
	if (matches(x - 1, y)) connections |= RtpTilesetRenderer::West;
	if (matches(x - 1, y - 1)) connections |= RtpTilesetRenderer::NorthWest;
	if (matches(x + 1, y - 1)) connections |= RtpTilesetRenderer::NorthEast;
	if (matches(x + 1, y + 1)) connections |= RtpTilesetRenderer::SouthEast;
	if (matches(x - 1, y + 1)) connections |= RtpTilesetRenderer::SouthWest;
	return connections;
}

bool Application::worldTileRenderReference(const WorldMap& area, int x, int y,
	RtpRenderLayer layer, RtpTileReference& reference) const
{
	const RtpTileReference* tile = worldTileLayer(area, x, y, layer);
	if (tile == NULL) return false;
	reference = *tile;
	if (layer != RtpRenderLayer::Ground) return true;

	const int offsets[][2] = {
		{ 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
		{ -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 }
	};
	std::map<int, int> scores;
	std::map<int, RtpTileReference> candidates;
	for (int neighbor = 0; neighbor < 8; ++neighbor)
	{
		const RtpTileReference* other = worldTileLayer(area,
			x + offsets[neighbor][0], y + offsets[neighbor][1], layer);
		if (other == NULL) continue;
		RtpTileReference candidate = *tile;
		if (!RtpTilesetRenderer::automaticGroundTransition(*tile, *other,
			candidate)) continue;
		scores[candidate.index] += neighbor < 4 ? 2 : 1;
		candidates.erase(candidate.index);
		candidates.insert(std::make_pair(candidate.index, candidate));
	}

	int bestScore = 0;
	for (std::map<int, int>::const_iterator score = scores.begin();
		score != scores.end(); ++score)
	{
		int adjustedScore = score->second + (score->first == tile->index ? 1 : 0);
		if (adjustedScore <= bestScore) continue;
		bestScore = adjustedScore;
		reference = candidates.find(score->first)->second;
	}
	return true;
}

bool Application::drawWorldTileLayer(const WorldMap& area, int x, int y,
	RtpRenderLayer layer, const SDL_Rect& destination)
{
	bool rendered = false;
	if (layer == RtpRenderLayer::Foreground)
	{
		const RtpTileReference* composite = worldTileLayer(area, x, y,
			RtpRenderLayer::Decoration);
		if (composite != NULL && RtpTilesetRenderer::isCompositeTile(*composite))
			rendered = mWorldTileRenderer->drawCatalogCompositeLayer(*composite, layer,
				destination);
	}
	const RtpTileReference* tile = worldTileLayer(area, x, y, layer);
	if (tile == NULL) return rendered;
	RtpTileReference renderTile = *tile;
	worldTileRenderReference(area, x, y, layer, renderTile);
	if (RtpTilesetRenderer::isCompositeTile(*tile))
		return mWorldTileRenderer->drawCatalogCompositeLayer(*tile, layer, destination) ||
			rendered;
	return mWorldTileRenderer->drawCatalog(renderTile,
		worldTileConnections(area, x, y, layer), destination,
		SDL_GetTicks() / 420) || rendered;
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
		recordWorldBuilderEntityUndo(BUILDER_UNDO_NPC, mWorldBuilderSelectedNpc,
			npc.mapId, npc.x, npc.y);
		npc.mapId = currentMapId();
		npc.setPosition(x, y);
		mWorldBuilderDirty = true;
	}
	else if (mWorldBuilderTab == WorldBuilderTab::Objects &&
		!mWorldBuilderObjectPalette && mWorldBuilderSelectedObject >= 0 &&
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
			recordWorldBuilderEntityUndo(BUILDER_UNDO_OBJECT,
				mWorldBuilderSelectedObject, object.mapId, object.x, object.y);
			object.mapId = currentMapId();
			object.x = x;
			object.y = y;
		}
		else
		{
			int shardIndex = mWorldBuilderSelectedObject - (int)mWorldObjects.size();
			MercerShard& shard = mMercerStock.shards[shardIndex];
			if (shard.mapId == currentMapId() && shard.x == x && shard.y == y) return;
			recordWorldBuilderEntityUndo(BUILDER_UNDO_SHARD, shardIndex,
				shard.mapId, shard.x, shard.y);
			shard.mapId = currentMapId();
			shard.x = x;
			shard.y = y;
		}
		mWorldBuilderDirty = true;
	}
	else if (mWorldBuilderTab == WorldBuilderTab::Portals)
		placeWorldBuilderPortalEndpoint(x, y);
}

bool Application::saveWorldBuilder(std::string& error)
{
	if (mWorldBuilderMapDialogOpen)
	{
		error = "finish or cancel the new map before saving";
		return false;
	}
	if (mWorldBuilderRegionPlacementMode != 0)
	{
		error = "finish or cancel the region bounds before saving";
		return false;
	}
	if (mWorldBuilderPortalCreating)
	{
		error = "finish or cancel the new portal before saving";
		return false;
	}
	commitWorldBuilderRegionNameEdit(true);
	if (mWorldBuilderRegionNameFocused)
	{
		error = "give the selected region a non-empty name before saving";
		return false;
	}
	commitWorldBuilderUndoAction();
	mWorld.npcPositions.clear();
	for (size_t index = 0; index < mNpcs.size(); ++index)
		mWorld.npcPositions[mNpcs[index].id] = {
			mNpcs[index].mapId, mNpcs[index].x, mNpcs[index].y
		};
	mWorld.objectPositions.clear();
	mWorld.objectDefinitions.clear();
	mWorld.objectAppearances.clear();
	for (size_t index = 0; index < mWorldObjects.size(); ++index)
	{
		mWorld.objectPositions[mWorldObjects[index].id] = {
			mWorldObjects[index].mapId, mWorldObjects[index].x, mWorldObjects[index].y
		};
		if (mWorldObjects[index].editorCreated)
			mWorld.objectDefinitions[mWorldObjects[index].id] = {
				mWorldObjects[index].templateId
			};
		if ((mWorldObjects[index].kind == WorldObjectKind::Chest ||
			mWorldObjects[index].kind == WorldObjectKind::DeckChest) &&
			!mWorldObjects[index].appearance.empty())
			mWorld.objectAppearances[mWorldObjects[index].id] =
				mWorldObjects[index].appearance;
	}
	mWorld.shardPositions.clear();
	for (size_t index = 0; index < mMercerStock.shards.size(); ++index)
		mWorld.shardPositions[mMercerStock.shards[index].id] = {
			mMercerStock.shards[index].mapId,
			mMercerStock.shards[index].x, mMercerStock.shards[index].y
		};
	if (!WorldStorage::save("World/World.json", mWorld, error)) return false;
	mWorldBuilderDirty = false;
	clearWorldBuilderUndoHistory();
	return true;
}

void Application::panWorldBuilder(int dx, int dy)
{
	const float step = TILE / (float)mWorldBuilderTileSize;
	mWorldBuilderCameraX += dx * step;
	mWorldBuilderCameraY += dy * step;
	clampMapCamera(currentMap(), mWorldBuilderCameraX, mWorldBuilderCameraY,
		mWorldBuilderTileSize);
}

void Application::zoomWorldBuilder(int direction, int anchorX, int anchorY)
{
	int currentLevel = builderZoomLevel(mWorldBuilderTileSize);
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
	float oldOriginX = builderMapOriginX((int)map[0].size(), mWorldBuilderTileSize) -
		mWorldBuilderCameraX * mWorldBuilderTileSize;
	float oldOriginY = builderMapOriginY((int)map.size(), mWorldBuilderTileSize) -
		mWorldBuilderCameraY * mWorldBuilderTileSize;
	float worldX = (anchorX - oldOriginX) / (float)mWorldBuilderTileSize;
	float worldY = (anchorY - oldOriginY) / (float)mWorldBuilderTileSize;
	mWorldBuilderTileSize = BUILDER_ZOOM_LEVELS[nextLevel];
	int newBaseX = builderMapOriginX((int)map[0].size(), mWorldBuilderTileSize);
	int newBaseY = builderMapOriginY((int)map.size(), mWorldBuilderTileSize);
	mWorldBuilderCameraX = worldX -
		(anchorX - newBaseX) / (float)mWorldBuilderTileSize;
	mWorldBuilderCameraY = worldY -
		(anchorY - newBaseY) / (float)mWorldBuilderTileSize;
	clampMapCamera(map, mWorldBuilderCameraX, mWorldBuilderCameraY,
		mWorldBuilderTileSize);
	commitWorldBuilderUndoAction();
	mWorldBuilderPainting = false;
	mWorldBuilderErasing = false;
	mWorldBuilderLastBrushX = -1;
	mWorldBuilderLastBrushY = -1;
	mWorldBuilderDragging = false;
}

void Application::updateWorldBuilder(Uint32 deltaTime)
{
	if (mWorldBuilderMapDialogOpen)
	{
		mWorldBuilderPanAccumulator = 0;
		return;
	}
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
	if (handleWorldBuilderMapDialogEvent(event)) return;
	if (event.type == SDL_TEXTINPUT && mWorldBuilderRegionNameFocused)
	{
		if (mWorldBuilderRegionNameInput.size() < 64)
			mWorldBuilderRegionNameInput += event.text.text;
		return;
	}
	if (event.type == SDL_WINDOWEVENT &&
		event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
	{
		commitWorldBuilderRegionNameEdit(true);
		commitWorldBuilderUndoAction();
		mWorldBuilderMoveUp = mWorldBuilderMoveDown = false;
		mWorldBuilderMoveLeft = mWorldBuilderMoveRight = false;
		mWorldBuilderPanAccumulator = 0;
		mWorldBuilderPainting = false;
		mWorldBuilderErasing = false;
		mWorldBuilderLastBrushX = -1;
		mWorldBuilderLastBrushY = -1;
		mWorldBuilderDragging = false;
		mWorldBuilderRegionDragging = false;
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
		if (mWorldBuilderRegionNameFocused)
		{
			if (key == SDLK_BACKSPACE)
			{
				if (!mWorldBuilderRegionNameInput.empty())
					mWorldBuilderRegionNameInput.pop_back();
				return;
			}
			if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
			{
				commitWorldBuilderRegionNameEdit(true);
				return;
			}
			if (key == SDLK_ESCAPE)
			{
				commitWorldBuilderRegionNameEdit(false);
				return;
			}
			return;
		}
		if (key == SDLK_ESCAPE)
		{
			if (mWorldBuilderRegionPlacementMode != 0)
			{
				cancelWorldBuilderRegionPlacement();
				return;
			}
			if (mWorldBuilderPortalCreating)
			{
				cancelWorldBuilderPortalCreation();
				return;
			}
			mRunning = false;
			return;
		}
		if (key == SDLK_z && (event.key.keysym.mod & KMOD_CTRL))
		{
			undoWorldBuilder();
			return;
		}
		if (key == SDLK_s && (event.key.keysym.mod & KMOD_CTRL))
		{
			std::string error;
			if (saveWorldBuilder(error)) showWorldBuilderNotice("World saved.");
			else showWorldBuilderNotice("Save failed: " + error, true);
			return;
		}
		if (key == SDLK_n && (event.key.keysym.mod & KMOD_CTRL))
		{
			beginWorldBuilderMapCreation();
			return;
		}
		if (key == SDLK_g)
		{
			mWorldBuilderShowGrid = !mWorldBuilderShowGrid;
			return;
		}
		if ((key == SDLK_DELETE || key == SDLK_BACKSPACE) &&
			mWorldBuilderTab == WorldBuilderTab::Objects &&
			!mWorldBuilderObjectPalette)
		{
			deleteWorldBuilderObject();
			return;
		}
		if ((key == SDLK_DELETE || key == SDLK_BACKSPACE) &&
			mWorldBuilderTab == WorldBuilderTab::Portals)
		{
			deleteWorldBuilderPortal();
			return;
		}
		if ((key == SDLK_DELETE || key == SDLK_BACKSPACE) &&
			mWorldBuilderTab == WorldBuilderTab::Regions)
		{
			deleteWorldBuilderRegion();
			return;
		}
		if (key == SDLK_LEFTBRACKET || key == SDLK_RIGHTBRACKET)
		{
			if (mWorldBuilderTab == WorldBuilderTab::Tiles &&
				worldBuilderBrushResizable())
			{
				int direction = key == SDLK_LEFTBRACKET ? -1 : 1;
				mWorldBuilderBrushSize = std::max(1, std::min(
					BUILDER_MAX_BRUSH_SIZE, mWorldBuilderBrushSize + direction));
			}
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
			const RtpSheetDescriptor* sheet = RtpTilesetRenderer::descriptor(
				tilesetFamily(mWorldBuilderTileCategory),
				(RtpTileSheet)mWorldBuilderTileSheet);
			int index = key - SDLK_1;
			if (sheet != NULL && index < sheet->tileCount)
				mWorldBuilderCatalogTile = index;
			return;
		}
		if (key == SDLK_t) mWorldBuilderTab = WorldBuilderTab::Tiles;
		else if (key == SDLK_n) mWorldBuilderTab = WorldBuilderTab::Npcs;
		else if (key == SDLK_o) mWorldBuilderTab = WorldBuilderTab::Objects;
		else if (key == SDLK_r) mWorldBuilderTab = WorldBuilderTab::Regions;
		else if (key == SDLK_p) mWorldBuilderTab = WorldBuilderTab::Portals;
		else if (key == SDLK_PAGEUP)
		{
			mCurrentWorldArea = (mCurrentWorldArea + (int)mWorld.maps.size() - 1) % (int)mWorld.maps.size();
			mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
		}
		else if (key == SDLK_PAGEDOWN)
		{
			mCurrentWorldArea = (mCurrentWorldArea + 1) % (int)mWorld.maps.size();
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
			mWorldBuilderListScroll = 0;
			return;
		}
		int count = mWorldBuilderTab == WorldBuilderTab::Npcs ? (int)mNpcs.size() :
			(mWorldBuilderTab == WorldBuilderTab::Regions ? (int)mWorld.regions.size() :
			(mWorldBuilderTab == WorldBuilderTab::Portals ? (int)mWorld.portals.size() :
			(mWorldBuilderObjectPalette ? (int)mWorldObjectTemplates.size() :
			(int)(mWorldObjects.size() + mMercerStock.shards.size()))));
		int rows = mWorldBuilderTab == WorldBuilderTab::Regions ?
			BUILDER_REGION_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Portals ?
			BUILDER_PORTAL_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Objects &&
			!mWorldBuilderObjectPalette ? BUILDER_OBJECT_LIST_ROWS : BUILDER_LIST_ROWS));
		int maximum = std::max(0, count - rows);
		mWorldBuilderListScroll = std::max(0,
			std::min(maximum, mWorldBuilderListScroll - event.wheel.y));
		return;
	}
	if (event.type == SDL_MOUSEBUTTONUP &&
		(event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT))
	{
		if (event.button.button == SDL_BUTTON_LEFT && mWorldBuilderRegionDragging)
		{
			finishWorldBuilderRegionPlacement();
			return;
		}
		commitWorldBuilderUndoAction();
		mWorldBuilderPainting = false;
		mWorldBuilderErasing = false;
		mWorldBuilderLastBrushX = -1;
		mWorldBuilderLastBrushY = -1;
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
			if (mWorldBuilderRegionDragging)
				updateWorldBuilderRegionDraft(cellX, cellY);
			else if (mWorldBuilderPainting || mWorldBuilderErasing)
			{
				applyWorldBuilderBrushStroke(mWorldBuilderLastBrushX,
					mWorldBuilderLastBrushY, cellX, cellY, mWorldBuilderErasing);
				mWorldBuilderLastBrushX = cellX;
				mWorldBuilderLastBrushY = cellY;
			}
			else if (mWorldBuilderDragging) placeWorldBuilderSelection(cellX, cellY);
		}
		else if (mWorldBuilderPainting || mWorldBuilderErasing)
		{
			mWorldBuilderLastBrushX = -1;
			mWorldBuilderLastBrushY = -1;
		}
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN ||
		(event.button.button != SDL_BUTTON_LEFT &&
		event.button.button != SDL_BUTTON_RIGHT)) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	mMouseX = x;
	mMouseY = y;
	if (mWorldBuilderRegionNameFocused && !contains(BUILDER_REGION_NAME, x, y))
	{
		commitWorldBuilderRegionNameEdit(true);
		if (mWorldBuilderRegionNameFocused) return;
	}
	if (contains(BUILDER_GRID, x, y))
	{
		mWorldBuilderShowGrid = !mWorldBuilderShowGrid;
		return;
	}
	if (contains(BUILDER_UNDO, x, y))
	{
		undoWorldBuilder();
		return;
	}
	if (contains(BUILDER_SAVE, x, y))
	{
		std::string error;
		if (saveWorldBuilder(error)) showWorldBuilderNotice("World saved.");
		else showWorldBuilderNotice("Save failed: " + error, true);
		return;
	}
	if (contains(BUILDER_NEW_MAP, x, y))
	{
		beginWorldBuilderMapCreation();
		return;
	}
	if (contains(BUILDER_PREVIOUS_MAP, x, y))
	{
		mCurrentWorldArea = (mCurrentWorldArea + (int)mWorld.maps.size() - 1) % (int)mWorld.maps.size();
		mWorldBuilderCameraX = mWorldBuilderCameraY = 0;
		return;
	}
	if (contains(BUILDER_NEXT_MAP, x, y))
	{
		mCurrentWorldArea = (mCurrentWorldArea + 1) % (int)mWorld.maps.size();
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
	else if (contains(BUILDER_REGIONS_TAB, x, y))
	{
		mWorldBuilderTab = WorldBuilderTab::Regions;
		mWorldBuilderListScroll = 0;
	}
	else if (contains(BUILDER_PORTALS_TAB, x, y))
	{
		mWorldBuilderTab = WorldBuilderTab::Portals;
		mWorldBuilderListScroll = 0;
	}
	else
	{
		if (mWorldBuilderTab == WorldBuilderTab::Regions &&
			contains(BUILDER_REGION_NEW, x, y))
		{
			if (mWorldBuilderRegionPlacementMode == 1)
				cancelWorldBuilderRegionPlacement();
			else beginWorldBuilderRegionCreation();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Regions &&
			contains(BUILDER_REGION_BOUNDS, x, y))
		{
			if (mWorldBuilderRegionPlacementMode == 2)
				cancelWorldBuilderRegionPlacement();
			else beginWorldBuilderRegionBounds();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Regions &&
			contains(BUILDER_REGION_DELETE, x, y))
		{
			deleteWorldBuilderRegion();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Regions &&
			contains(BUILDER_REGION_KIND, x, y))
		{
			toggleWorldBuilderRegionKind();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Regions &&
			contains(BUILDER_REGION_WEATHER, x, y))
		{
			toggleWorldBuilderRegionWeather();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Regions &&
			contains(BUILDER_REGION_NAME, x, y))
		{
			if (!mWorldBuilderRegionNameFocused)
				beginWorldBuilderRegionNameEdit();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Objects &&
			!mWorldBuilderObjectPalette &&
			(contains(BUILDER_PORTAL_APPEARANCE_PREVIOUS, x, y) ||
			contains(BUILDER_PORTAL_APPEARANCE_NEXT, x, y)))
		{
			cycleWorldBuilderChestAppearance(
				contains(BUILDER_PORTAL_APPEARANCE_PREVIOUS, x, y) ? -1 : 1);
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Objects &&
			(contains(BUILDER_OBJECT_ADD, x, y) ||
			contains(BUILDER_OBJECT_PLACED, x, y)))
		{
			mWorldBuilderObjectPalette = contains(BUILDER_OBJECT_ADD, x, y);
			if (mWorldBuilderObjectPalette && mWorldBuilderSelectedObjectTemplate < 0 &&
				!mWorldObjectTemplates.empty()) mWorldBuilderSelectedObjectTemplate = 0;
			mWorldBuilderListScroll = 0;
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Portals &&
			(contains(BUILDER_PORTAL_APPEARANCE_PREVIOUS, x, y) ||
			contains(BUILDER_PORTAL_APPEARANCE_NEXT, x, y)))
		{
			cycleWorldBuilderPortalAppearance(
				contains(BUILDER_PORTAL_APPEARANCE_PREVIOUS, x, y) ? -1 : 1);
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Portals &&
			contains(BUILDER_PORTAL_NEW, x, y))
		{
			if (mWorldBuilderPortalCreating) cancelWorldBuilderPortalCreation();
			else beginWorldBuilderPortalCreation();
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Portals &&
			(contains(BUILDER_PORTAL_FROM, x, y) ||
			contains(BUILDER_PORTAL_TO, x, y)))
		{
			if (!mWorldBuilderPortalCreating && mWorldBuilderSelectedPortal >= 0 &&
				mWorldBuilderSelectedPortal < (int)mWorld.portals.size())
			{
				mWorldBuilderPortalEndpoint = contains(BUILDER_PORTAL_FROM, x, y) ? 0 : 1;
				const WorldPortal& portal = mWorld.portals[mWorldBuilderSelectedPortal];
				const std::string& mapId = mWorldBuilderPortalEndpoint == 0 ?
					portal.fromMap : portal.toMap;
				int portalX = mWorldBuilderPortalEndpoint == 0 ? portal.fromX : portal.toX;
				int portalY = mWorldBuilderPortalEndpoint == 0 ? portal.fromY : portal.toY;
				int map = worldAreaIndex(mapId);
				if (map >= 0)
				{
					mCurrentWorldArea = map;
					centerMapCamera(currentMap(), portalX, portalY, mWorldBuilderCameraX,
						mWorldBuilderCameraY, mWorldBuilderTileSize);
				}
			}
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Tiles)
		{
			if (contains(BUILDER_BRUSH_DECREASE, x, y) ||
				contains(BUILDER_BRUSH_INCREASE, x, y))
			{
				if (worldBuilderBrushResizable())
				{
					int direction = contains(BUILDER_BRUSH_DECREASE, x, y) ? -1 : 1;
					mWorldBuilderBrushSize = std::max(1, std::min(
						BUILDER_MAX_BRUSH_SIZE, mWorldBuilderBrushSize + direction));
				}
				return;
			}
			for (int category = 0; category < TILE_CATEGORY_COUNT; ++category)
			{
				if (!contains(tileCategoryRect(category), x, y)) continue;
				mWorldBuilderTileCategory = category;
				std::vector<RtpTileSheet> sheets = familySheets(tilesetFamily(category));
				if (!sheets.empty()) mWorldBuilderTileSheet = (int)sheets[0];
				mWorldBuilderCatalogTile = 0;
				mWorldBuilderListScroll = 0;
				return;
			}
			std::vector<RtpTileSheet> sheets = familySheets(
				tilesetFamily(mWorldBuilderTileCategory));
			int sheetIndex = 0;
			for (size_t i = 0; i < sheets.size(); ++i)
				if ((int)sheets[i] == mWorldBuilderTileSheet) sheetIndex = (int)i;
			if (!sheets.empty() && (contains(BUILDER_PREVIOUS_SHEET, x, y) ||
				contains(BUILDER_NEXT_SHEET, x, y)))
			{
				int direction = contains(BUILDER_PREVIOUS_SHEET, x, y) ? -1 : 1;
				sheetIndex = (sheetIndex + direction + (int)sheets.size()) % (int)sheets.size();
				mWorldBuilderTileSheet = (int)sheets[sheetIndex];
				mWorldBuilderCatalogTile = 0;
				mWorldBuilderListScroll = 0;
				return;
			}
			const RtpSheetDescriptor* sheet = RtpTilesetRenderer::descriptor(
				tilesetFamily(mWorldBuilderTileCategory),
				(RtpTileSheet)mWorldBuilderTileSheet);
			if (sheet != NULL)
			{
				int tile = catalogTileAt(*sheet, catalogSheetRect(*sheet), x, y);
				if (tile >= 0)
				{
					mWorldBuilderCatalogTile = RtpTilesetRenderer::canonicalTileIndex(
						tilesetFamily(mWorldBuilderTileCategory),
						(RtpTileSheet)mWorldBuilderTileSheet, tile);
					return;
				}
			}
		}
		else if (x >= 1022 && x < 1250 && y >= BUILDER_LIST_Y &&
			y < BUILDER_LIST_Y + (mWorldBuilderTab == WorldBuilderTab::Regions ?
			BUILDER_REGION_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Portals ?
			BUILDER_PORTAL_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Objects &&
			!mWorldBuilderObjectPalette ? BUILDER_OBJECT_LIST_ROWS :
			BUILDER_LIST_ROWS))) * BUILDER_LIST_ROW)
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
				mWorldBuilderObjectPalette &&
				selected < (int)mWorldObjectTemplates.size())
			{
				mWorldBuilderSelectedObjectTemplate = selected;
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
			else if (mWorldBuilderTab == WorldBuilderTab::Portals &&
				selected < (int)mWorld.portals.size())
			{
				mWorldBuilderPortalCreating = false;
				mWorldBuilderPortalDraftOrigin = WorldPosition();
				mWorldBuilderSelectedPortal = selected;
				mWorldBuilderPortalEndpoint = 0;
				if (event.button.clicks >= 2)
				{
					const WorldPortal& portal = mWorld.portals[selected];
					const std::string& mapId = mWorldBuilderPortalEndpoint == 0 ?
						portal.fromMap : portal.toMap;
					int portalX = mWorldBuilderPortalEndpoint == 0 ? portal.fromX : portal.toX;
					int portalY = mWorldBuilderPortalEndpoint == 0 ? portal.fromY : portal.toY;
					int map = worldAreaIndex(mapId);
					if (map >= 0)
					{
						mCurrentWorldArea = map;
						centerMapCamera(currentMap(), portalX, portalY,
							mWorldBuilderCameraX, mWorldBuilderCameraY,
							mWorldBuilderTileSize);
					}
				}
			}
			else if (mWorldBuilderTab == WorldBuilderTab::Regions &&
				selected < (int)mWorld.regions.size())
			{
				mWorldBuilderSelectedRegion = selected;
				mWorldBuilderRegionNameInput = mWorld.regions[selected].name;
				if (event.button.clicks >= 2)
				{
					const WorldRegion& region = mWorld.regions[selected];
					int map = worldAreaIndex(region.mapId);
					if (map >= 0)
					{
						mCurrentWorldArea = map;
						centerMapCamera(currentMap(), region.x + region.width / 2,
							region.y + region.height / 2, mWorldBuilderCameraX,
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
			beginWorldBuilderUndoAction();
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				mWorldBuilderErasing = true;
				applyWorldBuilderBrush(cellX, cellY, true);
			}
			else
			{
				mWorldBuilderPainting = true;
				applyWorldBuilderBrush(cellX, cellY, false);
			}
			mWorldBuilderLastBrushX = cellX;
			mWorldBuilderLastBrushY = cellY;
			return;
		}
		if (mWorldBuilderTab == WorldBuilderTab::Npcs)
		{
			beginWorldBuilderUndoAction();
			int hit = -1;
			for (size_t i = 0; i < mNpcs.size(); ++i)
				if (mNpcs[i].mapId == currentMapId() && mNpcs[i].x == cellX &&
					mNpcs[i].y == cellY) hit = (int)i;
			if (hit >= 0) mWorldBuilderSelectedNpc = hit;
			else placeWorldBuilderSelection(cellX, cellY);
		}
		else if (mWorldBuilderTab == WorldBuilderTab::Objects &&
			mWorldBuilderObjectPalette)
		{
			if (event.button.button == SDL_BUTTON_LEFT &&
				mWorldBuilderSelectedObjectTemplate >= 0)
				addWorldBuilderObject(mWorldBuilderSelectedObjectTemplate, cellX, cellY);
			return;
		}
		else if (mWorldBuilderTab == WorldBuilderTab::Objects)
		{
			beginWorldBuilderUndoAction();
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
		else if (mWorldBuilderTab == WorldBuilderTab::Portals)
		{
			if (mWorldBuilderPortalCreating)
			{
				placeWorldBuilderPortalEndpoint(cellX, cellY);
				return;
			}
			beginWorldBuilderUndoAction();
			int hit = -1;
			bool hitFrom = true;
			for (size_t index = 0; index < mWorld.portals.size(); ++index)
			{
				const WorldPortal& portal = mWorld.portals[index];
				if (portal.fromMap == currentMapId() && portal.fromX == cellX &&
					portal.fromY == cellY)
				{
					hit = (int)index;
					hitFrom = true;
					break;
				}
				if (portal.toMap == currentMapId() && portal.toX == cellX &&
					portal.toY == cellY)
				{
					hit = (int)index;
					hitFrom = false;
					break;
				}
			}
			if (hit >= 0)
			{
				mWorldBuilderSelectedPortal = hit;
				mWorldBuilderPortalEndpoint = hitFrom ? 0 : 1;
			}
			else placeWorldBuilderSelection(cellX, cellY);
		}
		else if (mWorldBuilderTab == WorldBuilderTab::Regions)
		{
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				if (mWorldBuilderRegionPlacementMode != 0)
					cancelWorldBuilderRegionPlacement();
				return;
			}
			if (mWorldBuilderRegionPlacementMode != 0)
			{
				mWorldBuilderRegionDragging = true;
				mWorldBuilderRegionDragStartX = cellX;
				mWorldBuilderRegionDragStartY = cellY;
				updateWorldBuilderRegionDraft(cellX, cellY);
				return;
			}
			int hit = -1;
			for (size_t index = 0; index < mWorld.regions.size(); ++index)
				if (mWorld.regions[index].contains(currentMapId(), cellX, cellY))
				{
					hit = (int)index;
					break;
				}
			mWorldBuilderSelectedRegion = hit;
			mWorldBuilderRegionNameInput = hit >= 0 ? mWorld.regions[hit].name : "";
			return;
		}
		mWorldBuilderDragging = true;
		return;
	}
	mWorldBuilderListScroll = 0;
}

void Application::drawWorldBuilderNpcPortrait(const Npc& npc, const SDL_Rect& rect)
{
	bool savedScaleActive = mWorldBuilderTileScaleActive;
	SDL_Rect savedScaleDestination = mWorldBuilderTileScaleDestination;
	mWorldBuilderTileScaleActive = true;
	mWorldBuilderTileScaleDestination = rect;
	drawCharacterSprite(rect.x, rect.y, npc.appearance, false, false,
		npc.facingX, npc.facingY, npc.spriteSheet, npc.spriteIndex);
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

int Application::worldBuilderHoveredObject() const
{
	int cellX = -1;
	int cellY = -1;
	if (!mapCellAt(mMouseX, mMouseY, currentMap(), mWorldBuilderCameraX,
		mWorldBuilderCameraY, mWorldBuilderTileSize, cellX, cellY)) return -1;
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
		if (mWorldObjects[i].mapId == currentMapId() && mWorldObjects[i].x == cellX &&
			mWorldObjects[i].y == cellY) return (int)i;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		if (mMercerStock.shards[i].mapId == currentMapId() &&
			mMercerStock.shards[i].x == cellX && mMercerStock.shards[i].y == cellY)
			return (int)mWorldObjects.size() + (int)i;
	return -1;
}

int Application::worldBuilderHoveredPortal(bool& fromEndpoint) const
{
	fromEndpoint = true;
	int cellX = -1;
	int cellY = -1;
	if (!mapCellAt(mMouseX, mMouseY, currentMap(), mWorldBuilderCameraX,
		mWorldBuilderCameraY, mWorldBuilderTileSize, cellX, cellY)) return -1;
	for (size_t index = 0; index < mWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = mWorld.portals[index];
		if (portal.fromMap == currentMapId() && portal.fromX == cellX &&
			portal.fromY == cellY) return (int)index;
		if (portal.toMap == currentMapId() && portal.toX == cellX &&
			portal.toY == cellY)
		{
			fromEndpoint = false;
			return (int)index;
		}
	}
	return -1;
}

void Application::renderWorldBuilder()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 12, 18, 29);
	drawText("WORLD BUILDER", 32, 13, color(244, 207, 103), 25);
	drawText(mWorld.maps[mCurrentWorldArea].name, 320, 17, color(189, 207, 232), 18, 360);
	drawText("ZOOM " + std::to_string(
		BUILDER_ZOOM_PERCENTAGES[builderZoomLevel(mWorldBuilderTileSize)]) + "%",
		690, 19, color(143, 189, 231), 14);
	drawText(mWorldBuilderDirty ? "UNSAVED CHANGES" : "SAVED", 810, 19,
		mWorldBuilderDirty ? color(244, 139, 88) : color(105, 218, 139), 14);
	const std::vector<std::string>& map = currentMap();
	const int tileSize = mWorldBuilderTileSize;
	const int viewportColumns = builderViewportColumns(tileSize);
	const int viewportRows = builderViewportRows(tileSize);
	int mapX = builderMapOriginX((int)map[0].size(), tileSize) -
		(int)std::round(mWorldBuilderCameraX * tileSize);
	int mapY = builderMapOriginY((int)map.size(), tileSize) -
		(int)std::round(mWorldBuilderCameraY * tileSize);
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
			if (mWorld.maps[mCurrentWorldArea].catalogOnly) continue;
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
			bool texturedTerrain = mWorldTileRenderer->drawTerrain(
				type, map, column, row, tile);
			if (mWorldTileRenderer->canDrawDecoration(type)) continue;
			if (texturedTerrain)
				continue;
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
			{
				if (!texturedTerrain)
					fillRect({ tile.x + 7, tile.y + 17, 28, 3 }, 92, 189, 210, 190);
			}
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
		}
	}
	const WorldMap& builderArea = mWorld.maps[mCurrentWorldArea];
	for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
		for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
		{
			SDL_Rect tileRect = { mapX + x * tileSize, mapY + y * tileSize,
				tileSize, tileSize };
			drawWorldTileLayer(builderArea, x, y, RtpRenderLayer::Ground, tileRect);
		}
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].mapId == currentMapId() &&
			visibleTiles.contains(mNpcs[i].x, mNpcs[i].y))
			drawCharacterShadow((float)mNpcs[i].x, (float)mNpcs[i].y);
	for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
	{
		for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
		{
			if (builderArea.catalogOnly) continue;
			if (worldTileLayer(builderArea, x, y, RtpRenderLayer::Ground) != NULL) continue;
			WorldTileId tile = WorldTiles::fromGlyph(map[y][x]);
			if (!WorldTileRenderer::hasDecoration(tile)) continue;
			SDL_Rect tileRect = { mapX + x * tileSize, mapY + y * tileSize,
				tileSize, tileSize };
			mWorldTileRenderer->drawDecoration(tile, tileRect);
		}
	}
	for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
		for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
		{
			SDL_Rect tileRect = { mapX + x * tileSize, mapY + y * tileSize,
				tileSize, tileSize };
			drawWorldTileLayer(builderArea, x, y,
				RtpRenderLayer::Decoration, tileRect);
		}
	if (mWorldBuilderShowGrid)
		for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
			for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
				outlineRect({ mapX + x * tileSize, mapY + y * tileSize,
					tileSize, tileSize }, 10, 20, 27, 100, 1);

	if (currentMapId() == mWorld.start.mapId &&
		visibleTiles.contains(mWorld.start.x, mWorld.start.y))
	{
		int startX = mapX + mWorld.start.x * tileSize;
		int startY = mapY + mWorld.start.y * tileSize;
		int inset = std::max(2, tileSize / 4);
		fillRect({ startX + inset, startY + inset, tileSize - inset * 2,
			tileSize - inset * 2 }, 24, 66, 137, 235);
		if (tileSize >= 32)
			drawText("P", startX + tileSize / 2 - 5, startY + tileSize / 2 - 9,
				color(215, 232, 255), std::min(16, tileSize / 3));
	}
	for (size_t index = 0; index < mWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = mWorld.portals[index];
		if (portal.fromMap != currentMapId() ||
			!visibleTiles.contains(portal.fromX, portal.fromY)) continue;
		drawPortalSprite(portal, 0.f,
			{ mapX + portal.fromX * tileSize, mapY + portal.fromY * tileSize,
				tileSize, tileSize });
	}
	auto drawPortalEndpoint = [this, mapX, mapY, tileSize](int portalIndex,
		const std::string& mapId, int x, int y, bool fromEndpoint)
	{
		if (mapId != currentMapId()) return;
		int inset = std::max(2, tileSize / 12);
		int red = fromEndpoint ? 64 : 174;
		int green = fromEndpoint ? 214 : 119;
		int blue = fromEndpoint ? 229 : 231;
		bool selected = mWorldBuilderTab == WorldBuilderTab::Portals &&
			portalIndex == mWorldBuilderSelectedPortal &&
			(mWorldBuilderPortalEndpoint == 0) == fromEndpoint;
		SDL_Rect marker = { mapX + x * tileSize + inset, mapY + y * tileSize + inset,
			tileSize - inset * 2, tileSize - inset * 2 };
		fillRect(marker, red / 3, green / 3, blue / 3, 125);
		outlineRect(marker, selected ? 247 : red, selected ? 207 : green,
			selected ? 83 : blue, 255, selected ? std::max(3, tileSize / 16) :
			std::max(2, tileSize / 24));
		if (tileSize >= 24)
			drawText(fromEndpoint ? "F" : "T", marker.x + marker.w / 2 - 4,
				marker.y + marker.h / 2 - 7,
				color(selected ? 255 : 219, selected ? 231 : 235,
					selected ? 141 : 250), std::min(13, tileSize / 3));
	};
	for (size_t index = 0; index < mWorld.portals.size(); ++index)
	{
		const WorldPortal& portal = mWorld.portals[index];
		if (visibleTiles.contains(portal.fromX, portal.fromY))
			drawPortalEndpoint((int)index, portal.fromMap, portal.fromX, portal.fromY, true);
		if (visibleTiles.contains(portal.toX, portal.toY))
			drawPortalEndpoint((int)index, portal.toMap, portal.toX, portal.toY, false);
	}
	if (mWorldBuilderPortalCreating && mWorldBuilderPortalEndpoint == 1 &&
		mWorldBuilderPortalDraftOrigin.mapId == currentMapId() &&
		visibleTiles.contains(mWorldBuilderPortalDraftOrigin.x,
			mWorldBuilderPortalDraftOrigin.y))
		drawPortalEndpoint(-1, mWorldBuilderPortalDraftOrigin.mapId,
			mWorldBuilderPortalDraftOrigin.x, mWorldBuilderPortalDraftOrigin.y, true);
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
	{
		const WorldObject& object = mWorldObjects[i];
		if (object.mapId != currentMapId() || !visibleTiles.contains(object.x, object.y))
			continue;
		int x = mapX + object.x * tileSize;
		int y = mapY + object.y * tileSize;
		if (object.kind == WorldObjectKind::Signpost)
		{
			if (!mWorldTileRenderer->drawSignpost({ x, y, tileSize, tileSize }))
			{
			int postWidth = std::max(2, tileSize / 9);
			fillRect({ x + tileSize / 2 - postWidth / 2, y + tileSize / 3,
				postWidth, tileSize * 3 / 5 }, 91, 58, 32, 255);
			fillRect({ x + tileSize / 7, y + tileSize / 6,
				tileSize * 5 / 7, tileSize / 3 }, 178, 125, 59, 255);
			outlineRect({ x + tileSize / 7, y + tileSize / 6,
				tileSize * 5 / 7, tileSize / 3 }, 70, 43, 27, 255, 1);
			}
		}
		else
		{
			if (!drawWorldObjectSprite(object, false, { x, y, tileSize, tileSize }))
			{
				int inset = std::max(2, tileSize / 7);
				if (object.kind == WorldObjectKind::CuttableBush)
					fillRect({ x + inset, y + inset, tileSize - inset * 2,
						tileSize - inset * 2 }, 45, 122, 61, 255);
				else if (object.kind == WorldObjectKind::SmashableRock)
					fillRect({ x + inset, y + tileSize / 3, tileSize - inset * 2,
						tileSize / 2 }, 103, 111, 122, 255);
				else if (object.kind == WorldObjectKind::Environment)
					fillRect({ x + tileSize / 3, y + inset, tileSize / 3,
						tileSize - inset * 2 }, 116, 85, 164, 255);
				else
				{
					fillRect({ x + inset, y + tileSize * 2 / 5,
						tileSize - inset * 2, tileSize / 2 }, 121, 69, 32, 255);
					fillRect({ x + inset, y + tileSize / 4,
						tileSize - inset * 2, tileSize / 4 }, 166, 97, 39, 255);
					outlineRect({ x + inset, y + tileSize / 4,
						tileSize - inset * 2, tileSize * 13 / 20 }, 58, 35, 23, 255, 1);
					fillRect({ x + tileSize * 9 / 20, y + tileSize / 2,
						std::max(2, tileSize / 8), tileSize / 5 }, 224, 174, 65, 255);
				}
			}
		}
		if ((int)i == mWorldBuilderSelectedObject &&
			mWorldBuilderTab == WorldBuilderTab::Objects &&
			!mWorldBuilderObjectPalette)
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
		bool drewShard = mWorldTileRenderer->drawShard({ x, y, tileSize, tileSize });
		if (!drewShard)
		{
		int unit = std::max(1, tileSize / 12);
		fillRect({ x + tileSize / 2 - unit * 2, y + tileSize / 5,
			unit * 4, tileSize * 3 / 5 }, 47, 25, 71, 230);
		fillRect({ x + tileSize / 4, y + tileSize / 3,
			tileSize / 2, tileSize / 3 }, 146, 87, 211, 250);
		fillRect({ x + tileSize * 2 / 5, y + tileSize * 2 / 5,
			tileSize / 5, tileSize / 5 }, 231, 193, 255, 255);
		}
		if ((int)(mWorldObjects.size() + i) == mWorldBuilderSelectedObject &&
			mWorldBuilderTab == WorldBuilderTab::Objects &&
			!mWorldBuilderObjectPalette)
			outlineRect({ x + 2, y + 2, tileSize - 4, tileSize - 4 },
				246, 211, 99, 255, std::max(2, tileSize / 16));
	}
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (mNpcs[i].mapId != currentMapId()) continue;
		if (!visibleTiles.contains(mNpcs[i].x, mNpcs[i].y)) continue;
		int npcX = mapX + mNpcs[i].x * tileSize;
		int npcY = mapY + mNpcs[i].y * tileSize;
		drawCharacter((float)mNpcs[i].x, (float)mNpcs[i].y,
			mNpcs[i].appearance, false, false, mNpcs[i].facingX, mNpcs[i].facingY,
			mNpcs[i].spriteSheet, mNpcs[i].spriteIndex);
		if ((int)i == mWorldBuilderSelectedNpc && mWorldBuilderTab == WorldBuilderTab::Npcs)
				outlineRect({ npcX + 2, npcY + 2, tileSize - 4, tileSize - 4 },
					246, 211, 99, 255, std::max(2, tileSize / 16));
	}
	for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
	{
		for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
		{
			if (builderArea.catalogOnly) continue;
			if (worldTileLayer(builderArea, x, y, RtpRenderLayer::Ground) != NULL) continue;
			WorldTileId tile = WorldTiles::fromGlyph(map[y][x]);
			if (!WorldTileRenderer::hasForeground(tile)) continue;
			SDL_Rect tileRect = { mapX + x * tileSize, mapY + y * tileSize,
				tileSize, tileSize };
			mWorldTileRenderer->drawForeground(tile, tileRect);
		}
	}
	for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
		for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
		{
			SDL_Rect tileRect = { mapX + x * tileSize, mapY + y * tileSize,
				tileSize, tileSize };
			drawWorldTileLayer(builderArea, x, y, RtpRenderLayer::Foreground, tileRect);
		}
	if (mWorldBuilderTab == WorldBuilderTab::Regions)
	{
		auto drawRegion = [this, mapX, mapY, tileSize](const WorldRegion& region,
			bool selected, bool draft)
		{
			if (region.mapId != currentMapId() || region.width <= 0 || region.height <= 0)
				return;
			int left = mapX + region.x * tileSize;
			int top = mapY + region.y * tileSize;
			int right = left + region.width * tileSize;
			int bottom = top + region.height * tileSize;
			int clippedLeft = std::max(MAP_X, left);
			int clippedTop = std::max(MAP_Y, top);
			int clippedRight = std::min(MAP_X + MAP_VIEW_WIDTH, right);
			int clippedBottom = std::min(MAP_Y + MAP_VIEW_HEIGHT, bottom);
			if (clippedLeft >= clippedRight || clippedTop >= clippedBottom) return;
			int red = draft ? 236 : (region.connector ? 87 : 232);
			int green = draft ? 126 : (region.connector ? 190 : 184);
			int blue = draft ? 92 : (region.connector ? 229 : 82);
			fillRect({ clippedLeft, clippedTop, clippedRight - clippedLeft,
				clippedBottom - clippedTop }, red, green, blue, selected || draft ? 42 : 22);
			int thickness = selected || draft ? 3 : 2;
			if (left >= MAP_X && left < MAP_X + MAP_VIEW_WIDTH)
				fillRect({ left, clippedTop, thickness, clippedBottom - clippedTop },
					red, green, blue, 235);
			if (right > MAP_X && right <= MAP_X + MAP_VIEW_WIDTH)
				fillRect({ right - thickness, clippedTop, thickness,
					clippedBottom - clippedTop }, red, green, blue, 235);
			if (top >= MAP_Y && top < MAP_Y + MAP_VIEW_HEIGHT)
				fillRect({ clippedLeft, top, clippedRight - clippedLeft, thickness },
					red, green, blue, 235);
			if (bottom > MAP_Y && bottom <= MAP_Y + MAP_VIEW_HEIGHT)
				fillRect({ clippedLeft, bottom - thickness, clippedRight - clippedLeft,
					thickness }, red, green, blue, 235);
			if ((selected || draft) && !region.name.empty())
			{
				int labelWidth = std::max(80, std::min(240,
					18 + (int)region.name.size() * 8));
				int labelX = std::max(MAP_X + 3, std::min(
					MAP_X + MAP_VIEW_WIDTH - labelWidth - 3, clippedLeft + 4));
				int labelY = std::max(MAP_Y + 3, clippedTop + 4);
				fillRect({ labelX, labelY, labelWidth, 24 }, 12, 20, 34, 235);
				outlineRect({ labelX, labelY, labelWidth, 24 }, red, green, blue, 255, 2);
				drawText(region.name, labelX + 8, labelY + 5,
					color(241, 240, 225), 11, labelWidth - 16);
			}
		};
		for (size_t index = 0; index < mWorld.regions.size(); ++index)
			drawRegion(mWorld.regions[index],
				(int)index == mWorldBuilderSelectedRegion, false);
		if (mWorldBuilderRegionPlacementMode != 0 &&
			mWorldBuilderRegionDraft.width > 0)
		{
			WorldRegion draft = mWorldBuilderRegionDraft;
			draft.name = mWorldBuilderRegionPlacementMode == 1 ?
				"New region bounds" : "Updated bounds";
			drawRegion(draft, true, true);
		}
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
	int hoveredObject = worldBuilderHoveredObject();
	if (hoveredObject >= 0)
	{
		bool regularObject = hoveredObject < (int)mWorldObjects.size();
		const std::string& name = regularObject ? mWorldObjects[hoveredObject].name :
			mMercerStock.shards[hoveredObject - (int)mWorldObjects.size()].name;
		int objectX = regularObject ? mWorldObjects[hoveredObject].x :
			mMercerStock.shards[hoveredObject - (int)mWorldObjects.size()].x;
		int objectY = regularObject ? mWorldObjects[hoveredObject].y :
			mMercerStock.shards[hoveredObject - (int)mWorldObjects.size()].y;
		int objectScreenX = mapX + objectX * tileSize;
		int objectScreenY = mapY + objectY * tileSize;
		int labelWidth = std::max(54, std::min(230, 18 + (int)name.size() * 8));
		int labelX = std::max(MAP_X + 3, std::min(MAP_X + MAP_VIEW_WIDTH - labelWidth - 3,
			objectScreenX + tileSize / 2 - labelWidth / 2));
		int labelY = objectScreenY - 27;
		if (labelY < MAP_Y + 3) labelY = objectScreenY + tileSize + 3;
		fillRect({ labelX, labelY, labelWidth, 24 }, 12, 20, 34, 244);
		outlineRect({ labelX, labelY, labelWidth, 24 }, 238, 188, 79, 255, 2);
		drawText(name, labelX + 8, labelY + 5, color(247, 221, 151), 12,
			labelWidth - 16);
	}
	bool hoveredPortalFrom = true;
	int hoveredPortal = mWorldBuilderTab == WorldBuilderTab::Portals ?
		worldBuilderHoveredPortal(hoveredPortalFrom) : -1;
	if (hoveredPortal >= 0)
	{
		const WorldPortal& portal = mWorld.portals[hoveredPortal];
		int portalX = hoveredPortalFrom ? portal.fromX : portal.toX;
		int portalY = hoveredPortalFrom ? portal.fromY : portal.toY;
		const std::string& otherMap = hoveredPortalFrom ? portal.toMap : portal.fromMap;
		int otherX = hoveredPortalFrom ? portal.toX : portal.fromX;
		int otherY = hoveredPortalFrom ? portal.toY : portal.fromY;
		std::string label = std::string(hoveredPortalFrom ? "FROM to " : "TO from ") +
			otherMap + " " + std::to_string(otherX) + "," + std::to_string(otherY);
		int screenX = mapX + portalX * tileSize;
		int screenY = mapY + portalY * tileSize;
		int labelWidth = std::max(90, std::min(270, 18 + (int)label.size() * 8));
		int labelX = std::max(MAP_X + 3, std::min(MAP_X + MAP_VIEW_WIDTH - labelWidth - 3,
			screenX + tileSize / 2 - labelWidth / 2));
		int labelY = screenY - 27;
		if (labelY < MAP_Y + 3) labelY = screenY + tileSize + 3;
		fillRect({ labelX, labelY, labelWidth, 24 }, 12, 20, 34, 244);
		outlineRect({ labelX, labelY, labelWidth, 24 }, 108, 218, 235, 255, 2);
		drawText(label, labelX + 8, labelY + 5, color(211, 237, 247), 11,
			labelWidth - 16);
	}
	SDL_RenderSetClipRect(mRenderer, NULL);

	fillRect(BUILDER_PANEL, 20, 28, 44, 248);
	outlineRect(BUILDER_PANEL, 184, 140, 60, 255, 2);
	drawText("EDIT WORLD", 1027, 32, color(240, 205, 108), 21);
	fillRect(BUILDER_GRID, mWorldBuilderShowGrid ? 61 : 34,
		mWorldBuilderShowGrid ? 72 : 43, mWorldBuilderShowGrid ? 54 : 61, 245);
	outlineRect(BUILDER_GRID, mWorldBuilderShowGrid ? 207 : 91,
		mWorldBuilderShowGrid ? 161 : 108, mWorldBuilderShowGrid ? 73 : 132, 255, 1);
	drawText(mWorldBuilderShowGrid ? "GRID ON" : "GRID OFF",
		BUILDER_GRID.x + 11, BUILDER_GRID.y + 7,
		color(mWorldBuilderShowGrid ? 242 : 155, mWorldBuilderShowGrid ? 224 : 166,
			mWorldBuilderShowGrid ? 174 : 185), 9, 72);
	fillRect(BUILDER_PREVIOUS_MAP, 37, 47, 67, 245);
	fillRect(BUILDER_NEW_MAP, 46, 63, 58, 245);
	fillRect(BUILDER_NEXT_MAP, 37, 47, 67, 245);
	outlineRect(BUILDER_PREVIOUS_MAP, 113, 139, 176, 255, 2);
	outlineRect(BUILDER_NEW_MAP, 126, 164, 121, 255, 2);
	outlineRect(BUILDER_NEXT_MAP, 113, 139, 176, 255, 2);
	drawText("<", 1033, 68, color(229, 235, 245), 14);
	drawText(">", 1229, 68, color(229, 235, 245), 14);
	drawText("NEW", BUILDER_NEW_MAP.x + 6, BUILDER_NEW_MAP.y + 10,
		color(210, 235, 205), 8, 25);
	drawText(mWorld.maps[mCurrentWorldArea].name, 1061, 69,
		color(214, 222, 236), 12, 112);
	auto tab = [this](const SDL_Rect& rect, const std::string& label, bool active)
	{
		fillRect(rect, active ? 82 : 38, active ? 67 : 46, active ? 39 : 65, 245);
		outlineRect(rect, active ? 235 : 109, active ? 184 : 120, active ? 80 : 143, 255, 2);
		drawText(label, rect.x + 4, rect.y + 11, color(235, 238, 245),
			label.size() > 5 ? 7 : 9, rect.w - 7);
	};
	tab(BUILDER_TILES_TAB, "TILE", mWorldBuilderTab == WorldBuilderTab::Tiles);
	tab(BUILDER_NPCS_TAB, "NPC", mWorldBuilderTab == WorldBuilderTab::Npcs);
	tab(BUILDER_OBJECTS_TAB, "OBJECT", mWorldBuilderTab == WorldBuilderTab::Objects);
	tab(BUILDER_REGIONS_TAB, "REGION", mWorldBuilderTab == WorldBuilderTab::Regions);
	tab(BUILDER_PORTALS_TAB, "PORTAL", mWorldBuilderTab == WorldBuilderTab::Portals);

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
				color(232, 236, 244), 10, button.w - 12);
		}

		RtpTilesetFamily family = tilesetFamily(mWorldBuilderTileCategory);
		RtpTileSheet selectedSheet = (RtpTileSheet)mWorldBuilderTileSheet;
		const RtpSheetDescriptor* sheet = RtpTilesetRenderer::descriptor(family,
			selectedSheet);
		fillRect(BUILDER_PREVIOUS_SHEET, 34, 45, 63, 245);
		fillRect(BUILDER_SHEET_NAME, 28, 39, 57, 245);
		fillRect(BUILDER_NEXT_SHEET, 34, 45, 63, 245);
		outlineRect(BUILDER_PREVIOUS_SHEET, 100, 123, 151, 255, 1);
		outlineRect(BUILDER_SHEET_NAME, 100, 123, 151, 255, 1);
		outlineRect(BUILDER_NEXT_SHEET, 100, 123, 151, 255, 1);
		drawText("<", 1031, BUILDER_SHEET_Y + 7, color(224, 231, 242), 12);
		drawText(sheetName(selectedSheet), 1122, BUILDER_SHEET_Y + 7,
			color(235, 205, 112), 12, 38);
		drawText(">", 1231, BUILDER_SHEET_Y + 7, color(224, 231, 242), 12);
		mWorldBuilderHoveredTileName.clear();
		fillRect(BUILDER_TILESET_VIEW, 13, 18, 27, 255);
		outlineRect(BUILDER_TILESET_VIEW, 73, 92, 118, 255, 1);
		int hoveredTile = -1;
		if (sheet != NULL)
		{
			SDL_Rect sheetRect = catalogSheetRect(*sheet);
			SDL_Texture* texture = mAssets == NULL ? NULL :
				mAssets->texture(sheet->imagePath, true);
			if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &sheetRect);
			outlineRect(sheetRect, 109, 127, 151, 255, 1);
			hoveredTile = catalogTileAt(*sheet, sheetRect, mMouseX, mMouseY);
			SDL_Rect source;
			if (RtpTilesetRenderer::paletteTileSource(selectedSheet,
				mWorldBuilderCatalogTile, source))
			{
				SDL_Rect selected = scaledCatalogTileRect(*sheet, sheetRect, source);
				outlineRect(selected, 247, 194, 72, 255, 2);
			}
			if (hoveredTile >= 0 && hoveredTile != mWorldBuilderCatalogTile &&
				RtpTilesetRenderer::paletteTileSource(selectedSheet, hoveredTile, source))
			{
				SDL_Rect hovered = scaledCatalogTileRect(*sheet, sheetRect, source);
				outlineRect(hovered, 129, 218, 239, 255, 2);
			}
			if (hoveredTile >= 0) mWorldBuilderHoveredTileName =
				catalogTileName(family, selectedSheet, hoveredTile);
		}
		bool brushResizable = worldBuilderBrushResizable();
		int displayedBrushSize = brushResizable ? mWorldBuilderBrushSize : 1;
		fillRect(BUILDER_BRUSH_DECREASE, brushResizable ? 43 : 29,
			brushResizable ? 55 : 35, brushResizable ? 73 : 48, 245);
		fillRect(BUILDER_BRUSH_LABEL, 27, 37, 53, 245);
		fillRect(BUILDER_BRUSH_INCREASE, brushResizable ? 43 : 29,
			brushResizable ? 55 : 35, brushResizable ? 73 : 48, 245);
		outlineRect(BUILDER_BRUSH_DECREASE, brushResizable ? 116 : 67,
			brushResizable ? 143 : 78, brushResizable ? 174 : 96, 255, 1);
		outlineRect(BUILDER_BRUSH_LABEL, 88, 112, 143, 255, 1);
		outlineRect(BUILDER_BRUSH_INCREASE, brushResizable ? 116 : 67,
			brushResizable ? 143 : 78, brushResizable ? 174 : 96, 255, 1);
		drawText("-", 1034, 659, color(brushResizable ? 229 : 113,
			brushResizable ? 235 : 126, brushResizable ? 245 : 145), 14);
		drawText("BRUSH  " + std::to_string(displayedBrushSize) + " x " +
			std::to_string(displayedBrushSize), 1072, 661,
			color(brushResizable ? 224 : 130, brushResizable ? 232 : 143,
				brushResizable ? 243 : 162), 11, 132);
		drawText("+", 1227, 659, color(brushResizable ? 229 : 113,
			brushResizable ? 235 : 126, brushResizable ? 245 : 145), 14);
		fillRect(BUILDER_TILE_INFO, 27, 37, 53, 245);
		outlineRect(BUILDER_TILE_INFO, 88, 112, 143, 255, 1);
		std::string tileLabel = mWorldBuilderHoveredTileName.empty() ?
			catalogTileName(family, selectedSheet, mWorldBuilderCatalogTile) :
			mWorldBuilderHoveredTileName;
		int describedTile = hoveredTile >= 0 ? hoveredTile : mWorldBuilderCatalogTile;
		RtpTileReference described(family, selectedSheet, describedTile);
		described.layer = RtpTilesetRenderer::inferredLayer(described);
		RtpTileCollision collision = RtpTilesetRenderer::collision(described);
		std::string collisionText = family == RtpTilesetFamily::World ? "MAP ONLY" :
			(collision == RtpTileCollision::Walkable ? "WALKABLE" :
				(collision == RtpTileCollision::Blocked ? "BLOCKED" : "INHERITS"));
		drawText(tileLabel, 1028, 692, color(224, 232, 243), 9, 216);
		drawText(std::string(familyName(family)) + " " + sheetName(selectedSheet) +
			" #" + std::to_string(describedTile) + "  " +
			layerName(described.layer) + "  " + collisionText, 1028, 704,
			color(151, 181, 215), 8, 216);
	}
	else
	{
		mWorldBuilderHoveredTileName.clear();
		bool objectPalette = mWorldBuilderTab == WorldBuilderTab::Objects &&
			mWorldBuilderObjectPalette;
		int count = mWorldBuilderTab == WorldBuilderTab::Npcs ? (int)mNpcs.size() :
			(mWorldBuilderTab == WorldBuilderTab::Regions ? (int)mWorld.regions.size() :
			(mWorldBuilderTab == WorldBuilderTab::Portals ? (int)mWorld.portals.size() :
			(objectPalette ? (int)mWorldObjectTemplates.size() :
			(int)(mWorldObjects.size() + mMercerStock.shards.size()))));
		int selected = mWorldBuilderTab == WorldBuilderTab::Npcs ?
			mWorldBuilderSelectedNpc : (mWorldBuilderTab == WorldBuilderTab::Portals ?
			mWorldBuilderSelectedPortal : (mWorldBuilderTab == WorldBuilderTab::Regions ?
			mWorldBuilderSelectedRegion : (objectPalette ?
			mWorldBuilderSelectedObjectTemplate : mWorldBuilderSelectedObject)));
		int listRows = mWorldBuilderTab == WorldBuilderTab::Regions ?
			BUILDER_REGION_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Portals ?
			BUILDER_PORTAL_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Objects &&
			!objectPalette ? BUILDER_OBJECT_LIST_ROWS : BUILDER_LIST_ROWS));
		for (int row = 0; row < listRows; ++row)
		{
			int index = mWorldBuilderListScroll + row;
			if (index >= count) break;
			SDL_Rect item = { 1022, BUILDER_LIST_Y + row * BUILDER_LIST_ROW, 228, 34 };
			fillRect(item, index == selected ? 74 : (row % 2 ? 31 : 37),
				index == selected ? 61 : 42, index == selected ? 42 : 59, 238);
			if (index == selected) outlineRect(item, 233, 184, 82, 255, 2);
			if (objectPalette)
			{
				const WorldObject& object = mWorldObjectTemplates[index].object;
				if (object.kind == WorldObjectKind::Signpost)
				{
					fillRect({ item.x + 8, item.y + 17, 4, 13 }, 91, 58, 32, 255);
					fillRect({ item.x + 3, item.y + 7, 18, 11 }, 178, 125, 59, 255);
				}
				else if (!drawWorldObjectSprite(object, false,
					{ item.x + 1, item.y + 5, 24, 24 }, false))
				{
					fillRect({ item.x + 4, item.y + 8, 18, 20 }, 92, 112, 135, 255);
				}
				drawText(object.name, item.x + 27, item.y + 3,
					color(232, 236, 244), 9, 194);
				drawText(worldObjectKindName(object.kind), item.x + 27, item.y + 18,
					color(171, 192, 218), 8, 188);
			}
			else if (mWorldBuilderTab == WorldBuilderTab::Npcs)
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
			else if (mWorldBuilderTab == WorldBuilderTab::Portals)
			{
				const WorldPortal& portal = mWorld.portals[index];
				if (!portal.hasAppearance())
				{
					outlineRect({ item.x + 7, item.y + 7, 18, 18 },
						112, 137, 168, 255, 1);
					drawText("X", item.x + 12, item.y + 7,
						color(169, 188, 211), 10);
				}
				else if (!drawPortalSprite(portal, 0.f,
					{ item.x + 1, item.y + 2, 32, 32 }))
				{
					fillRect({ item.x + 5, item.y + 5, 22, 24 }, 102, 72, 48, 255);
					outlineRect({ item.x + 5, item.y + 5, 22, 24 }, 54, 37, 27, 255, 2);
				}
				drawText("F " + portal.fromMap + " " + std::to_string(portal.fromX) +
					"," + std::to_string(portal.fromY), item.x + 36, item.y + 3,
					color(205, 235, 241), 8, 186);
				drawText("T " + portal.toMap + " " + std::to_string(portal.toX) +
					"," + std::to_string(portal.toY), item.x + 36, item.y + 18,
					color(225, 208, 242), 8, 186);
			}
			else if (mWorldBuilderTab == WorldBuilderTab::Regions)
			{
				const WorldRegion& region = mWorld.regions[index];
				fillRect({ item.x + 4, item.y + 5, 7, 24 },
					region.connector ? 87 : 232, region.connector ? 190 : 184,
					region.connector ? 229 : 82, 255);
				drawText(region.name, item.x + 18, item.y + 3,
					color(232, 236, 244), 9, 202);
				drawText(std::string(region.snow ? "SNOW  " : "RAIN  ") +
					region.mapId + "  " + std::to_string(region.x) + "," +
					std::to_string(region.y) + "  " + std::to_string(region.width) +
					"x" + std::to_string(region.height), item.x + 18, item.y + 18,
					color(171, 192, 218), 8, 202);
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
						if (!drawWorldObjectSprite(mWorldObjects[index], false,
							{ item.x + 1, item.y + 5, 24, 24 }, false))
						{
							fillRect({ item.x + 3, item.y + 13, 20, 16 }, 117, 67, 31, 255);
							fillRect({ item.x + 3, item.y + 8, 20, 8 }, 165, 96, 39, 255);
							fillRect({ item.x + 11, item.y + 16, 5, 7 }, 225, 176, 66, 255);
						}
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
					worldObjectKindName(mWorldObjects[index].kind) : "Shard";
				drawText(kindLabel, item.x + 27, item.y + 18,
					color(171, 192, 218), 8, 80);
				drawText(std::to_string(entityX) + "," + std::to_string(entityY), item.x + 181,
					item.y + 10, color(173, 193, 220), 9);
			}
		}
		if (mWorldBuilderTab == WorldBuilderTab::Objects)
		{
			bool chestSelected = !objectPalette && mWorldBuilderSelectedObject >= 0 &&
				mWorldBuilderSelectedObject < (int)mWorldObjects.size() &&
				(mWorldObjects[mWorldBuilderSelectedObject].kind == WorldObjectKind::Chest ||
				mWorldObjects[mWorldBuilderSelectedObject].kind == WorldObjectKind::DeckChest);
			if (!objectPalette)
			{
				fillRect(BUILDER_PORTAL_APPEARANCE_PREVIOUS, chestSelected ? 43 : 29,
					chestSelected ? 55 : 35, chestSelected ? 73 : 48, 245);
				fillRect(BUILDER_PORTAL_APPEARANCE, chestSelected ? 37 : 29,
					chestSelected ? 54 : 37, chestSelected ? 70 : 49, 245);
				fillRect(BUILDER_PORTAL_APPEARANCE_NEXT, chestSelected ? 43 : 29,
					chestSelected ? 55 : 35, chestSelected ? 73 : 48, 245);
				outlineRect(BUILDER_PORTAL_APPEARANCE_PREVIOUS,
					chestSelected ? 116 : 67, chestSelected ? 143 : 78,
					chestSelected ? 174 : 96, 255, 1);
				outlineRect(BUILDER_PORTAL_APPEARANCE, chestSelected ? 88 : 67,
					chestSelected ? 112 : 78, chestSelected ? 143 : 96, 255, 1);
				outlineRect(BUILDER_PORTAL_APPEARANCE_NEXT, chestSelected ? 116 : 67,
					chestSelected ? 143 : 78, chestSelected ? 174 : 96, 255, 1);
				drawText("<", BUILDER_PORTAL_APPEARANCE_PREVIOUS.x + 9,
					BUILDER_PORTAL_APPEARANCE_PREVIOUS.y + 7,
					color(chestSelected ? 229 : 113, chestSelected ? 235 : 126,
						chestSelected ? 245 : 145), 13);
				drawText(">", BUILDER_PORTAL_APPEARANCE_NEXT.x + 9,
					BUILDER_PORTAL_APPEARANCE_NEXT.y + 7,
					color(chestSelected ? 229 : 113, chestSelected ? 235 : 126,
						chestSelected ? 245 : 145), 13);
				if (chestSelected)
				{
					const WorldObject& chest = mWorldObjects[mWorldBuilderSelectedObject];
					drawWorldObjectSprite(chest, false,
						{ BUILDER_PORTAL_APPEARANCE.x + 3,
						BUILDER_PORTAL_APPEARANCE.y + 2, 28, 28 }, false);
					drawText(chest.appearance, BUILDER_PORTAL_APPEARANCE.x + 35,
						BUILDER_PORTAL_APPEARANCE.y + 9,
						color(224, 232, 243), 9, 124);
				}
				else drawText("Select a chest", BUILDER_PORTAL_APPEARANCE.x + 35,
					BUILDER_PORTAL_APPEARANCE.y + 9,
					color(126, 139, 158), 9, 124);
			}
			fillRect(BUILDER_OBJECT_ADD, objectPalette ? 80 : 35,
				objectPalette ? 67 : 45, objectPalette ? 43 : 61, 245);
			fillRect(BUILDER_OBJECT_PLACED, objectPalette ? 35 : 80,
				objectPalette ? 45 : 67, objectPalette ? 61 : 43, 245);
			outlineRect(BUILDER_OBJECT_ADD, objectPalette ? 233 : 104,
				objectPalette ? 184 : 122, objectPalette ? 82 : 151, 255, 1);
			outlineRect(BUILDER_OBJECT_PLACED, objectPalette ? 104 : 233,
				objectPalette ? 122 : 184, objectPalette ? 151 : 82, 255, 1);
			drawText("ADD", BUILDER_OBJECT_ADD.x + 39, BUILDER_OBJECT_ADD.y + 6,
				color(232, 236, 244), 10);
			drawText("PLACED", BUILDER_OBJECT_PLACED.x + 25,
				BUILDER_OBJECT_PLACED.y + 6, color(232, 236, 244), 10);
			drawText(objectPalette ? "Select a type, then click the map" :
				"Drag to move • Delete removes created objects", 1022, 698,
				color(166, 184, 211), 9, 228);
		}
		else if (mWorldBuilderTab == WorldBuilderTab::Regions)
		{
			bool hasSelection = mWorldBuilderSelectedRegion >= 0 &&
				mWorldBuilderSelectedRegion < (int)mWorld.regions.size();
			bool creating = mWorldBuilderRegionPlacementMode == 1;
			bool changingBounds = mWorldBuilderRegionPlacementMode == 2;
			fillRect(BUILDER_REGION_NEW, creating ? 91 : 49, creating ? 48 : 63,
				creating ? 49 : 82, 245);
			fillRect(BUILDER_REGION_BOUNDS, changingBounds ? 91 : (hasSelection ? 49 : 31),
				changingBounds ? 48 : (hasSelection ? 63 : 42),
				changingBounds ? 49 : (hasSelection ? 82 : 57), 245);
			fillRect(BUILDER_REGION_DELETE, hasSelection ? 82 : 38,
				hasSelection ? 48 : 41, hasSelection ? 47 : 56, 245);
			outlineRect(BUILDER_REGION_NEW, creating ? 232 : 123,
				creating ? 113 : 151, creating ? 101 : 184, 255, 1);
			outlineRect(BUILDER_REGION_BOUNDS, changingBounds ? 232 : 108,
				changingBounds ? 113 : 132, changingBounds ? 101 : 164, 255, 1);
			outlineRect(BUILDER_REGION_DELETE, hasSelection ? 218 : 91,
				hasSelection ? 112 : 101, hasSelection ? 94 : 124, 255, 1);
			drawText(creating ? "CANCEL" : "NEW", BUILDER_REGION_NEW.x +
				(creating ? 11 : 22), BUILDER_REGION_NEW.y + 6,
				color(232, 236, 244), 10);
			drawText(changingBounds ? "CANCEL" : "BOUNDS",
				BUILDER_REGION_BOUNDS.x + (changingBounds ? 14 : 12),
				BUILDER_REGION_BOUNDS.y + 6,
				color(hasSelection || changingBounds ? 232 : 126,
					hasSelection || changingBounds ? 236 : 139,
					hasSelection || changingBounds ? 244 : 158), 9);
			drawText("DELETE", BUILDER_REGION_DELETE.x + 13,
				BUILDER_REGION_DELETE.y + 6,
				color(hasSelection ? 244 : 126, hasSelection ? 207 : 139,
					hasSelection ? 199 : 158), 9);
			bool connector = hasSelection &&
				mWorld.regions[mWorldBuilderSelectedRegion].connector;
			bool snow = hasSelection &&
				mWorld.regions[mWorldBuilderSelectedRegion].snow;
			fillRect(BUILDER_REGION_KIND, hasSelection ? 37 : 29,
				hasSelection ? 54 : 37, hasSelection ? 70 : 49, 245);
			fillRect(BUILDER_REGION_WEATHER, hasSelection ? 37 : 29,
				hasSelection ? 54 : 37, hasSelection ? 70 : 49, 245);
			outlineRect(BUILDER_REGION_KIND, connector ? 87 : (hasSelection ? 203 : 80),
				connector ? 190 : (hasSelection ? 165 : 94),
				connector ? 229 : (hasSelection ? 76 : 112), 255, 1);
			outlineRect(BUILDER_REGION_WEATHER, snow ? 192 : (hasSelection ? 91 : 80),
				snow ? 221 : (hasSelection ? 151 : 94),
				snow ? 239 : (hasSelection ? 217 : 112), 255, 1);
			drawText(hasSelection ? std::string("TYPE: ") +
				(connector ? "CONNECT" : "TOWN") : "TYPE: --",
				BUILDER_REGION_KIND.x + 8, BUILDER_REGION_KIND.y + 7,
				color(hasSelection ? 229 : 126, hasSelection ? 235 : 139,
					hasSelection ? 244 : 158), 9, 96);
			drawText(hasSelection ? std::string("WEATHER: ") +
				(snow ? "SNOW" : "RAIN") : "WEATHER: --",
				BUILDER_REGION_WEATHER.x + 7, BUILDER_REGION_WEATHER.y + 7,
				color(hasSelection ? 229 : 126, hasSelection ? 235 : 139,
					hasSelection ? 244 : 158), 8, 101);
			fillRect(BUILDER_REGION_NAME, mWorldBuilderRegionNameFocused ? 48 : 27,
				mWorldBuilderRegionNameFocused ? 55 : 37,
				mWorldBuilderRegionNameFocused ? 68 : 53, 245);
			outlineRect(BUILDER_REGION_NAME, mWorldBuilderRegionNameFocused ? 112 : 88,
				mWorldBuilderRegionNameFocused ? 174 : 112,
				mWorldBuilderRegionNameFocused ? 226 : 143, 255, 1);
			std::string name = hasSelection ? mWorldBuilderRegionNameInput :
				"Select a region to edit its name";
			if (mWorldBuilderRegionNameFocused) name += "_";
			drawText(name, BUILDER_REGION_NAME.x + 9, BUILDER_REGION_NAME.y + 10,
				color(hasSelection ? 232 : 126, hasSelection ? 236 : 139,
					hasSelection ? 244 : 158), 10, 210);
			std::string help = mWorldBuilderRegionPlacementMode != 0 ?
				"Drag map rectangle • Right-click cancels" :
				"Double-click locates • R opens this tab";
			drawText(help, 1022, 698, color(166, 184, 211), 9, 228);
		}
		else if (mWorldBuilderTab == WorldBuilderTab::Portals)
		{
			bool hasSelection = !mWorldBuilderPortalCreating &&
				mWorldBuilderSelectedPortal >= 0 &&
				mWorldBuilderSelectedPortal < (int)mWorld.portals.size();
			fillRect(BUILDER_PORTAL_NEW, mWorldBuilderPortalCreating ? 91 : 49,
				mWorldBuilderPortalCreating ? 48 : 63,
				mWorldBuilderPortalCreating ? 49 : 82, 245);
			outlineRect(BUILDER_PORTAL_NEW, mWorldBuilderPortalCreating ? 232 : 123,
				mWorldBuilderPortalCreating ? 113 : 151,
				mWorldBuilderPortalCreating ? 101 : 184, 255, 1);
			drawText(mWorldBuilderPortalCreating ? "CANCEL" : "NEW",
				BUILDER_PORTAL_NEW.x + (mWorldBuilderPortalCreating ? 13 : 23),
				BUILDER_PORTAL_NEW.y + 6, color(232, 236, 244), 10);
			bool fromSelected = hasSelection && mWorldBuilderPortalEndpoint == 0;
			bool toSelected = hasSelection && mWorldBuilderPortalEndpoint == 1;
			fillRect(BUILDER_PORTAL_APPEARANCE_PREVIOUS, hasSelection ? 43 : 29,
				hasSelection ? 55 : 35, hasSelection ? 73 : 48, 245);
			fillRect(BUILDER_PORTAL_APPEARANCE, hasSelection ? 37 : 29,
				hasSelection ? 54 : 37, hasSelection ? 70 : 49, 245);
			fillRect(BUILDER_PORTAL_APPEARANCE_NEXT, hasSelection ? 43 : 29,
				hasSelection ? 55 : 35, hasSelection ? 73 : 48, 245);
			outlineRect(BUILDER_PORTAL_APPEARANCE_PREVIOUS, hasSelection ? 116 : 67,
				hasSelection ? 143 : 78, hasSelection ? 174 : 96, 255, 1);
			outlineRect(BUILDER_PORTAL_APPEARANCE, hasSelection ? 88 : 67,
				hasSelection ? 112 : 78, hasSelection ? 143 : 96, 255, 1);
			outlineRect(BUILDER_PORTAL_APPEARANCE_NEXT, hasSelection ? 116 : 67,
				hasSelection ? 143 : 78, hasSelection ? 174 : 96, 255, 1);
			drawText("<", BUILDER_PORTAL_APPEARANCE_PREVIOUS.x + 9,
				BUILDER_PORTAL_APPEARANCE_PREVIOUS.y + 7,
				color(hasSelection ? 229 : 113, hasSelection ? 235 : 126,
					hasSelection ? 245 : 145), 13);
			drawText(">", BUILDER_PORTAL_APPEARANCE_NEXT.x + 9,
				BUILDER_PORTAL_APPEARANCE_NEXT.y + 7,
				color(hasSelection ? 229 : 113, hasSelection ? 235 : 126,
					hasSelection ? 245 : 145), 13);
			std::string appearance = hasSelection ? portalAppearanceLabel(
				mWorld.portals[mWorldBuilderSelectedPortal].appearance) : "Select a portal";
			if (hasSelection)
			{
				const WorldPortal& portal = mWorld.portals[mWorldBuilderSelectedPortal];
				if (portal.hasAppearance())
					drawPortalSprite(portal, 0.f,
						{ BUILDER_PORTAL_APPEARANCE.x + 3,
						BUILDER_PORTAL_APPEARANCE.y + 2, 28, 28 });
				else
				{
					outlineRect({ BUILDER_PORTAL_APPEARANCE.x + 7,
						BUILDER_PORTAL_APPEARANCE.y + 6, 20, 20 },
						112, 137, 168, 255, 1);
					drawText("X", BUILDER_PORTAL_APPEARANCE.x + 13,
						BUILDER_PORTAL_APPEARANCE.y + 7,
						color(169, 188, 211), 11);
				}
				drawText(appearance, BUILDER_PORTAL_APPEARANCE.x + 35,
					BUILDER_PORTAL_APPEARANCE.y + 9, color(224, 232, 243), 9, 124);
			}
			else drawText(appearance, BUILDER_PORTAL_APPEARANCE.x + 31,
				BUILDER_PORTAL_APPEARANCE.y + 9, color(126, 139, 158), 9, 130);
			fillRect(BUILDER_PORTAL_FROM, fromSelected ? 51 : 31,
				fromSelected ? 88 : 48, fromSelected ? 94 : 67, 245);
			fillRect(BUILDER_PORTAL_TO, toSelected ? 78 : 43,
				toSelected ? 57 : 43, toSelected ? 104 : 68, 245);
			outlineRect(BUILDER_PORTAL_FROM, fromSelected ? 88 : 83,
				fromSelected ? 220 : 111, fromSelected ? 229 : 137, 255, 1);
			outlineRect(BUILDER_PORTAL_TO, toSelected ? 193 : 101,
				toSelected ? 134 : 103, toSelected ? 235 : 141, 255, 1);
			drawText("FROM", BUILDER_PORTAL_FROM.x + 18, BUILDER_PORTAL_FROM.y + 6,
				color(hasSelection ? 232 : 126, hasSelection ? 236 : 139,
					hasSelection ? 244 : 158), 10);
			drawText("TO", BUILDER_PORTAL_TO.x + 31, BUILDER_PORTAL_TO.y + 6,
				color(hasSelection ? 232 : 126, hasSelection ? 236 : 139,
					hasSelection ? 244 : 158), 10);
			std::string help = mWorldBuilderPortalCreating ?
				(mWorldBuilderPortalEndpoint == 0 ? "Click the origin tile" :
				"Switch maps if needed; click destination") :
				"Directed portals • Delete removes selected";
			drawText(help, 1022, 698, color(166, 184, 211), 9, 228);
		}
		else
		{
			int visibleRows = mWorldBuilderTab == WorldBuilderTab::Regions ?
				BUILDER_REGION_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Portals ?
				BUILDER_PORTAL_LIST_ROWS : (mWorldBuilderTab == WorldBuilderTab::Objects &&
				!objectPalette ? BUILDER_OBJECT_LIST_ROWS : BUILDER_LIST_ROWS));
			if (count > visibleRows)
				drawText("Mouse wheel scrolls", 1052, 682, color(166, 184, 211), 12);
			drawText("Click: select  •  Double-click: locate", 1022, 701,
				color(166, 184, 211), 10, 224);
		}
	}

	bool canUndo = !mWorldBuilderUndoHistory.empty();
	fillRect(BUILDER_UNDO, canUndo ? 66 : 35, canUndo ? 64 : 43,
		canUndo ? 48 : 58, 250);
	outlineRect(BUILDER_UNDO, canUndo ? 197 : 91, canUndo ? 158 : 104,
		canUndo ? 72 : 125, 255, 2);
	drawText("UNDO", BUILDER_UNDO.x + 24, BUILDER_UNDO.y + 7,
		color(canUndo ? 245 : 135, canUndo ? 226 : 145, canUndo ? 181 : 162), 12);
	drawText("Ctrl+Z", BUILDER_UNDO.x + 24, BUILDER_UNDO.y + 24,
		color(canUndo ? 201 : 112, canUndo ? 211 : 124, canUndo ? 225 : 143), 8);
	fillRect(BUILDER_SAVE, mWorldBuilderDirty ? 74 : 45, mWorldBuilderDirty ? 76 : 55,
		mWorldBuilderDirty ? 43 : 67, 250);
	outlineRect(BUILDER_SAVE, 207, 161, 66, 255, 2);
	drawText("SAVE", BUILDER_SAVE.x + 15, BUILDER_SAVE.y + 8,
		color(245, 226, 181), 13);
	drawText("Ctrl+S", BUILDER_SAVE.x + 75, BUILDER_SAVE.y + 11,
		color(201, 211, 225), 9);
	drawText("T/N/O/P: tabs  •  G: grid  •  Arrows/WASD: pan  •  Wheel or +/-: zoom  •  [ / ]: brush",
		32, 650, color(180, 196, 219), 14, 930);
	drawText("Tiles: paint  •  Entities: place  •  Ctrl+N: new map  •  PageUp/PageDown: maps",
		32, 676, color(142, 173, 217), 13);
	if (!mWorldBuilderNotice.empty() && SDL_GetTicks() < mWorldBuilderNoticeUntil)
	{
		fillRect({ 32, 716, 930, 42 }, mWorldBuilderNoticeError ? 71 : 25,
			mWorldBuilderNoticeError ? 30 : 62, mWorldBuilderNoticeError ? 31 : 43, 235);
		drawText(mWorldBuilderNotice, 45, 727,
			mWorldBuilderNoticeError ? color(255, 176, 166) : color(132, 234, 156), 15, 900);
	}
	if (mWorldBuilderMapDialogOpen)
	{
		fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 3, 7, 13, 185);
		fillRect(BUILDER_MAP_DIALOG, 23, 31, 47, 255);
		outlineRect(BUILDER_MAP_DIALOG, 210, 166, 75, 255, 3);
		drawText("NEW MAP", BUILDER_MAP_DIALOG.x + 28,
			BUILDER_MAP_DIALOG.y + 22, color(244, 213, 126), 24);
		drawText("Creates an empty, blocked catalog map. Paint ground tiles to make it walkable.",
			BUILDER_MAP_DIALOG.x + 28, BUILDER_MAP_DIALOG.y + 61,
			color(174, 191, 214), 11, BUILDER_MAP_DIALOG.w - 56);
		const SDL_Rect fields[] = { BUILDER_MAP_ID, BUILDER_MAP_NAME,
			BUILDER_MAP_WIDTH, BUILDER_MAP_HEIGHT };
		const char* labels[] = { "MAP ID", "DISPLAY NAME", "WIDTH", "HEIGHT" };
		const std::string values[] = { mWorldBuilderMapIdInput,
			mWorldBuilderMapNameInput, mWorldBuilderMapWidthInput,
			mWorldBuilderMapHeightInput };
		for (int field = 0; field < 4; ++field)
		{
			bool active = field == mWorldBuilderMapDialogField;
			int labelX = field < 2 ? fields[field].x - 142 : fields[field].x;
			int labelY = field < 2 ? fields[field].y + 12 : fields[field].y - 17;
			drawText(labels[field], labelX, labelY, color(185, 200, 220), 10);
			fillRect(fields[field], active ? 43 : 30, active ? 57 : 42,
				active ? 75 : 59, 255);
			outlineRect(fields[field], active ? 104 : 73, active ? 190 : 101,
				active ? 224 : 135, 255, active ? 2 : 1);
			std::string value = values[field];
			if (active && (SDL_GetTicks() / 500) % 2 == 0) value += "_";
			drawText(value, fields[field].x + 10, fields[field].y + 11,
				color(232, 237, 245), 11, fields[field].w - 20);
		}
		fillRect(BUILDER_MAP_INDOOR, mWorldBuilderMapIndoorInput ? 55 : 31,
			mWorldBuilderMapIndoorInput ? 77 : 44,
			mWorldBuilderMapIndoorInput ? 62 : 59, 255);
		outlineRect(BUILDER_MAP_INDOOR, mWorldBuilderMapIndoorInput ? 141 : 78,
			mWorldBuilderMapIndoorInput ? 204 : 105,
			mWorldBuilderMapIndoorInput ? 128 : 139, 255, 2);
		drawText(mWorldBuilderMapIndoorInput ? "INDOOR: YES" : "INDOOR: NO",
			BUILDER_MAP_INDOOR.x + 16, BUILDER_MAP_INDOOR.y + 11,
			color(224, 232, 241), 11);
		drawText("1-1024 tiles", BUILDER_MAP_WIDTH.x,
			BUILDER_MAP_WIDTH.y + BUILDER_MAP_WIDTH.h + 9,
			color(132, 153, 181), 9, 356);
		fillRect(BUILDER_MAP_CANCEL, 48, 48, 62, 255);
		outlineRect(BUILDER_MAP_CANCEL, 130, 137, 162, 255, 2);
		drawText("CANCEL", BUILDER_MAP_CANCEL.x + 29,
			BUILDER_MAP_CANCEL.y + 13, color(220, 226, 237), 12);
		fillRect(BUILDER_MAP_CREATE, 62, 78, 48, 255);
		outlineRect(BUILDER_MAP_CREATE, 191, 177, 80, 255, 2);
		drawText("CREATE MAP", BUILDER_MAP_CREATE.x + 22,
			BUILDER_MAP_CREATE.y + 13, color(244, 229, 181), 12);
		if (!mWorldBuilderMapDialogError.empty())
			drawText(mWorldBuilderMapDialogError, BUILDER_MAP_DIALOG.x + 28,
				BUILDER_MAP_DIALOG.y + 324, color(255, 154, 144), 11,
				BUILDER_MAP_DIALOG.w - 56);
		drawText("Tab changes fields • Enter creates • Esc cancels",
			BUILDER_MAP_DIALOG.x + 28, BUILDER_MAP_DIALOG.y + 414,
			color(139, 161, 191), 10, BUILDER_MAP_DIALOG.w - 56);
	}
}
