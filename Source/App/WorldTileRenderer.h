#pragma once

#include "WorldTile.h"

#include <SDL.h>

class AssetManager;

class WorldTileRenderer
{
public:
	WorldTileRenderer(SDL_Renderer* renderer, AssetManager* assets);

	// Draws a direct 32-by-32 atlas tile beneath the existing procedural
	// details. Tiles without an atlas mapping retain their old rendering.
	bool drawTerrain(WorldTileId tile, const SDL_Rect& destination);

	// Redraws the terrain and its transparent decoration as one atomic visual.
	// If either atlas entry is unavailable, the procedural decoration remains.
	bool drawDecorationTile(WorldTileId tile, const SDL_Rect& destination);

	static bool atlasSourceRect(int tileIndex, int columns, int tileSize,
		int textureWidth, int textureHeight, SDL_Rect& source);

private:
	WorldTileRenderer(const WorldTileRenderer&);
	WorldTileRenderer& operator=(const WorldTileRenderer&);

	SDL_Renderer* mRenderer;
	AssetManager* mAssets;
};
