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

using namespace AppSupport;

namespace
{
	const SDL_Rect BUILDER_PANEL = { 1008, 18, 256, 764 };
	const SDL_Rect BUILDER_TILES_TAB = { 1022, 67, 70, 35 };
	const SDL_Rect BUILDER_NPCS_TAB = { 1096, 67, 70, 35 };
	const SDL_Rect BUILDER_SHARDS_TAB = { 1170, 67, 80, 35 };
	const SDL_Rect BUILDER_SAVE = { 1022, 724, 228, 44 };
	const int BUILDER_LIST_Y = 126;
	const int BUILDER_LIST_ROW = 39;
	const int BUILDER_LIST_ROWS = 14;
	const char TILE_TYPES[] = { '.', '=', '~', 'H', 'T', '#' };
	const char* TILE_NAMES[] = { "Grass", "Path", "Water", "House", "Tree", "Forest" };

	SDL_Rect paletteRect(int index)
	{
		return { 1022 + (index % 2) * 116, 130 + (index / 2) * 62, 108, 50 };
	}

	bool mapCellAt(int x, int y, int& cellX, int& cellY)
	{
		cellX = (x - MAP_X) / TILE;
		cellY = (y - MAP_Y) / TILE;
		return x >= MAP_X && y >= MAP_Y && cellX >= 0 && cellX < 20 &&
			cellY >= 0 && cellY < 12;
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
	lua_getfield(state, root, "map");
	if (!lua_istable(state, -1) || lua_rawlen(state, -1) != 12)
	{
		error = "world map must contain exactly 12 rows";
		lua_close(state);
		return false;
	}
	std::vector<std::string> loaded;
	for (int row = 1; row <= 12; ++row)
	{
		lua_rawgeti(state, -1, row);
		std::string value = lua_isstring(state, -1) ? lua_tostring(state, -1) : "";
		lua_pop(state, 1);
		if (value.size() != 20 || value.find_first_not_of(".=~HT#") != std::string::npos)
		{
			error = "row " + std::to_string(row) + " must contain 20 valid tile characters";
			lua_close(state);
			return false;
		}
		loaded.push_back(value);
	}
	lua_pop(state, 1);
	if (loaded[10][2] != '.' && loaded[10][2] != '=')
	{
		error = "player start at 2,10 must be grass or path";
		lua_close(state);
		return false;
	}

	std::vector<std::pair<int, int> > npcPositions(mNpcs.size());
	std::vector<std::pair<int, int> > shardPositions(mMercerStock.shards.size());
	std::set<std::pair<int, int> > occupied;
	occupied.insert(std::make_pair(2, 10));
	auto readPositions = [&state, &loaded, &occupied, &error, allowMissingPositions](int rootTable,
		const char* field, const std::vector<std::string>& ids,
		std::vector<std::pair<int, int> >& positions) -> bool
	{
		lua_getfield(state, rootTable, field);
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
				if (allowMissingPositions)
				{
					positions[i] = std::make_pair(-1, -1);
					continue;
				}
				error = std::string("missing ") + field + " position for '" + ids[i] + "'";
				lua_pop(state, 1);
				return false;
			}
			lua_getfield(state, -1, "x");
			int x = lua_isnumber(state, -1) ? (int)lua_tointeger(state, -1) : -1;
			lua_pop(state, 1);
			lua_getfield(state, -1, "y");
			int y = lua_isnumber(state, -1) ? (int)lua_tointeger(state, -1) : -1;
			lua_pop(state, 2);
			if (x < 0 || x >= 20 || y < 0 || y >= 12 ||
				(loaded[y][x] != '.' && loaded[y][x] != '=') ||
				!occupied.insert(std::make_pair(x, y)).second)
			{
				error = std::string("invalid or occupied ") + field + " position for '" + ids[i] + "'";
				lua_pop(state, 1);
				return false;
			}
			positions[i] = std::make_pair(x, y);
		}
		lua_pop(state, 1);
		return true;
	};
	std::vector<std::string> npcIds;
	for (size_t i = 0; i < mNpcs.size(); ++i) npcIds.push_back(mNpcs[i].id);
	std::vector<std::string> shardIds;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		shardIds.push_back(mMercerStock.shards[i].id);
	if (!readPositions(root, "npcs", npcIds, npcPositions) ||
		!readPositions(root, "shards", shardIds, shardPositions))
	{
		lua_close(state);
		return false;
	}
	bool placedMissing = false;
	auto placeMissing = [&loaded, &occupied, &error, &placedMissing](
		std::vector<std::pair<int, int> >& positions) -> bool
	{
		for (size_t i = 0; i < positions.size(); ++i)
		{
			if (positions[i].first >= 0) continue;
			bool placed = false;
			for (int y = 0; y < 12 && !placed; ++y)
				for (int x = 0; x < 20 && !placed; ++x)
					if ((loaded[y][x] == '.' || loaded[y][x] == '=') &&
						occupied.insert(std::make_pair(x, y)).second)
					{
						positions[i] = std::make_pair(x, y);
						placed = true;
						placedMissing = true;
					}
			if (!placed)
			{
				error = "no free walkable tile is available for new world entities";
				return false;
			}
		}
		return true;
	};
	if (!placeMissing(npcPositions) || !placeMissing(shardPositions))
	{
		lua_close(state);
		return false;
	}
	lua_close(state);
	mMap.swap(loaded);
	for (size_t i = 0; i < mNpcs.size(); ++i)
		mNpcs[i].setPosition(npcPositions[i].first, npcPositions[i].second);
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		mMercerStock.shards[i].x = shardPositions[i].first;
		mMercerStock.shards[i].y = shardPositions[i].second;
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

