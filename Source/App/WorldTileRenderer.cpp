#include "WorldTileRenderer.h"

#include "AssetManager.h"

namespace
{
	const char* OUTSIDE_AUTOTILES = "Resources/Graphics/Tilesets/Outside_A2.png";
	const char* INSIDE_AUTOTILES = "Resources/Graphics/Tilesets/Inside_A2.png";
	const char* OUTSIDE_WATER_AUTOTILES = "Resources/Graphics/Tilesets/Outside_A1.png";
	const char* OUTSIDE_TERRAIN = "Resources/Graphics/Tilesets/Outside_A5.png";
	const char* INSIDE_TERRAIN = "Resources/Graphics/Tilesets/Inside_A5.png";
	const char* OUTSIDE_DECOR = "Resources/Graphics/Tilesets/Outside_B.png";

	struct AtlasTile
	{
		const char* path;
		int index;
		int columns;
		Uint8 red;
		Uint8 green;
		Uint8 blue;

		AtlasTile()
			: path(NULL), index(0), columns(0), red(255), green(255), blue(255)
		{
		}
	};

	struct Autotile
	{
		const char* path;
		int group;
		int family;
		Uint8 red;
		Uint8 green;
		Uint8 blue;
		bool animatedWater;

		Autotile()
			: path(NULL), group(0), family(0), red(255), green(255), blue(255),
			  animatedWater(false)
		{
		}
	};

	struct TileVisual
	{
		AtlasTile terrain;
		AtlasTile decoration;
		Autotile autotile;
	};

	void setAtlas(AtlasTile& atlas, const char* path, int index, int columns,
		Uint8 red = 255, Uint8 green = 255, Uint8 blue = 255)
	{
		atlas.path = path;
		atlas.index = index;
		atlas.columns = columns;
		atlas.red = red;
		atlas.green = green;
		atlas.blue = blue;
	}

	void setAutotile(Autotile& autotile, const char* path, int group, int family,
		Uint8 red = 255, Uint8 green = 255, Uint8 blue = 255,
		bool animatedWater = false)
	{
		autotile.path = path;
		autotile.group = group;
		autotile.family = family;
		autotile.red = red;
		autotile.green = green;
		autotile.blue = blue;
		autotile.animatedWater = animatedWater;
	}

