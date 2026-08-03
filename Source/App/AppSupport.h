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
	constexpr int MAP_MAX_COLUMNS = 20;
	constexpr int MAP_MAX_ROWS = 12;

	inline int mapOriginX(int columns)
	{
		return MAP_X + (MAP_MAX_COLUMNS - columns) * TILE / 2;
	}

	inline int mapOriginY(int rows)
	{
		return MAP_Y + (MAP_MAX_ROWS - rows) * TILE / 2;
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
