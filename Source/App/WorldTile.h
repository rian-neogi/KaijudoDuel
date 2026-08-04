#pragma once

// Stable one-byte tile IDs. Lua uses the byte values as compact glyphs, while
// gameplay and rendering use these semantic names instead of map-dependent
// interpretations of the same glyph.
using WorldTileId = unsigned char;

namespace WorldTiles
{
	constexpr WorldTileId Grass = '.';
	constexpr WorldTileId Path = '=';
	constexpr WorldTileId Water = '~';
	constexpr WorldTileId House = 'H';
	constexpr WorldTileId Tree = 'T';
	constexpr WorldTileId Forest = '#';
	constexpr WorldTileId WoodWall = 'W';
	constexpr WorldTileId Door = 'D';
	constexpr WorldTileId WoodFloor = 'F';
	constexpr WorldTileId Counter = 'C';
	constexpr WorldTileId Bonfire = 'B';
	constexpr WorldTileId FeastTable = 'A';
	constexpr WorldTileId DuelSand = 'S';
	constexpr WorldTileId Marble = 'M';
	constexpr WorldTileId MarbleRoof = 'Q';
	constexpr WorldTileId WorkshopTools = 'E';
	constexpr WorldTileId Rail = 'R';
	constexpr WorldTileId RailCrossing = 'X';
	constexpr WorldTileId MetalGrate = 'G';
	constexpr WorldTileId IndustrialBrick = 'I';
	constexpr WorldTileId Machinery = 'P';
	constexpr WorldTileId Furnace = 'V';
	constexpr WorldTileId TimberRoof = 'K';
	constexpr WorldTileId IndustrialRoof = 'J';
	constexpr WorldTileId TimberBridge = 'U';
	constexpr WorldTileId RockyCliff = 'O';
	constexpr WorldTileId OldRoadPath = '1';
	constexpr WorldTileId OldRoadWaystone = '2';
	constexpr WorldTileId CinderrailGround = '3';
	constexpr WorldTileId CinderrailPath = '4';
	constexpr WorldTileId CinderrailRubble = '5';
	constexpr WorldTileId CinderrailDuelSand = '6';
	constexpr WorldTileId CinderrailDoor = '7';
	constexpr WorldTileId WatershedGround = '8';
	constexpr WorldTileId WatershedPath = '9';
	constexpr WorldTileId WatershedMarker = '0';
	constexpr WorldTileId GlasswaterGround = 'a';
	constexpr WorldTileId GlasswaterPaving = 'b';
	constexpr WorldTileId GlasswaterRoof = 'c';
	constexpr WorldTileId GlasswaterDock = 'd';
	constexpr WorldTileId GlasswaterWall = 'e';
	constexpr WorldTileId GlasswaterDoor = 'f';
	constexpr WorldTileId GlasswaterArena = 'g';
	constexpr WorldTileId GlasswaterMarker = 'h';
	constexpr WorldTileId RootmazeGround = 'i';
	constexpr WorldTileId RootmazePath = 'j';
	constexpr WorldTileId RootmazeRoot = 'k';
	constexpr WorldTileId RootmazeBridge = 'l';
	constexpr WorldTileId RootmazeRoof = 'm';
	constexpr WorldTileId RootmazeWall = 'n';
	constexpr WorldTileId RootmazeDoor = 'o';
	constexpr WorldTileId RootmazeArena = 'p';
	constexpr WorldTileId RootmazeMarker = 'q';

	inline WorldTileId fromGlyph(char glyph)
	{
		return static_cast<WorldTileId>(static_cast<unsigned char>(glyph));
	}

	inline char glyph(WorldTileId tile)
	{
		return static_cast<char>(tile);
	}

	inline bool isValid(WorldTileId tile)
	{
		switch (tile)
		{
		case Grass: case Path: case Water: case House: case Tree: case Forest:
		case WoodWall: case Door: case WoodFloor: case Counter: case Bonfire:
		case FeastTable: case DuelSand: case Marble: case MarbleRoof:
		case WorkshopTools: case Rail: case RailCrossing: case MetalGrate:
		case IndustrialBrick: case Machinery: case Furnace: case TimberRoof:
		case IndustrialRoof: case TimberBridge: case RockyCliff:
		case OldRoadPath: case OldRoadWaystone: case CinderrailGround:
		case CinderrailPath: case CinderrailRubble: case CinderrailDuelSand:
		case CinderrailDoor: case WatershedGround: case WatershedPath:
		case WatershedMarker: case GlasswaterGround: case GlasswaterPaving:
		case GlasswaterRoof: case GlasswaterDock: case GlasswaterWall:
		case GlasswaterDoor: case GlasswaterArena: case GlasswaterMarker:
		case RootmazeGround: case RootmazePath: case RootmazeRoot:
		case RootmazeBridge: case RootmazeRoof: case RootmazeWall:
		case RootmazeDoor: case RootmazeArena: case RootmazeMarker:
			return true;
		default:
			return false;
		}
	}

	inline bool isWalkable(WorldTileId tile)
	{
		switch (tile)
		{
		case Grass: case Path: case Door: case WoodFloor: case DuelSand:
		case RailCrossing: case MetalGrate: case TimberBridge: case OldRoadPath:
		case CinderrailGround: case CinderrailPath: case CinderrailDuelSand:
		case CinderrailDoor: case WatershedGround: case WatershedPath:
		case GlasswaterGround: case GlasswaterPaving: case GlasswaterDock:
		case GlasswaterDoor: case GlasswaterArena:
		case RootmazeGround: case RootmazePath: case RootmazeBridge:
		case RootmazeDoor: case RootmazeArena:
			return true;
		default:
			return false;
		}
	}
}