void Application::showWorldBuilderNotice(const std::string& notice, bool error)
{
	mWorldBuilderNotice = notice;
	mWorldBuilderNoticeError = error;
	mWorldBuilderNoticeUntil = SDL_GetTicks() + 4500;
}

bool Application::worldBuilderCanPlace(int x, int y, int ignoredNpc, int ignoredShard) const
{
	if (!isWalkable(x, y) || (x == 2 && y == 10)) return false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if ((int)i != ignoredNpc && mNpcs[i].x == x && mNpcs[i].y == y) return false;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		if ((int)i != ignoredShard && mMercerStock.shards[i].x == x &&
			mMercerStock.shards[i].y == y) return false;
	return true;
}

void Application::paintWorldBuilderTile(int x, int y)
{
	if (y < 0 || y >= (int)mMap.size() || x < 0 || x >= (int)mMap[y].size()) return;
	bool walkable = mWorldBuilderTile == '.' || mWorldBuilderTile == '=';
	if (!walkable)
	{
		bool hasNpc = false;
		for (size_t i = 0; i < mNpcs.size(); ++i)
			if (mNpcs[i].x == x && mNpcs[i].y == y) hasNpc = true;
		if ((x == 2 && y == 10) || hasNpc)
		{
			showWorldBuilderNotice("Move the player start or NPC before blocking this tile.", true);
			return;
		}
		for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
			if (mMercerStock.shards[i].x == x && mMercerStock.shards[i].y == y)
			{
				showWorldBuilderNotice("Move the shard before blocking this tile.", true);
				return;
			}
	}
	if (mMap[y][x] == mWorldBuilderTile) return;
	mMap[y][x] = mWorldBuilderTile;
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
		if (npc.x == x && npc.y == y) return;
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
		if (shard.x == x && shard.y == y) return;
		shard.x = x;
		shard.y = y;
		mWorldBuilderDirty = true;
	}
}

