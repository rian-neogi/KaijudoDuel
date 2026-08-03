#pragma once

#include "Game/Message.h"

#include <SDL.h>

#include <cstdlib>
#include <map>
#include <string>

namespace AppSupport
{
	constexpr int LOGICAL_WIDTH = 1280;
	constexpr int LOGICAL_HEIGHT = 800;
	constexpr int WINDOW_WIDTH = 1600;
	constexpr int WINDOW_HEIGHT = 1000;
	constexpr int MAP_X = 32;
	constexpr int MAP_Y = 54;
	constexpr int TILE = 48;
	constexpr int MAP_VIEW_COLUMNS = 20;
	constexpr int MAP_VIEW_ROWS = 12;
	constexpr int WORLD_MAX_COLUMNS = 128;
	constexpr int WORLD_MAX_ROWS = 128;
	constexpr int MAP_VIEW_WIDTH = MAP_VIEW_COLUMNS * TILE;
	constexpr int MAP_VIEW_HEIGHT = MAP_VIEW_ROWS * TILE;
	constexpr const char* STARTER_DECK_PATH = "Decks/My Decks/7 - L Tappy Tappy.txt";

	inline int mapOriginX(int columns)
	{
		int visibleColumns = columns < MAP_VIEW_COLUMNS ? columns : MAP_VIEW_COLUMNS;
		return MAP_X + (MAP_VIEW_COLUMNS - visibleColumns) * TILE / 2;
	}

	inline int mapOriginY(int rows)
	{
		int visibleRows = rows < MAP_VIEW_ROWS ? rows : MAP_VIEW_ROWS;
		return MAP_Y + (MAP_VIEW_ROWS - visibleRows) * TILE / 2;
	}

	inline SDL_Color color(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255)
	{
		SDL_Color result = { r, g, b, a };
		return result;
	}

	inline int messageInt(const Message& message, const std::string& key, int fallback = -1)
	{
		std::map<std::string, std::string>::const_iterator found = message.map.find(key);
		return found == message.map.end() ? fallback : std::atoi(found->second.c_str());
	}
}
