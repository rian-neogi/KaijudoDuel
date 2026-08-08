#include "WorldTileRenderer.h"

#include "DeprecatedWorldTileRenderer.h"

WorldTileRenderer::WorldTileRenderer(SDL_Renderer* renderer, AssetManager* assets)
	: mCatalog(new RtpTilesetRenderer(renderer, assets)),
	  mDeprecated(new DeprecatedWorldTileRenderer(renderer, assets))
{
}

WorldTileRenderer::~WorldTileRenderer()
{
	delete mCatalog;
	delete mDeprecated;
}

bool WorldTileRenderer::drawTerrain(WorldTileId tile,
	const std::vector<std::string>& map, int tileX, int tileY,
	const SDL_Rect& destination)
{
	return mDeprecated->drawTerrain(tile, map, tileX, tileY, destination);
}

bool WorldTileRenderer::drawTerrain(WorldTileId tile, const SDL_Rect& destination)
{
	return mDeprecated->drawTerrain(tile, destination);
}

bool WorldTileRenderer::drawDecoration(WorldTileId tile, const SDL_Rect& destination)
{
	return mDeprecated->drawDecoration(tile, destination);
}

bool WorldTileRenderer::drawForeground(WorldTileId tile, const SDL_Rect& destination)
{
	return mDeprecated->drawForeground(tile, destination);
}

bool WorldTileRenderer::drawPreview(WorldTileId tile, const SDL_Rect& destination)
{
	return mDeprecated->drawPreview(tile, destination);
}

bool WorldTileRenderer::canDrawDecoration(WorldTileId tile) const
{
	return mDeprecated->canDrawDecoration(tile);
}

bool WorldTileRenderer::hasDecoration(WorldTileId tile)
{
	return DeprecatedWorldTileRenderer::hasDecoration(tile);
}

bool WorldTileRenderer::hasForeground(WorldTileId tile)
{
	return DeprecatedWorldTileRenderer::hasForeground(tile);
}

bool WorldTileRenderer::atlasSourceRect(int tileIndex, int columns, int tileSize,
	int textureWidth, int textureHeight, SDL_Rect& source)
{
	return DeprecatedWorldTileRenderer::atlasSourceRect(tileIndex, columns,
		tileSize, textureWidth, textureHeight, source);
}

bool WorldTileRenderer::autotileQuarterSource(int quadrant,
	unsigned int connections, SDL_Point& sourceQuarter)
{
	return DeprecatedWorldTileRenderer::autotileQuarterSource(
		quadrant, connections, sourceQuarter);
}

bool WorldTileRenderer::drawCatalog(const RtpTileReference& tile,
	unsigned int connections, const SDL_Rect& destination,
	unsigned int animationFrame)
{
	return mCatalog != NULL &&
		mCatalog->draw(tile, connections, destination, animationFrame);
}

bool WorldTileRenderer::drawCatalogTreeLayer(const RtpTileReference& tile,
	RtpRenderLayer layer, const SDL_Rect& destination)
{
	return mCatalog != NULL && mCatalog->drawTreeLayer(tile, layer, destination);
}

bool WorldTileRenderer::drawSignpost(const SDL_Rect& destination)
{
	return drawCatalog(RtpTileReference(RtpTilesetFamily::Outside,
		RtpTileSheet::B, 73, RtpRenderLayer::Decoration), 0, destination);
}

bool WorldTileRenderer::drawChest(const SDL_Rect& destination, bool opened)
{
	return drawCatalog(RtpTileReference(RtpTilesetFamily::Inside,
		RtpTileSheet::B, opened ? 56 : 48, RtpRenderLayer::Decoration),
		0, destination);
}

bool WorldTileRenderer::drawShard(const SDL_Rect& destination)
{
	return drawCatalog(RtpTileReference(RtpTilesetFamily::Dungeon,
		RtpTileSheet::B, 61, RtpRenderLayer::Decoration, 225, 190, 255),
		0, destination);
}