	bool tileVisual(WorldTileId tile, TileVisual& visual)
	{
		switch (tile)
		{
		case WorldTiles::Grass:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			break;
		case WorldTiles::Water:
			setAutotile(visual.autotile, OUTSIDE_WATER_AUTOTILES, 0, 20,
				255, 255, 255, true);
			break;
		case WorldTiles::Path:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 17, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 1, 2);
			break;
		case WorldTiles::DuelSand:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 17, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 16, 3);
			break;
		case WorldTiles::OldRoadPath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 32, 8, 204, 191, 164);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 3, 4, 204, 191, 164);
			break;
		case WorldTiles::Marble:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 50, 8, 225, 235, 238);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 3, 5, 225, 235, 238);
			break;
		case WorldTiles::WoodFloor:
			setAtlas(visual.terrain, INSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, INSIDE_AUTOTILES, 0, 6);
			break;
		case WorldTiles::CinderrailGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 49, 8, 174, 151, 112);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 8, 7, 174, 151, 112);
			break;
		case WorldTiles::CinderrailPath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 17, 8, 191, 146, 100);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 10, 8, 191, 146, 100);
			break;
		case WorldTiles::CinderrailDuelSand:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 17, 8, 191, 146, 100);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 16, 9, 191, 146, 100);
			break;
		case WorldTiles::WatershedGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 19, 8, 177, 232, 182);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 10, 177, 232, 182);
			break;
		case WorldTiles::WatershedPath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 20, 8, 211, 207, 167);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 1, 11, 211, 207, 167);
			break;
		case WorldTiles::GlasswaterGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 48, 8, 160, 220, 230);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 3, 12, 160, 220, 230);
			break;
		case WorldTiles::GlasswaterPaving:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 50, 8, 178, 226, 239);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 19, 13, 178, 226, 239);
			break;
		case WorldTiles::GlasswaterArena:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 50, 8, 178, 226, 239);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 19, 14, 178, 226, 239);
			break;
		case WorldTiles::RootmazeGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8, 181, 226, 155);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 15, 181, 226, 155);
			break;
		case WorldTiles::RootmazePath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 18, 8, 185, 196, 125);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 1, 16, 185, 196, 125);
			break;
		case WorldTiles::RootmazeArena:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 18, 8, 185, 196, 125);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 1, 17, 185, 196, 125);
			break;
		case WorldTiles::BlackstoneGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 72, 8, 130, 130, 128);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 8, 18, 130, 130, 128);
			break;
		case WorldTiles::BlackstonePath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 68, 8, 156, 151, 142);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 10, 19, 156, 151, 142);
			break;
		case WorldTiles::Tree:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			break;
		case WorldTiles::Forest:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8, 183, 219, 162);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			break;
		case WorldTiles::Rocks:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 199, 16);
			break;
		case WorldTiles::Bush:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 178, 16);
			break;
		case WorldTiles::Shrub:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 177, 16);
			break;
		case WorldTiles::CaveEntrance:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8, 190, 190, 170);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1, 190, 190, 170);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 209, 16);
			break;
		case WorldTiles::TreeStump:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAutotile(visual.autotile, OUTSIDE_AUTOTILES, 0, 1);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 180, 16);
			break;
		default:
			return false;
		}
		return true;
	}

	bool resolveAtlas(AssetManager* assets, const AtlasTile& atlas,
		SDL_Texture*& texture, SDL_Rect& source)
	{
		if (assets == NULL || atlas.path == NULL) return false;
		int width = 0;
		int height = 0;
		texture = assets->texture(atlas.path, true);
		return texture != NULL &&
			SDL_QueryTexture(texture, NULL, NULL, &width, &height) == 0 &&
			WorldTileRenderer::atlasSourceRect(atlas.index, atlas.columns, 32,
				width, height, source);
	}

	bool renderAtlas(SDL_Renderer* renderer, SDL_Texture* texture,
		const SDL_Rect& source, const SDL_Rect& destination, Uint8 red = 255,
		Uint8 green = 255, Uint8 blue = 255)
	{
		if (renderer == NULL || texture == NULL) return false;
		SDL_SetTextureColorMod(texture, red, green, blue);
		bool rendered = SDL_RenderCopy(renderer, texture, &source, &destination) == 0;
		SDL_SetTextureColorMod(texture, 255, 255, 255);
		return rendered;
	}

	bool isTallTree(WorldTileId tile)
	{
		return tile == WorldTiles::Tree || tile == WorldTiles::Forest;
	}

	bool drawTreeLayer(SDL_Renderer* renderer, AssetManager* assets, WorldTileId tile,
		const SDL_Rect& destination, bool foreground)
	{
		if (!isTallTree(tile) || renderer == NULL || assets == NULL) return false;
		SDL_Texture* texture = assets->texture(OUTSIDE_DECOR, true);
		int width = 0;
		int height = 0;
		if (texture == NULL || SDL_QueryTexture(texture, NULL, NULL, &width, &height) != 0)
			return false;
		int sourceX = tile == WorldTiles::Tree ? 0 : 64;
		SDL_Rect source = { sourceX, foreground ? 448 : 480, 64, 32 };
		if (source.x + source.w > width || source.y + source.h > height) return false;
		SDL_Rect target = { destination.x - destination.w / 2,
			foreground ? destination.y - destination.h : destination.y,
			destination.w * 2, destination.h };
		return renderAtlas(renderer, texture, source, target,
			tile == WorldTiles::Forest ? 220 : 255,
			tile == WorldTiles::Forest ? 235 : 255,
			tile == WorldTiles::Forest ? 215 : 255);
	}
}

WorldTileRenderer::WorldTileRenderer(SDL_Renderer* renderer, AssetManager* assets)
	: mRenderer(renderer), mAssets(assets)
{
}

bool WorldTileRenderer::drawTerrain(WorldTileId tile,
	const std::vector<std::string>& map, int tileX, int tileY,
	const SDL_Rect& destination)
{
	if (drawAutotile(tile, connectionsFor(tile, map, tileX, tileY), destination))
		return true;
	return drawTerrain(tile, destination);
}

bool WorldTileRenderer::drawTerrain(WorldTileId tile, const SDL_Rect& destination)
{
	if (drawAutotile(tile, North | East | South | West | NorthWest |
		NorthEast | SouthEast | SouthWest, destination)) return true;
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.terrain.path == NULL) return false;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	if (!resolveAtlas(mAssets, visual.terrain, texture, source)) return false;
	return renderAtlas(mRenderer, texture, source, destination,
		visual.terrain.red, visual.terrain.green, visual.terrain.blue);
}

