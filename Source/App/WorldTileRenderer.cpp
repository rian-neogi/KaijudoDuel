#include "WorldTileRenderer.h"

#include "AssetManager.h"

namespace
{
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

	struct TileVisual
	{
		AtlasTile terrain;
		AtlasTile decoration;
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

	bool tileVisual(WorldTileId tile, TileVisual& visual)
	{
		switch (tile)
		{
		case WorldTiles::Grass:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			break;
		case WorldTiles::Path:
		case WorldTiles::DuelSand:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 17, 8);
			break;
		case WorldTiles::OldRoadPath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 32, 8, 204, 191, 164);
			break;
		case WorldTiles::Marble:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 50, 8, 225, 235, 238);
			break;
		case WorldTiles::WoodFloor:
			setAtlas(visual.terrain, INSIDE_TERRAIN, 16, 8);
			break;
		case WorldTiles::CinderrailGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 49, 8, 174, 151, 112);
			break;
		case WorldTiles::CinderrailPath:
		case WorldTiles::CinderrailDuelSand:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 17, 8, 191, 146, 100);
			break;
		case WorldTiles::WatershedGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 19, 8, 177, 232, 182);
			break;
		case WorldTiles::WatershedPath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 20, 8, 211, 207, 167);
			break;
		case WorldTiles::GlasswaterGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 48, 8, 160, 220, 230);
			break;
		case WorldTiles::GlasswaterPaving:
		case WorldTiles::GlasswaterArena:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 50, 8, 178, 226, 239);
			break;
		case WorldTiles::RootmazeGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8, 181, 226, 155);
			break;
		case WorldTiles::RootmazePath:
		case WorldTiles::RootmazeArena:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 18, 8, 185, 196, 125);
			break;
		case WorldTiles::BlackstoneGround:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 72, 8, 130, 130, 128);
			break;
		case WorldTiles::BlackstonePath:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 68, 8, 156, 151, 142);
			break;
		case WorldTiles::Tree:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 181, 16);
			break;
		case WorldTiles::Forest:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8, 183, 219, 162);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 197, 16, 210, 235, 205);
			break;
		case WorldTiles::Rocks:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 199, 16);
			break;
		case WorldTiles::Bush:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 178, 16);
			break;
		case WorldTiles::Shrub:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 177, 16);
			break;
		case WorldTiles::CaveEntrance:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8, 190, 190, 170);
			setAtlas(visual.decoration, OUTSIDE_DECOR, 209, 16);
			break;
		case WorldTiles::TreeStump:
			setAtlas(visual.terrain, OUTSIDE_TERRAIN, 16, 8);
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
		const SDL_Rect& source, const SDL_Rect& destination, const AtlasTile& atlas)
	{
		if (renderer == NULL || texture == NULL) return false;
		SDL_SetTextureColorMod(texture, atlas.red, atlas.green, atlas.blue);
		bool rendered = SDL_RenderCopy(renderer, texture, &source, &destination) == 0;
		SDL_SetTextureColorMod(texture, 255, 255, 255);
		return rendered;
	}
}

WorldTileRenderer::WorldTileRenderer(SDL_Renderer* renderer, AssetManager* assets)
	: mRenderer(renderer), mAssets(assets)
{
}

bool WorldTileRenderer::drawTerrain(WorldTileId tile, const SDL_Rect& destination)
{
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.terrain.path == NULL) return false;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	if (!resolveAtlas(mAssets, visual.terrain, texture, source)) return false;
	return renderAtlas(mRenderer, texture, source, destination, visual.terrain);
}

bool WorldTileRenderer::drawDecorationTile(WorldTileId tile,
	const SDL_Rect& destination)
{
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.terrain.path == NULL ||
		visual.decoration.path == NULL) return false;
	SDL_Texture* terrainTexture = NULL;
	SDL_Texture* decorationTexture = NULL;
	SDL_Rect terrainSource;
	SDL_Rect decorationSource;
	if (!resolveAtlas(mAssets, visual.terrain, terrainTexture, terrainSource) ||
		!resolveAtlas(mAssets, visual.decoration, decorationTexture, decorationSource))
		return false;
	if (!renderAtlas(mRenderer, terrainTexture, terrainSource, destination, visual.terrain))
		return false;
	return renderAtlas(mRenderer, decorationTexture, decorationSource, destination,
		visual.decoration);
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
