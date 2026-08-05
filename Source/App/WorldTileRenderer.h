#pragma once

#include "WorldTile.h"
#include "RtpTilesetRenderer.h"

#include <SDL.h>

#include <string>
#include <vector>

class AssetManager;
class RtpTilesetRenderer;

class WorldTileRenderer
{
public:
	WorldTileRenderer(SDL_Renderer* renderer, AssetManager* assets);
	~WorldTileRenderer();

	// Ground pass. Connections are derived from adjacent semantic tile IDs.
	bool drawTerrain(WorldTileId tile, const std::vector<std::string>& map,
		int tileX, int tileY, const SDL_Rect& destination);
	bool drawTerrain(WorldTileId tile, const SDL_Rect& destination);

	// Decoration pass is below actors. Tall decoration tops are drawn later by
	// drawForeground, allowing actors to walk behind tree canopies.
	bool drawDecoration(WorldTileId tile, const SDL_Rect& destination);
	bool drawForeground(WorldTileId tile, const SDL_Rect& destination);
	bool drawPreview(WorldTileId tile, const SDL_Rect& destination);
	bool drawCatalog(const RtpTileReference& tile, unsigned int connections,
		const SDL_Rect& destination, unsigned int animationFrame = 0);
	bool drawSignpost(const SDL_Rect& destination);
	bool drawChest(const SDL_Rect& destination, bool opened);
	bool drawShard(const SDL_Rect& destination);
	bool canDrawDecoration(WorldTileId tile) const;
	static bool hasDecoration(WorldTileId tile);
	static bool hasForeground(WorldTileId tile);

	static bool atlasSourceRect(int tileIndex, int columns, int tileSize,
		int textureWidth, int textureHeight, SDL_Rect& source);
	static bool autotileQuarterSource(int quadrant, unsigned int connections,
		SDL_Point& sourceQuarter);

	static constexpr unsigned int North = 1u << 0;
	static constexpr unsigned int East = 1u << 1;
	static constexpr unsigned int South = 1u << 2;
	static constexpr unsigned int West = 1u << 3;
	static constexpr unsigned int NorthWest = 1u << 4;
	static constexpr unsigned int NorthEast = 1u << 5;
	static constexpr unsigned int SouthEast = 1u << 6;
	static constexpr unsigned int SouthWest = 1u << 7;

private:
	WorldTileRenderer(const WorldTileRenderer&);
	WorldTileRenderer& operator=(const WorldTileRenderer&);

	unsigned int connectionsFor(WorldTileId tile, const std::vector<std::string>& map,
		int tileX, int tileY) const;
	bool drawAutotile(WorldTileId tile, unsigned int connections,
		const SDL_Rect& destination);

	SDL_Renderer* mRenderer;
	AssetManager* mAssets;
	RtpTilesetRenderer* mRtpTiles;
};