bool WorldTileRenderer::drawDecoration(WorldTileId tile, const SDL_Rect& destination)
{
	if (isTallTree(tile)) return drawTreeLayer(mRenderer, mAssets, tile, destination, false);
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.decoration.path == NULL) return false;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	if (!resolveAtlas(mAssets, visual.decoration, texture, source)) return false;
	return renderAtlas(mRenderer, texture, source, destination,
		visual.decoration.red, visual.decoration.green, visual.decoration.blue);
}

bool WorldTileRenderer::drawForeground(WorldTileId tile, const SDL_Rect& destination)
{
	return drawTreeLayer(mRenderer, mAssets, tile, destination, true);
}

bool WorldTileRenderer::drawPreview(WorldTileId tile, const SDL_Rect& destination)
{
	if (hasDecoration(tile) && !canDrawDecoration(tile)) return false;
	bool terrainDrawn = drawTerrain(tile, destination);
	if (!hasDecoration(tile)) return terrainDrawn;
	if (!isTallTree(tile)) return drawDecoration(tile, destination);
	SDL_Texture* texture = mAssets == NULL ? NULL : mAssets->texture(OUTSIDE_DECOR, true);
	int width = 0;
	int height = 0;
	if (texture == NULL || SDL_QueryTexture(texture, NULL, NULL, &width, &height) != 0)
		return false;
	SDL_Rect source = { tile == WorldTiles::Tree ? 0 : 64, 448, 64, 64 };
	if (source.x + source.w > width || source.y + source.h > height) return false;
	return renderAtlas(mRenderer, texture, source, destination,
		tile == WorldTiles::Forest ? 220 : 255,
		tile == WorldTiles::Forest ? 235 : 255,
		tile == WorldTiles::Forest ? 215 : 255);
}

bool WorldTileRenderer::canDrawDecoration(WorldTileId tile) const
{
	if (!hasDecoration(tile) || mAssets == NULL) return false;
	if (isTallTree(tile))
	{
		SDL_Texture* texture = mAssets->texture(OUTSIDE_DECOR, true);
		int width = 0;
		int height = 0;
		return texture != NULL && SDL_QueryTexture(texture, NULL, NULL, &width, &height) == 0 &&
			width >= (tile == WorldTiles::Tree ? 64 : 128) && height >= 512;
	}
	TileVisual visual;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	return tileVisual(tile, visual) &&
		resolveAtlas(mAssets, visual.decoration, texture, source);
}

bool WorldTileRenderer::hasDecoration(WorldTileId tile)
{
	if (isTallTree(tile)) return true;
	TileVisual visual;
	return tileVisual(tile, visual) && visual.decoration.path != NULL;
}

bool WorldTileRenderer::hasForeground(WorldTileId tile)
{
	return isTallTree(tile);
}

unsigned int WorldTileRenderer::connectionsFor(WorldTileId tile,
	const std::vector<std::string>& map, int tileX, int tileY) const
{
	TileVisual center;
	if (!tileVisual(tile, center) || center.autotile.path == NULL) return 0;
	auto connected = [&map, &center](int x, int y)
	{
		if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size())
			return false;
		TileVisual neighbor;
		return tileVisual(WorldTiles::fromGlyph(map[y][x]), neighbor) &&
			neighbor.autotile.path != NULL && neighbor.autotile.family == center.autotile.family;
	};
	unsigned int result = 0;
	if (connected(tileX, tileY - 1)) result |= North;
	if (connected(tileX + 1, tileY)) result |= East;
	if (connected(tileX, tileY + 1)) result |= South;
	if (connected(tileX - 1, tileY)) result |= West;
	if (connected(tileX - 1, tileY - 1)) result |= NorthWest;
	if (connected(tileX + 1, tileY - 1)) result |= NorthEast;
	if (connected(tileX + 1, tileY + 1)) result |= SouthEast;
	if (connected(tileX - 1, tileY + 1)) result |= SouthWest;
	return result;
}

