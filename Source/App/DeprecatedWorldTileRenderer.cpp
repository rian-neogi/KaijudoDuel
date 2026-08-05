#include "DeprecatedWorldTileRenderer.h"

#include "AssetManager.h"
#include "RtpTilesetRenderer.h"

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

	bool catalogTerrain(WorldTileId tile, RtpTileReference& reference)
	{
		switch (tile)
		{
		case WorldTiles::House:
		case WorldTiles::Bonfire:
		case WorldTiles::FeastTable:
		case WorldTiles::WatershedMarker:
		case WorldTiles::RootmazeMarker:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A2, 0, RtpRenderLayer::Ground);
			return true;
		case WorldTiles::WoodWall:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::A4, 16, RtpRenderLayer::Ground);
			return true;
		case WorldTiles::Door:
		case WorldTiles::Counter:
		case WorldTiles::WorkshopTools:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::A2, 0, RtpRenderLayer::Ground);
			return true;
		case WorldTiles::MarbleRoof:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A3, 4, RtpRenderLayer::Ground, 238, 238, 230);
			return true;
		case WorldTiles::Rail:
		case WorldTiles::RailCrossing:
		case WorldTiles::MetalGrate:
		case WorldTiles::Machinery:
		case WorldTiles::Furnace:
		case WorldTiles::CinderrailDoor:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::A5, 51, RtpRenderLayer::Ground, 190, 170, 145);
			return true;
		case WorldTiles::IndustrialBrick:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::A4, 43, RtpRenderLayer::Ground, 205, 150, 135);
			return true;
		case WorldTiles::TimberRoof:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A3, 16, RtpRenderLayer::Ground);
			return true;
		case WorldTiles::IndustrialRoof:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A3, 7, RtpRenderLayer::Ground, 190, 190, 195);
			return true;
		case WorldTiles::TimberBridge:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A5, 5, RtpRenderLayer::Ground);
			return true;
		case WorldTiles::RockyCliff:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A4, 40, RtpRenderLayer::Ground);
			return true;
		case WorldTiles::OldRoadWaystone:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A2, 3, RtpRenderLayer::Ground, 205, 195, 175);
			return true;
		case WorldTiles::CinderrailRubble:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A2, 8, RtpRenderLayer::Ground, 174, 151, 112);
			return true;
		case WorldTiles::GlasswaterRoof:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A3, 2, RtpRenderLayer::Ground, 145, 205, 235);
			return true;
		case WorldTiles::GlasswaterDock:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A5, 5, RtpRenderLayer::Ground, 175, 155, 135);
			return true;
		case WorldTiles::GlasswaterWall:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A4, 13, RtpRenderLayer::Ground, 175, 225, 235);
			return true;
		case WorldTiles::GlasswaterDoor:
		case WorldTiles::GlasswaterMarker:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A2, 3, RtpRenderLayer::Ground, 165, 220, 230);
			return true;
		case WorldTiles::RootmazeRoot:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A4, 37, RtpRenderLayer::Ground, 175, 220, 145);
			return true;
		case WorldTiles::RootmazeBridge:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A5, 5, RtpRenderLayer::Ground, 190, 180, 135);
			return true;
		case WorldTiles::RootmazeRoof:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A3, 19, RtpRenderLayer::Ground, 185, 225, 145);
			return true;
		case WorldTiles::RootmazeWall:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A4, 25, RtpRenderLayer::Ground, 195, 185, 135);
			return true;
		case WorldTiles::RootmazeDoor:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::A2, 0, RtpRenderLayer::Ground, 181, 226, 155);
			return true;
		case WorldTiles::BlackstoneWall:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::A4, 25, RtpRenderLayer::Ground, 115, 115, 120);
			return true;
		case WorldTiles::BlackstoneGate:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::A5, 51, RtpRenderLayer::Ground, 130, 125, 115);
			return true;
		default:
			return false;
		}
	}

	bool catalogDecoration(WorldTileId tile, RtpTileReference& reference)
	{
		switch (tile)
		{
		case WorldTiles::House:
			reference = RtpTileReference(RtpTilesetFamily::World,
				RtpTileSheet::B, 63, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::Door:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::B, 195, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::Counter:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::B, 114, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::Bonfire:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::B, 83, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::FeastTable:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::B, 96, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::OldRoadWaystone:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::B, 145, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::WorkshopTools:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::C, 148, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::Rail:
		case WorldTiles::RailCrossing:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::C, tile == WorldTiles::Rail ? 104 : 112,
				RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::MetalGrate:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::C, 113, RtpRenderLayer::Decoration, 190, 195, 195);
			return true;
		case WorldTiles::Machinery:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::C, 151, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::Furnace:
			reference = RtpTileReference(RtpTilesetFamily::Inside,
				RtpTileSheet::B, 112, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::CinderrailRubble:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::B, 104, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::CinderrailDoor:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::B, 129, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::WatershedMarker:
		case WorldTiles::GlasswaterMarker:
		case WorldTiles::RootmazeMarker:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::B, 73, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::GlasswaterDoor:
		case WorldTiles::RootmazeDoor:
			reference = RtpTileReference(RtpTilesetFamily::Outside,
				RtpTileSheet::B, 67, RtpRenderLayer::Decoration);
			return true;
		case WorldTiles::BlackstoneGate:
			reference = RtpTileReference(RtpTilesetFamily::Dungeon,
				RtpTileSheet::C, 56, RtpRenderLayer::Decoration, 205, 175, 105);
			return true;
		default:
			return false;
		}
	}

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
			DeprecatedWorldTileRenderer::atlasSourceRect(atlas.index, atlas.columns, 32,
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

DeprecatedWorldTileRenderer::DeprecatedWorldTileRenderer(SDL_Renderer* renderer, AssetManager* assets)
	: mRenderer(renderer), mAssets(assets), mRtpTiles(new RtpTilesetRenderer(renderer, assets))
{
}

DeprecatedWorldTileRenderer::~DeprecatedWorldTileRenderer()
{
	delete mRtpTiles;
}

bool DeprecatedWorldTileRenderer::drawTerrain(WorldTileId tile,
	const std::vector<std::string>& map, int tileX, int tileY,
	const SDL_Rect& destination)
{
	unsigned int connections = connectionsFor(tile, map, tileX, tileY);
	if (drawAutotile(tile, connections, destination))
		return true;
	RtpTileReference catalogTile(RtpTilesetFamily::Outside, RtpTileSheet::A5, 0);
	if (catalogTerrain(tile, catalogTile) && mRtpTiles != NULL &&
		mRtpTiles->draw(catalogTile, connections, destination)) return true;
	return drawTerrain(tile, destination);
}

bool DeprecatedWorldTileRenderer::drawTerrain(WorldTileId tile, const SDL_Rect& destination)
{
	if (drawAutotile(tile, North | East | South | West | NorthWest |
		NorthEast | SouthEast | SouthWest, destination)) return true;
	RtpTileReference catalogTile(RtpTilesetFamily::Outside, RtpTileSheet::A5, 0);
	if (catalogTerrain(tile, catalogTile) && mRtpTiles != NULL &&
		mRtpTiles->draw(catalogTile, North | East | South | West | NorthWest |
			NorthEast | SouthEast | SouthWest, destination)) return true;
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.terrain.path == NULL) return false;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	if (!resolveAtlas(mAssets, visual.terrain, texture, source)) return false;
	return renderAtlas(mRenderer, texture, source, destination,
		visual.terrain.red, visual.terrain.green, visual.terrain.blue);
}

bool DeprecatedWorldTileRenderer::drawDecoration(WorldTileId tile, const SDL_Rect& destination)
{
	if (isTallTree(tile)) return drawTreeLayer(mRenderer, mAssets, tile, destination, false);
	RtpTileReference catalogTile(RtpTilesetFamily::Outside, RtpTileSheet::B, 0,
		RtpRenderLayer::Decoration);
	if (catalogDecoration(tile, catalogTile) && mRtpTiles != NULL)
		return mRtpTiles->draw(catalogTile, 0, destination);
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.decoration.path == NULL) return false;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	if (!resolveAtlas(mAssets, visual.decoration, texture, source)) return false;
	return renderAtlas(mRenderer, texture, source, destination,
		visual.decoration.red, visual.decoration.green, visual.decoration.blue);
}

