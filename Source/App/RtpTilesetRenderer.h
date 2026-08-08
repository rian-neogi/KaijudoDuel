#pragma once

#include "RtpTile.h"

#include <SDL.h>

#include <string>
#include <vector>

class AssetManager;

struct RtpSheetDescriptor
{
	RtpTilesetFamily family;
	RtpTileSheet sheet;
	const char* imagePath;
	const char* metadataPath;
	int width;
	int height;
	int tileCount;
};

class RtpTilesetRenderer
{
public:
	RtpTilesetRenderer(SDL_Renderer* renderer, AssetManager* assets);

	bool draw(const RtpTileReference& tile, unsigned int connections,
		const SDL_Rect& destination, unsigned int animationFrame = 0);
	bool drawTreeLayer(const RtpTileReference& tile, RtpRenderLayer layer,
		const SDL_Rect& destination);
	bool drawLayer(const std::vector<RtpTileReference>& tiles, RtpRenderLayer layer,
		unsigned int connections, const SDL_Rect& destination,
		unsigned int animationFrame = 0);

	bool validateAllAssets(std::string& error);
	static std::vector<RtpSheetDescriptor> availableSheets();
	static const RtpSheetDescriptor* descriptor(RtpTilesetFamily family,
		RtpTileSheet sheet);
	static bool loadTileNames(RtpTilesetFamily family, RtpTileSheet sheet,
		std::vector<std::string>& names, std::string& error);
	static RtpTileCollision collision(const RtpTileReference& tile);
	static RtpRenderLayer inferredLayer(const RtpTileReference& tile);
	static int canonicalTileIndex(RtpTilesetFamily family, RtpTileSheet sheet,
		int tileIndex);
	static bool isTreeAutotile(const RtpTileReference& tile);
	static bool treeAutotileFootprint(const RtpTileReference& tile,
		int& width, int& height);
	static bool largeTreeAnchorsConflict(const RtpTileReference& first,
		int firstX, int firstY, const RtpTileReference& second,
		int secondX, int secondY);
	static const char* treeAutotileName(RtpTilesetFamily family,
		RtpTileSheet sheet, int tileIndex);
	static bool regularTileSource(RtpTileSheet sheet, int tileIndex,
		int textureWidth, int textureHeight, SDL_Rect& source);
	static bool paletteTileSource(RtpTileSheet sheet, int tileIndex,
		SDL_Rect& source);

	static bool floorQuarterSource(int quadrant, unsigned int connections,
		SDL_Point& sourceQuarter);
	static bool wallQuarterSource(int quadrant, unsigned int connections,
		SDL_Point& sourceQuarter);
	static bool waterfallQuarterSource(int quadrant, unsigned int connections,
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
	enum class AutotileFormat
	{
		None,
		Floor,
		Wall,
		Waterfall
	};

	bool drawRegular(const RtpTileReference& tile, const RtpSheetDescriptor& sheet,
		const SDL_Rect& destination);
	bool drawAutotile(const RtpTileReference& tile, const RtpSheetDescriptor& sheet,
		unsigned int connections, const SDL_Rect& destination,
		unsigned int animationFrame);
	bool drawTreeAutotile(const RtpTileReference& tile,
		const SDL_Rect& destination, bool drawCanopy = true, bool drawBase = true);
	static bool autotileOrigin(const RtpTileReference& tile,
		unsigned int animationFrame, int& sourceX, int& sourceY,
		AutotileFormat& format);
	static bool metadataName(const RtpTileReference& tile, std::string& name);

	SDL_Renderer* mRenderer;
	AssetManager* mAssets;
};
