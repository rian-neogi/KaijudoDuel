#pragma once

#include <SDL.h>

#include <string>
#include <vector>

class AssetManager;

enum class RtpTilesetFamily
{
	Dungeon,
	Inside,
	Outside,
	World
};

enum class RtpTileSheet
{
	A1,
	A2,
	A3,
	A4,
	A5,
	B,
	C
};

enum class RtpRenderLayer
{
	Ground,
	Decoration,
	Foreground
};

struct RtpTileReference
{
	RtpTilesetFamily family;
	RtpTileSheet sheet;
	int index;
	RtpRenderLayer layer;
	Uint8 red;
	Uint8 green;
	Uint8 blue;

	RtpTileReference(RtpTilesetFamily familyValue, RtpTileSheet sheetValue,
		int indexValue, RtpRenderLayer layerValue = RtpRenderLayer::Ground,
		Uint8 redValue = 255, Uint8 greenValue = 255, Uint8 blueValue = 255)
		: family(familyValue), sheet(sheetValue), index(indexValue), layer(layerValue),
		  red(redValue), green(greenValue), blue(blueValue)
	{
	}
};

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
	bool drawLayer(const std::vector<RtpTileReference>& tiles, RtpRenderLayer layer,
		unsigned int connections, const SDL_Rect& destination,
		unsigned int animationFrame = 0);

	bool validateAllAssets(std::string& error);
	static std::vector<RtpSheetDescriptor> availableSheets();
	static const RtpSheetDescriptor* descriptor(RtpTilesetFamily family,
		RtpTileSheet sheet);
	static bool loadTileNames(RtpTilesetFamily family, RtpTileSheet sheet,
		std::vector<std::string>& names, std::string& error);
	static RtpRenderLayer defaultLayer(RtpTileSheet sheet);
	static bool regularTileSource(RtpTileSheet sheet, int tileIndex,
		int textureWidth, int textureHeight, SDL_Rect& source);

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
	static bool autotileOrigin(const RtpTileReference& tile,
		unsigned int animationFrame, int& sourceX, int& sourceY,
		AutotileFormat& format);

	SDL_Renderer* mRenderer;
	AssetManager* mAssets;
};