bool DeprecatedWorldTileRenderer::drawForeground(WorldTileId tile, const SDL_Rect& destination)
{
	return drawTreeLayer(mRenderer, mAssets, tile, destination, true);
}

bool DeprecatedWorldTileRenderer::drawPreview(WorldTileId tile, const SDL_Rect& destination)
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

bool DeprecatedWorldTileRenderer::drawCatalog(const RtpTileReference& tile,
	unsigned int connections, const SDL_Rect& destination,
	unsigned int animationFrame)
{
	return mRtpTiles != NULL &&
		mRtpTiles->draw(tile, connections, destination, animationFrame);
}

bool DeprecatedWorldTileRenderer::drawSignpost(const SDL_Rect& destination)
{
	if (mRtpTiles == NULL) return false;
	return mRtpTiles->draw(RtpTileReference(RtpTilesetFamily::Outside,
		RtpTileSheet::B, 73, RtpRenderLayer::Decoration), 0, destination);
}

bool DeprecatedWorldTileRenderer::drawChest(const SDL_Rect& destination, bool opened)
{
	if (mRtpTiles == NULL) return false;
	return mRtpTiles->draw(RtpTileReference(RtpTilesetFamily::Inside,
		RtpTileSheet::B, opened ? 56 : 48, RtpRenderLayer::Decoration),
		0, destination);
}