bool WorldTileRenderer::drawAutotile(WorldTileId tile, unsigned int connections,
	const SDL_Rect& destination)
{
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.autotile.path == NULL ||
		mRenderer == NULL || mAssets == NULL) return false;
	SDL_Texture* texture = mAssets->texture(visual.autotile.path, true);
	int width = 0;
	int height = 0;
	if (texture == NULL || SDL_QueryTexture(texture, NULL, NULL, &width, &height) != 0)
		return false;
	const int blockX = visual.autotile.animatedWater ?
		(int)((SDL_GetTicks() / 420) % 3) * 64 : (visual.autotile.group % 8) * 64;
	const int blockY = visual.autotile.animatedWater ? 0 :
		(visual.autotile.group / 8) * 96;
	if (blockX + 64 > width || blockY + 96 > height) return false;
	const int leftWidth = destination.w / 2;
	const int topHeight = destination.h / 2;
	SDL_Rect targets[4] = {
		{ destination.x, destination.y, leftWidth, topHeight },
		{ destination.x + leftWidth, destination.y,
			destination.w - leftWidth, topHeight },
		{ destination.x, destination.y + topHeight,
			leftWidth, destination.h - topHeight },
		{ destination.x + leftWidth, destination.y + topHeight,
			destination.w - leftWidth, destination.h - topHeight }
	};
	SDL_SetTextureColorMod(texture, visual.autotile.red,
		visual.autotile.green, visual.autotile.blue);
	bool rendered = true;
	for (int quadrant = 0; quadrant < 4; ++quadrant)
	{
		SDL_Point quarter;
		if (!autotileQuarterSource(quadrant, connections, quarter))
		{
			rendered = false;
			break;
		}
		SDL_Rect source = { blockX + quarter.x * 16, blockY + quarter.y * 16, 16, 16 };
		if (SDL_RenderCopy(mRenderer, texture, &source, &targets[quadrant]) != 0)
			rendered = false;
	}
	SDL_SetTextureColorMod(texture, 255, 255, 255);
	return rendered;
}

bool WorldTileRenderer::atlasSourceRect(int tileIndex, int columns, int tileSize,
	int textureWidth, int textureHeight, SDL_Rect& source)
{
	if (tileIndex < 0 || columns <= 0 || tileSize <= 0 || textureWidth <= 0 ||
		textureHeight <= 0 || textureWidth / tileSize < columns)
		return false;
	int rows = textureHeight / tileSize;
	if (tileIndex >= columns * rows) return false;
	source = { (tileIndex % columns) * tileSize,
		(tileIndex / columns) * tileSize, tileSize, tileSize };
	return source.x + source.w <= textureWidth && source.y + source.h <= textureHeight;
}

bool WorldTileRenderer::autotileQuarterSource(int quadrant, unsigned int connections,
	SDL_Point& sourceQuarter)
{
	bool north = (connections & North) != 0;
	bool east = (connections & East) != 0;
	bool south = (connections & South) != 0;
	bool west = (connections & West) != 0;
	if (quadrant == 0)
	{
		if (north && west)
			sourceQuarter = (connections & NorthWest) != 0 ? SDL_Point{ 2, 4 } : SDL_Point{ 0, 0 };
		else if (north) sourceQuarter = { 0, 4 };
		else if (west) sourceQuarter = { 2, 2 };
		else sourceQuarter = { 0, 2 };
	}
	else if (quadrant == 1)
	{
		if (north && east)
			sourceQuarter = (connections & NorthEast) != 0 ? SDL_Point{ 1, 4 } : SDL_Point{ 1, 0 };
		else if (north) sourceQuarter = { 3, 4 };
		else if (east) sourceQuarter = { 1, 2 };
		else sourceQuarter = { 3, 2 };
	}
	else if (quadrant == 2)
	{
		if (south && west)
			sourceQuarter = (connections & SouthWest) != 0 ? SDL_Point{ 2, 3 } : SDL_Point{ 0, 1 };
		else if (south) sourceQuarter = { 0, 3 };
		else if (west) sourceQuarter = { 2, 5 };
		else sourceQuarter = { 0, 5 };
	}
	else if (quadrant == 3)
	{
		if (south && east)
			sourceQuarter = (connections & SouthEast) != 0 ? SDL_Point{ 1, 3 } : SDL_Point{ 1, 1 };
		else if (south) sourceQuarter = { 3, 3 };
		else if (east) sourceQuarter = { 1, 5 };
		else sourceQuarter = { 3, 5 };
	}
	else return false;
	return true;
}
