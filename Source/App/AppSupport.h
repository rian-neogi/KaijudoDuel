#pragma once

#include "Game/Message.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
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
	constexpr int OVERWORLD_VIEW_COLUMNS = 25;
	constexpr int WORLD_MAX_COLUMNS = 1024;
	constexpr int WORLD_MAX_ROWS = 1024;
	constexpr int MAP_VIEW_WIDTH = MAP_VIEW_COLUMNS * TILE;
	constexpr int MAP_VIEW_HEIGHT = MAP_VIEW_ROWS * TILE;
	constexpr int OVERWORLD_VIEW_WIDTH = OVERWORLD_VIEW_COLUMNS * TILE;
	constexpr const char* STARTER_DECK_PATH = "Decks/Starter/Fire.txt";

	struct TileBounds
	{
		int left;
		int top;
		int right;
		int bottom;

		int tileCount() const
		{
			return std::max(0, right - left) * std::max(0, bottom - top);
		}

		bool contains(int x, int y) const
		{
			return x >= left && x < right && y >= top && y < bottom;
		}

		bool intersects(int otherLeft, int otherTop, int otherRight, int otherBottom) const
		{
			return otherLeft < right && otherRight > left &&
				otherTop < bottom && otherBottom > top;
		}
	};

	inline TileBounds visibleTileBounds(float cameraX, float cameraY,
		int columns, int rows, int viewportColumns = MAP_VIEW_COLUMNS,
		int viewportRows = MAP_VIEW_ROWS, int margin = 1)
	{
		TileBounds result;
		result.left = std::max(0, (int)std::floor(cameraX) - margin);
		result.top = std::max(0, (int)std::floor(cameraY) - margin);
		result.right = std::min(columns,
			(int)std::ceil(cameraX + viewportColumns) + margin);
		result.bottom = std::min(rows,
			(int)std::ceil(cameraY + viewportRows) + margin);
		return result;
	}

	inline int mapOriginX(int columns, int viewportColumns = MAP_VIEW_COLUMNS)
	{
		int visibleColumns = columns < viewportColumns ? columns : viewportColumns;
		return MAP_X + (viewportColumns - visibleColumns) * TILE / 2;
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