bool DeprecatedWorldTileRenderer::drawShard(const SDL_Rect& destination)
{
	if (mRtpTiles == NULL) return false;
	return mRtpTiles->draw(RtpTileReference(RtpTilesetFamily::Dungeon,
		RtpTileSheet::B, 61, RtpRenderLayer::Decoration, 225, 190, 255),
		0, destination);
}

bool DeprecatedWorldTileRenderer::canDrawDecoration(WorldTileId tile) const
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
	RtpTileReference catalogTile(RtpTilesetFamily::Outside, RtpTileSheet::B, 0,
		RtpRenderLayer::Decoration);
	if (catalogDecoration(tile, catalogTile))
	{
		const RtpSheetDescriptor* info = RtpTilesetRenderer::descriptor(
			catalogTile.family, catalogTile.sheet);
		return info != NULL && mAssets->texture(info->imagePath, true) != NULL;
	}
	TileVisual visual;
	SDL_Texture* texture = NULL;
	SDL_Rect source;
	return tileVisual(tile, visual) &&
		resolveAtlas(mAssets, visual.decoration, texture, source);
}

bool DeprecatedWorldTileRenderer::hasDecoration(WorldTileId tile)
{
	if (isTallTree(tile)) return true;
	RtpTileReference catalogTile(RtpTilesetFamily::Outside, RtpTileSheet::B, 0,
		RtpRenderLayer::Decoration);
	if (catalogDecoration(tile, catalogTile)) return true;
	TileVisual visual;
	return tileVisual(tile, visual) && visual.decoration.path != NULL;
}

bool DeprecatedWorldTileRenderer::hasForeground(WorldTileId tile)
{
	return isTallTree(tile);
}

unsigned int DeprecatedWorldTileRenderer::connectionsFor(WorldTileId tile,
	const std::vector<std::string>& map, int tileX, int tileY) const
{
	TileVisual center;
	bool hasAutotile = tileVisual(tile, center) && center.autotile.path != NULL;
	RtpTileReference catalogTile(RtpTilesetFamily::Outside, RtpTileSheet::A5, 0);
	bool hasCatalogTile = catalogTerrain(tile, catalogTile) &&
		(catalogTile.sheet == RtpTileSheet::A1 || catalogTile.sheet == RtpTileSheet::A2 ||
			catalogTile.sheet == RtpTileSheet::A3 || catalogTile.sheet == RtpTileSheet::A4);
	if (!hasAutotile && !hasCatalogTile) return 0;
	auto connected = [&map, &center](int x, int y)
	{
		if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size())
			return false;
		TileVisual neighbor;
		return tileVisual(WorldTiles::fromGlyph(map[y][x]), neighbor) &&
			neighbor.autotile.path != NULL && neighbor.autotile.family == center.autotile.family;
	};
	if (!hasAutotile)
	{
		auto sameTile = [&map, tile](int x, int y)
		{
			return y >= 0 && y < (int)map.size() && x >= 0 && x < (int)map[y].size() &&
				WorldTiles::fromGlyph(map[y][x]) == tile;
		};
		unsigned int result = 0;
		if (sameTile(tileX, tileY - 1)) result |= North;
		if (sameTile(tileX + 1, tileY)) result |= East;
		if (sameTile(tileX, tileY + 1)) result |= South;
		if (sameTile(tileX - 1, tileY)) result |= West;
		if (sameTile(tileX - 1, tileY - 1)) result |= NorthWest;
		if (sameTile(tileX + 1, tileY - 1)) result |= NorthEast;
		if (sameTile(tileX + 1, tileY + 1)) result |= SouthEast;
		if (sameTile(tileX - 1, tileY + 1)) result |= SouthWest;
		return result;
	}
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

bool DeprecatedWorldTileRenderer::drawAutotile(WorldTileId tile, unsigned int connections,
	const SDL_Rect& destination)
{
	TileVisual visual;
	if (!tileVisual(tile, visual) || visual.autotile.path == NULL ||
		mRtpTiles == NULL) return false;
	RtpTilesetFamily family = visual.autotile.path == INSIDE_AUTOTILES ?
		RtpTilesetFamily::Inside : RtpTilesetFamily::Outside;
	RtpTileSheet sheet = visual.autotile.animatedWater ?
		RtpTileSheet::A1 : RtpTileSheet::A2;
	RtpTileReference reference(family, sheet, visual.autotile.group,
		RtpRenderLayer::Ground, visual.autotile.red, visual.autotile.green,
		visual.autotile.blue);
	return mRtpTiles->draw(reference, connections, destination,
		visual.autotile.animatedWater ? SDL_GetTicks() / 420 : 0);
}

bool DeprecatedWorldTileRenderer::atlasSourceRect(int tileIndex, int columns, int tileSize,
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

bool DeprecatedWorldTileRenderer::autotileQuarterSource(int quadrant, unsigned int connections,
	SDL_Point& sourceQuarter)
{
	return RtpTilesetRenderer::floorQuarterSource(quadrant, connections, sourceQuarter);
}