bool Application::saveWorldBuilder(std::string& error)
{
	std::ostringstream world;
	world << "-- World Builder map data. Each character is one 48x48 overworld tile.\n"
		<< "-- This file is entirely maintained by the World Builder.\n"
		<< "-- . grass, = path, ~ water, H house, T tree, # dense forest\nreturn {\n\tmap = {\n";
	for (size_t row = 0; row < mMap.size(); ++row)
		world << "\t\t\"" << mMap[row] << "\"" << (row + 1 == mMap.size() ? "\n" : ",\n");
	world << "\t},\n\tnpcs = {\n";
	for (size_t i = 0; i < mNpcs.size(); ++i)
		world << "\t\t[\"" << mNpcs[i].id << "\"] = { x = " << mNpcs[i].x <<
			", y = " << mNpcs[i].y << " },\n";
	world << "\t},\n\tshards = {\n";
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		world << "\t\t[\"" << mMercerStock.shards[i].id << "\"] = { x = " <<
			mMercerStock.shards[i].x << ", y = " << mMercerStock.shards[i].y << " },\n";
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
		if (key >= SDLK_1 && key <= SDLK_6)
		{
			mWorldBuilderTab = WorldBuilderTab::Tiles;
			mWorldBuilderTile = TILE_TYPES[key - SDLK_1];
			return;
		}
		if (key == SDLK_t) mWorldBuilderTab = WorldBuilderTab::Tiles;
		else if (key == SDLK_n) mWorldBuilderTab = WorldBuilderTab::Npcs;
		else if (key == SDLK_s) mWorldBuilderTab = WorldBuilderTab::Shards;
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
		if (mapCellAt(x, y, cellX, cellY))
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
			for (int i = 0; i < 6; ++i)
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
				mWorldBuilderSelectedNpc = selected;
			else if (mWorldBuilderTab == WorldBuilderTab::Shards &&
				selected < (int)mMercerStock.shards.size()) mWorldBuilderSelectedShard = selected;
			return;
		}
		int cellX, cellY;
		if (!mapCellAt(x, y, cellX, cellY)) return;
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
				if (mNpcs[i].x == cellX && mNpcs[i].y == cellY) hit = (int)i;
			if (hit >= 0) mWorldBuilderSelectedNpc = hit;
			else placeWorldBuilderSelection(cellX, cellY);
		}
		else
		{
			int hit = -1;
			for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
				if (mMercerStock.shards[i].x == cellX && mMercerStock.shards[i].y == cellY) hit = (int)i;
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
	drawText(mWorldBuilderDirty ? "UNSAVED CHANGES" : "SAVED", 795, 19,
		mWorldBuilderDirty ? color(244, 139, 88) : color(105, 218, 139), 14);
	for (size_t row = 0; row < mMap.size(); ++row)
	{
		for (size_t column = 0; column < mMap[row].size(); ++column)
		{
			SDL_Rect tile = { MAP_X + (int)column * TILE, MAP_Y + (int)row * TILE, TILE, TILE };
			char type = mMap[row][column];
			if (type == '=') fillRect(tile, 162, 132, 76);
			else if (type == '~') fillRect(tile, 25, 111, 157);
			else if (type == 'H') fillRect(tile, 126, 65, 43);
			else if (type == '#' || type == 'T') fillRect(tile, 26, 75, 33);
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
			outlineRect(tile, 10, 20, 27, 100, 1);
		}
	}

	fillRect({ MAP_X + 2 * TILE + 13, MAP_Y + 10 * TILE + 12, 23, 25 }, 24, 66, 137, 235);
	drawText("P", MAP_X + 2 * TILE + 19, MAP_Y + 10 * TILE + 16, color(215, 232, 255), 14);
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		int x = MAP_X + shard.x * TILE;
		int y = MAP_Y + shard.y * TILE;
		fillRect({ x + 18, y + 9, 14, 30 }, 47, 25, 71, 230);
		fillRect({ x + 13, y + 15, 24, 18 }, 146, 87, 211, 250);
		fillRect({ x + 19, y + 19, 12, 10 }, 231, 193, 255, 255);
		if ((int)i == mWorldBuilderSelectedShard && mWorldBuilderTab == WorldBuilderTab::Shards)
			outlineRect({ x + 2, y + 2, TILE - 4, TILE - 4 }, 246, 211, 99, 255, 3);
	}
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		drawCharacter((float)mNpcs[i].x, (float)mNpcs[i].y, mNpcs[i].appearance, false, false);
		if ((int)i == mWorldBuilderSelectedNpc && mWorldBuilderTab == WorldBuilderTab::Npcs)
			outlineRect({ MAP_X + mNpcs[i].x * TILE + 2, MAP_Y + mNpcs[i].y * TILE + 2,
				TILE - 4, TILE - 4 }, 246, 211, 99, 255, 3);
	}

	fillRect(BUILDER_PANEL, 20, 28, 44, 248);
	outlineRect(BUILDER_PANEL, 184, 140, 60, 255, 2);
	drawText("EDIT WORLD", 1027, 32, color(240, 205, 108), 21);
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
		for (int i = 0; i < 6; ++i)
		{
			SDL_Rect button = paletteRect(i);
			bool selected = mWorldBuilderTile == TILE_TYPES[i];
			fillRect(button, selected ? 79 : 38, selected ? 68 : 47, selected ? 43 : 64, 245);
			outlineRect(button, selected ? 241 : 110, selected ? 190 : 125,
				selected ? 87 : 147, 255, 2);
			drawText(std::to_string(i + 1) + "  " + TILE_NAMES[i], button.x + 9,
				button.y + 15, color(236, 239, 246), 13);
		}
		drawText("Click and drag across the map to paint.", 1022, 334,
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
	drawText("T/N/S: tabs  •  Tiles: click/drag  •  Entities: select then click/drag  •  Esc: exit",
		32, 650, color(180, 196, 219), 14, 930);
	drawText("P marks the fixed player start at 2,10.", 32, 676, color(142, 173, 217), 13);
	if (!mWorldBuilderNotice.empty() && SDL_GetTicks() < mWorldBuilderNoticeUntil)
	{
		fillRect({ 32, 716, 930, 42 }, mWorldBuilderNoticeError ? 71 : 25,
			mWorldBuilderNoticeError ? 30 : 62, mWorldBuilderNoticeError ? 31 : 43, 235);
		drawText(mWorldBuilderNotice, 45, 727,
			mWorldBuilderNoticeError ? color(255, 176, 166) : color(132, 234, 156), 15, 900);
	}
}
