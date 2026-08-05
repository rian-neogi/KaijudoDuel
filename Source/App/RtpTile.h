#pragma once

#include <cstdint>

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

enum class RtpTileCollision
{
	Ignore,
	Walkable,
	Blocked
};

struct RtpTileReference
{
	RtpTilesetFamily family;
	RtpTileSheet sheet;
	int index;
	RtpRenderLayer layer;
	std::uint8_t red;
	std::uint8_t green;
	std::uint8_t blue;

	RtpTileReference(RtpTilesetFamily familyValue, RtpTileSheet sheetValue,
		int indexValue, RtpRenderLayer layerValue = RtpRenderLayer::Ground,
		std::uint8_t redValue = 255, std::uint8_t greenValue = 255,
		std::uint8_t blueValue = 255)
		: family(familyValue), sheet(sheetValue), index(indexValue), layer(layerValue),
		  red(redValue), green(greenValue), blue(blueValue)
	{
	}
};

