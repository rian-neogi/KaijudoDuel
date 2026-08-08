#include "RtpTilesetRenderer.h"

#include "AssetManager.h"

#include <fstream>
#include <map>

namespace
{
	const RtpSheetDescriptor SHEETS[] = {
		{ RtpTilesetFamily::Dungeon, RtpTileSheet::A1,
			"Resources/Graphics/Tilesets/Dungeon_A1.png",
			"Resources/Graphics/Tilesets/Dungeon_A1.txt", 512, 384, 16 },
		{ RtpTilesetFamily::Dungeon, RtpTileSheet::A2,
			"Resources/Graphics/Tilesets/Dungeon_A2.png",
			"Resources/Graphics/Tilesets/Dungeon_A2.txt", 512, 384, 32 },
		{ RtpTilesetFamily::Dungeon, RtpTileSheet::A4,
			"Resources/Graphics/Tilesets/Dungeon_A4.png",
			"Resources/Graphics/Tilesets/Dungeon_A4.txt", 512, 480, 48 },
		{ RtpTilesetFamily::Dungeon, RtpTileSheet::A5,
			"Resources/Graphics/Tilesets/Dungeon_A5.png",
			"Resources/Graphics/Tilesets/Dungeon_A5.txt", 256, 512, 128 },
		{ RtpTilesetFamily::Dungeon, RtpTileSheet::B,
			"Resources/Graphics/Tilesets/Dungeon_B.png",
			"Resources/Graphics/Tilesets/Dungeon_B.txt", 512, 512, 256 },
		{ RtpTilesetFamily::Dungeon, RtpTileSheet::C,
			"Resources/Graphics/Tilesets/Dungeon_C.png",
			"Resources/Graphics/Tilesets/Dungeon_C.txt", 512, 512, 256 },
		{ RtpTilesetFamily::Inside, RtpTileSheet::A1,
			"Resources/Graphics/Tilesets/Inside_A1.png",
			"Resources/Graphics/Tilesets/Inside_A1.txt", 512, 384, 16 },
		{ RtpTilesetFamily::Inside, RtpTileSheet::A2,
			"Resources/Graphics/Tilesets/Inside_A2.png",
			"Resources/Graphics/Tilesets/Inside_A2.txt", 512, 384, 32 },
		{ RtpTilesetFamily::Inside, RtpTileSheet::A4,
			"Resources/Graphics/Tilesets/Inside_A4.png",
			"Resources/Graphics/Tilesets/Inside_A4.txt", 512, 480, 48 },
		{ RtpTilesetFamily::Inside, RtpTileSheet::A5,
			"Resources/Graphics/Tilesets/Inside_A5.png",
			"Resources/Graphics/Tilesets/Inside_A5.txt", 256, 512, 128 },
		{ RtpTilesetFamily::Inside, RtpTileSheet::B,
			"Resources/Graphics/Tilesets/Inside_B.png",
			"Resources/Graphics/Tilesets/Inside_B.txt", 512, 512, 256 },
		{ RtpTilesetFamily::Inside, RtpTileSheet::C,
			"Resources/Graphics/Tilesets/Inside_C.png",
			"Resources/Graphics/Tilesets/Inside_C.txt", 512, 512, 256 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::A1,
			"Resources/Graphics/Tilesets/Outside_A1.png",
			"Resources/Graphics/Tilesets/Outside_A1.txt", 512, 384, 16 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::A2,
			"Resources/Graphics/Tilesets/Outside_A2.png",
			"Resources/Graphics/Tilesets/Outside_A2.txt", 512, 384, 32 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::A3,
			"Resources/Graphics/Tilesets/Outside_A3.png",
			"Resources/Graphics/Tilesets/Outside_A3.txt", 512, 256, 32 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::A4,
			"Resources/Graphics/Tilesets/Outside_A4.png",
			"Resources/Graphics/Tilesets/Outside_A4.txt", 512, 480, 48 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::A5,
			"Resources/Graphics/Tilesets/Outside_A5.png",
			"Resources/Graphics/Tilesets/Outside_A5.txt", 256, 512, 128 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::B,
			"Resources/Graphics/Tilesets/Outside_B.png",
			"Resources/Graphics/Tilesets/Outside_B.txt", 512, 512, 256 },
		{ RtpTilesetFamily::Outside, RtpTileSheet::C,
			"Resources/Graphics/Tilesets/Outside_C.png",
			"Resources/Graphics/Tilesets/Outside_C.txt", 512, 512, 256 },
		{ RtpTilesetFamily::World, RtpTileSheet::A1,
			"Resources/Graphics/Tilesets/World_A1.png",
			"Resources/Graphics/Tilesets/World_A1.txt", 512, 384, 16 },
		{ RtpTilesetFamily::World, RtpTileSheet::A2,
			"Resources/Graphics/Tilesets/World_A2.png",
			"Resources/Graphics/Tilesets/World_A2.txt", 512, 384, 32 },
		{ RtpTilesetFamily::World, RtpTileSheet::B,
			"Resources/Graphics/Tilesets/World_B.png",
			"Resources/Graphics/Tilesets/World_B.txt", 512, 512, 256 }
	};

	const int SHEET_COUNT = sizeof(SHEETS) / sizeof(SHEETS[0]);

	struct TreeAutotileDescriptor
	{
		int canonicalIndex;
		int sourceX;
		int sourceY;
		int sourceWidth;
		int members[8];
		int memberCount;
		const char* name;
	};

	const TreeAutotileDescriptor TREE_AUTOTILES[] = {
		{ 93, 160, 352, 32, { 93, 94, 101 }, 3, "Tree" },
		{ 112, 0, 448, 64,
			{ 112, 113, 114, 115, 120, 121, 122, 123 }, 8, "Large Tree" },
		{ 128, 256, 0, 64,
			{ 128, 129, 130, 131, 136, 137, 138, 139 }, 8, "Large Snowy Tree" },
		{ 157, 416, 96, 32, { 157, 158, 165 }, 3, "Snowy Tree" },
		{ 168, 256, 160, 32,
			{ 168, 169, 176, 177 }, 4, "Spooky Tree" },
		{ 172, 384, 160, 32, { 172, 173, 180 }, 3, "Palm Tree" }
	};

	const int TREE_AUTOTILE_COUNT = sizeof(TREE_AUTOTILES) /
		sizeof(TREE_AUTOTILES[0]);

	const TreeAutotileDescriptor* treeAutotileForMember(int tileIndex)
	{
		for (int tree = 0; tree < TREE_AUTOTILE_COUNT; ++tree)
			for (int member = 0; member < TREE_AUTOTILES[tree].memberCount; ++member)
				if (TREE_AUTOTILES[tree].members[member] == tileIndex)
					return &TREE_AUTOTILES[tree];
		return NULL;
	}

	const TreeAutotileDescriptor* treeAutotileForCanonical(int tileIndex)
	{
		for (int tree = 0; tree < TREE_AUTOTILE_COUNT; ++tree)
			if (TREE_AUTOTILES[tree].canonicalIndex == tileIndex)
				return &TREE_AUTOTILES[tree];
		return NULL;
	}

	bool renderQuarter(SDL_Renderer* renderer, SDL_Texture* texture,
		const SDL_Rect& source, const SDL_Rect& destination)
	{
		return renderer != NULL && texture != NULL &&
			SDL_RenderCopy(renderer, texture, &source, &destination) == 0;
	}
}

RtpTilesetRenderer::RtpTilesetRenderer(SDL_Renderer* renderer, AssetManager* assets)
	: mRenderer(renderer), mAssets(assets)
{
}

bool RtpTilesetRenderer::draw(const RtpTileReference& tile, unsigned int connections,
	const SDL_Rect& destination, unsigned int animationFrame)
{
	const RtpSheetDescriptor* sheet = descriptor(tile.family, tile.sheet);
	if (sheet == NULL || tile.index < 0 || tile.index >= sheet->tileCount) return false;
	if (isTreeAutotile(tile))
		return drawTreeAutotile(tile, destination);
	if (tile.sheet == RtpTileSheet::A1 || tile.sheet == RtpTileSheet::A2 ||
		tile.sheet == RtpTileSheet::A3 || tile.sheet == RtpTileSheet::A4)
		return drawAutotile(tile, *sheet, connections, destination, animationFrame);
	return drawRegular(tile, *sheet, destination);
}

bool RtpTilesetRenderer::drawTreeAutotile(const RtpTileReference& tile,
	const SDL_Rect& destination, bool drawCanopy, bool drawBase)
{
	if (tile.family != RtpTilesetFamily::Outside || tile.sheet != RtpTileSheet::B ||
		mAssets == NULL || mRenderer == NULL || (!drawCanopy && !drawBase)) return false;
	const TreeAutotileDescriptor* tree = treeAutotileForCanonical(tile.index);
	const RtpSheetDescriptor* sheet = descriptor(tile.family, tile.sheet);
	if (tree == NULL || sheet == NULL) return false;
	SDL_Texture* texture = mAssets->texture(sheet->imagePath, true);
	if (texture == NULL) return false;

	int columns = tree->sourceWidth / 32;
	SDL_Rect topSource = { tree->sourceX, tree->sourceY, tree->sourceWidth, 32 };
	SDL_Rect bottomSource = { tree->sourceX, tree->sourceY + 32,
		tree->sourceWidth, 32 };
	SDL_Rect topTarget = { destination.x - (columns - 1) * destination.w,
		destination.y - destination.h, columns * destination.w, destination.h };
	SDL_Rect bottomTarget = { topTarget.x, destination.y, topTarget.w, destination.h };
	if (topSource.x < 0 || bottomSource.x < 0 || topSource.y < 0 ||
		topSource.x + topSource.w > sheet->width ||
		bottomSource.x + bottomSource.w > sheet->width ||
		bottomSource.y + bottomSource.h > sheet->height) return false;
	SDL_SetTextureColorMod(texture, tile.red, tile.green, tile.blue);
	bool rendered = true;
	if (drawCanopy)
		rendered = SDL_RenderCopy(mRenderer, texture, &topSource, &topTarget) == 0;
	if (drawBase)
		rendered = SDL_RenderCopy(mRenderer, texture, &bottomSource, &bottomTarget) == 0 &&
			rendered;
	SDL_SetTextureColorMod(texture, 255, 255, 255);
	return rendered;
}

bool RtpTilesetRenderer::drawTreeLayer(const RtpTileReference& tile,
	RtpRenderLayer layer, const SDL_Rect& destination)
{
	if (!isTreeAutotile(tile)) return false;
	if (layer == RtpRenderLayer::Decoration)
		return drawTreeAutotile(tile, destination, false, true);
	if (layer == RtpRenderLayer::Foreground)
		return drawTreeAutotile(tile, destination, true, false);
	return false;
}

bool RtpTilesetRenderer::drawLayer(const std::vector<RtpTileReference>& tiles,
	RtpRenderLayer layer, unsigned int connections, const SDL_Rect& destination,
	unsigned int animationFrame)
{
	bool drewAny = false;
	for (size_t i = 0; i < tiles.size(); ++i)
		if (tiles[i].layer == layer)
			drewAny = draw(tiles[i], connections, destination, animationFrame) || drewAny;
	return drewAny;
}

bool RtpTilesetRenderer::drawRegular(const RtpTileReference& tile,
	const RtpSheetDescriptor& sheet, const SDL_Rect& destination)
{
	if (mAssets == NULL || mRenderer == NULL) return false;
	SDL_Texture* texture = mAssets->texture(sheet.imagePath, true);
	if (texture == NULL) return false;
	SDL_Rect source;
	if (!regularTileSource(tile.sheet, tile.index, sheet.width, sheet.height, source))
		return false;
	SDL_SetTextureColorMod(texture, tile.red, tile.green, tile.blue);
	bool rendered = SDL_RenderCopy(mRenderer, texture, &source, &destination) == 0;
	SDL_SetTextureColorMod(texture, 255, 255, 255);
	return rendered;
}

bool RtpTilesetRenderer::drawAutotile(const RtpTileReference& tile,
	const RtpSheetDescriptor& sheet, unsigned int connections,
	const SDL_Rect& destination, unsigned int animationFrame)
{
	if (mAssets == NULL || mRenderer == NULL) return false;
	SDL_Texture* texture = mAssets->texture(sheet.imagePath, true);
	if (texture == NULL) return false;
	int originX = 0;
	int originY = 0;
	AutotileFormat format = AutotileFormat::None;
	if (!autotileOrigin(tile, animationFrame, originX, originY, format)) return false;
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
	SDL_SetTextureColorMod(texture, tile.red, tile.green, tile.blue);
	bool rendered = true;
	for (int quadrant = 0; quadrant < 4; ++quadrant)
	{
		SDL_Point sourceQuarter;
		bool valid = format == AutotileFormat::Floor ?
			floorQuarterSource(quadrant, connections, sourceQuarter) :
			(format == AutotileFormat::Wall ?
				wallQuarterSource(quadrant, connections, sourceQuarter) :
				waterfallQuarterSource(quadrant, connections, sourceQuarter));
		if (!valid)
		{
			rendered = false;
			break;
		}
		SDL_Rect source = { originX + sourceQuarter.x * 16,
			originY + sourceQuarter.y * 16, 16, 16 };
		if (source.x < 0 || source.y < 0 || source.x + source.w > sheet.width ||
			source.y + source.h > sheet.height ||
			!renderQuarter(mRenderer, texture, source, targets[quadrant]))
			rendered = false;
	}
	SDL_SetTextureColorMod(texture, 255, 255, 255);
	return rendered;
}

bool RtpTilesetRenderer::autotileOrigin(const RtpTileReference& tile,
	unsigned int animationFrame, int& sourceX, int& sourceY, AutotileFormat& format)
{
	if (tile.sheet == RtpTileSheet::A2)
	{
		sourceX = (tile.index % 8) * 64;
		sourceY = (tile.index / 8) * 96;
		format = AutotileFormat::Floor;
		return true;
	}
	if (tile.sheet == RtpTileSheet::A3)
	{
		sourceX = (tile.index % 8) * 64;
		sourceY = (tile.index / 8) * 64;
		format = AutotileFormat::Wall;
		return true;
	}
	if (tile.sheet == RtpTileSheet::A4)
	{
		int row = tile.index / 8;
		sourceX = (tile.index % 8) * 64;
		if (row % 2 == 0)
		{
			sourceY = (row / 2) * 160;
			format = AutotileFormat::Floor;
		}
		else
		{
			sourceY = (row / 2) * 160 + 96;
			format = AutotileFormat::Wall;
		}
		return true;
	}
	if (tile.sheet != RtpTileSheet::A1) return false;
	unsigned int frame = animationFrame % 4;
	if (frame == 3) frame = 1;
	if (tile.index == 0 || tile.index == 1)
	{
		sourceX = (int)frame * 64;
		sourceY = tile.index == 0 ? 0 : 96;
		format = AutotileFormat::Floor;
		return true;
	}
	if (tile.index == 2 || tile.index == 3)
	{
		sourceX = 192;
		sourceY = tile.index == 2 ? 0 : 96;
		format = AutotileFormat::Floor;
		return true;
	}
	int tx = tile.index % 8;
	int ty = tile.index / 8;
	int blockX = (tx / 4) * 8;
	int blockY = ty * 6 + ((tx / 2) % 2) * 3;
	if (tile.index % 2 == 0)
	{
		blockX += (int)frame * 2;
		format = AutotileFormat::Floor;
	}
	else
	{
		blockX += 6;
		format = AutotileFormat::Waterfall;
	}
	sourceX = blockX * 32;
	sourceY = blockY * 32;
	return true;
}

std::vector<RtpSheetDescriptor> RtpTilesetRenderer::availableSheets()
{
	return std::vector<RtpSheetDescriptor>(SHEETS, SHEETS + SHEET_COUNT);
}

const RtpSheetDescriptor* RtpTilesetRenderer::descriptor(RtpTilesetFamily family,
	RtpTileSheet sheet)
{
	for (int i = 0; i < SHEET_COUNT; ++i)
		if (SHEETS[i].family == family && SHEETS[i].sheet == sheet) return &SHEETS[i];
	return NULL;
}

bool RtpTilesetRenderer::loadTileNames(RtpTilesetFamily family, RtpTileSheet sheet,
	std::vector<std::string>& names, std::string& error)
{
	names.clear();
	const RtpSheetDescriptor* info = descriptor(family, sheet);
	if (info == NULL)
	{
		error = "Tileset sheet is not available";
		return false;
	}
	std::ifstream input(info->metadataPath);
	if (!input)
	{
		error = std::string("Unable to open ") + info->metadataPath;
		return false;
	}
	std::string line;
	while (std::getline(input, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
		if (names.empty() && line.size() >= 3 &&
			(unsigned char)line[0] == 0xef && (unsigned char)line[1] == 0xbb &&
			(unsigned char)line[2] == 0xbf) line.erase(0, 3);
		size_t separator = line.find('|');
		names.push_back(line.substr(0, separator));
	}
	if ((int)names.size() != info->tileCount)
	{
		error = std::string("Tile metadata count mismatch for ") + info->metadataPath;
		return false;
	}
	return true;
}

bool RtpTilesetRenderer::metadataName(const RtpTileReference& tile,
	std::string& name)
{
	static std::map<std::pair<int, int>, std::vector<std::string> > cache;
	std::pair<int, int> key((int)tile.family, (int)tile.sheet);
	if (cache.count(key) == 0)
	{
		std::vector<std::string> names;
		std::string error;
		if (!loadTileNames(tile.family, tile.sheet, names, error)) return false;
		cache[key] = names;
	}
	const std::vector<std::string>& names = cache[key];
	if (tile.index < 0 || tile.index >= (int)names.size()) return false;
	name = names[tile.index];
	return true;
}

RtpTileCollision RtpTilesetRenderer::collision(const RtpTileReference& tile)
{
	if (tile.family == RtpTilesetFamily::World) return RtpTileCollision::Ignore;
	if (tile.sheet == RtpTileSheet::A1) return RtpTileCollision::Blocked;
	if (tile.sheet == RtpTileSheet::A3 || tile.sheet == RtpTileSheet::A4)
		return RtpTileCollision::Blocked;
	std::string name;
	if (!metadataName(tile, name)) return RtpTileCollision::Blocked;
	if (name.empty() || name == "Transparent") return RtpTileCollision::Ignore;
	if (tile.sheet == RtpTileSheet::A2)
		return inferredLayer(tile) == RtpRenderLayer::Decoration ?
			RtpTileCollision::Blocked : RtpTileCollision::Walkable;
	if (tile.sheet == RtpTileSheet::A5)
	{
		if (name.find("Broken Bridge") == 0 || name.find("Cliff") == 0 ||
			name.find("Ledge") == 0 || name == "Darkness")
			return RtpTileCollision::Blocked;
		return RtpTileCollision::Walkable;
	}
	bool passage = name == "Entrance" || name == "Exit" || name == "Gate" ||
		name.find("(Gate)") != std::string::npos ||
		name.find("Cave Entrance") == 0 || name.find("Mine Entrance") == 0 ||
		(name.find("Bridge") != std::string::npos &&
			name.find("Bridge Spar") == std::string::npos &&
			name.find("Broken Bridge") == std::string::npos) ||
		name.find("Stairs") == 0 || name.find("Ladder") != std::string::npos ||
		name.find("Rug") == 0 || name.find("Carpet") == 0 ||
		name.find("Straw Mat") == 0;
	return passage ? RtpTileCollision::Walkable : RtpTileCollision::Blocked;
}

bool RtpTilesetRenderer::validateAllAssets(std::string& error)
{
	if (mAssets == NULL)
	{
		error = "Asset manager is unavailable";
		return false;
	}
	for (int i = 0; i < SHEET_COUNT; ++i)
	{
		int width = 0;
		int height = 0;
		if (!mAssets->textureSize(SHEETS[i].imagePath, width, height, true) ||
			width != SHEETS[i].width || height != SHEETS[i].height)
		{
			error = std::string("Invalid tileset image: ") + SHEETS[i].imagePath;
			return false;
		}
		std::vector<std::string> names;
		if (!loadTileNames(SHEETS[i].family, SHEETS[i].sheet, names, error)) return false;
	}
	return true;
}

RtpRenderLayer RtpTilesetRenderer::inferredLayer(const RtpTileReference& tile)
{
	if (tile.sheet == RtpTileSheet::A1)
		return tile.index >= 1 && tile.index <= 3 ?
			RtpRenderLayer::Decoration : RtpRenderLayer::Ground;
	if (tile.sheet == RtpTileSheet::A2)
		return tile.index >= 0 && tile.index % 8 >= 4 ?
			RtpRenderLayer::Decoration : RtpRenderLayer::Ground;
	if (tile.sheet != RtpTileSheet::B && tile.sheet != RtpTileSheet::C)
		return RtpRenderLayer::Ground;
	std::string name;
	if (metadataName(tile, name) &&
		(name.find("(Front") != std::string::npos ||
		name.find("Foreground") != std::string::npos))
		return RtpRenderLayer::Foreground;
	return RtpRenderLayer::Decoration;
}

int RtpTilesetRenderer::canonicalTileIndex(RtpTilesetFamily family,
	RtpTileSheet sheet, int tileIndex)
{
	if (family != RtpTilesetFamily::Outside || sheet != RtpTileSheet::B)
		return tileIndex;
	const TreeAutotileDescriptor* tree = treeAutotileForMember(tileIndex);
	return tree == NULL ? tileIndex : tree->canonicalIndex;
}

bool RtpTilesetRenderer::isTreeAutotile(const RtpTileReference& tile)
{
	return tile.family == RtpTilesetFamily::Outside &&
		tile.sheet == RtpTileSheet::B &&
		treeAutotileForCanonical(tile.index) != NULL;
}

bool RtpTilesetRenderer::treeAutotileFootprint(const RtpTileReference& tile,
	int& width, int& height)
{
	width = 0;
	height = 0;
	if (!isTreeAutotile(tile)) return false;
	const TreeAutotileDescriptor* tree = treeAutotileForCanonical(tile.index);
	if (tree == NULL) return false;
	width = tree->sourceWidth / 32;
	height = 2;
	return true;
}

bool RtpTilesetRenderer::largeTreeAnchorsConflict(
	const RtpTileReference& first, int firstX, int firstY,
	const RtpTileReference& second, int secondX, int secondY)
{
	int firstWidth = 0;
	int firstHeight = 0;
	int secondWidth = 0;
	int secondHeight = 0;
	if (!treeAutotileFootprint(first, firstWidth, firstHeight) ||
		!treeAutotileFootprint(second, secondWidth, secondHeight) ||
		firstWidth != 2 || secondWidth != 2) return false;
	int columnDistance = firstX > secondX ? firstX - secondX : secondX - firstX;
	int rowDistance = firstY > secondY ? firstY - secondY : secondY - firstY;
	return columnDistance == 1 && rowDistance == 0;
}

const char* RtpTilesetRenderer::treeAutotileName(RtpTilesetFamily family,
	RtpTileSheet sheet, int tileIndex)
{
	if (family != RtpTilesetFamily::Outside || sheet != RtpTileSheet::B) return NULL;
	const TreeAutotileDescriptor* tree = treeAutotileForMember(tileIndex);
	return tree == NULL ? NULL : tree->name;
}

bool RtpTilesetRenderer::regularTileSource(RtpTileSheet sheet, int tileIndex,
	int textureWidth, int textureHeight, SDL_Rect& source)
{
	if (tileIndex < 0 || textureWidth <= 0 || textureHeight <= 0) return false;
	if (sheet == RtpTileSheet::A5)
		source = { (tileIndex % 8) * 32, (tileIndex / 8) * 32, 32, 32 };
	else if (sheet == RtpTileSheet::B || sheet == RtpTileSheet::C)
	{
		int page = tileIndex / 128;
		int withinPage = tileIndex % 128;
		source = { (page * 8 + withinPage % 8) * 32,
			(withinPage / 8) * 32, 32, 32 };
	}
	else return false;
	return source.x + source.w <= textureWidth && source.y + source.h <= textureHeight;
}

bool RtpTilesetRenderer::paletteTileSource(RtpTileSheet sheet, int tileIndex,
	SDL_Rect& source)
{
	if (tileIndex < 0) return false;
	if (sheet == RtpTileSheet::A1)
	{
		if (tileIndex >= 16) return false;
		if (tileIndex < 4)
		{
			bool animated = tileIndex < 2;
			source = { animated ? 0 : 192, (tileIndex % 2) * 96,
				animated ? 192 : 64, 96 };
			return true;
		}
		int tx = tileIndex % 8;
		int ty = tileIndex / 8;
		bool floor = tileIndex % 2 == 0;
		source = { (tx / 4) * 256 + (floor ? 0 : 192),
			ty * 192 + ((tx / 2) % 2) * 96, floor ? 192 : 64, 96 };
		return true;
	}
	if (sheet == RtpTileSheet::A2)
	{
		if (tileIndex >= 32) return false;
		source = { (tileIndex % 8) * 64, (tileIndex / 8) * 96, 64, 96 };
		return true;
	}
	if (sheet == RtpTileSheet::A3)
	{
		if (tileIndex >= 32) return false;
		source = { (tileIndex % 8) * 64, (tileIndex / 8) * 64, 64, 64 };
		return true;
	}
	if (sheet == RtpTileSheet::A4)
	{
		if (tileIndex >= 48) return false;
		int row = tileIndex / 8;
		source = { (tileIndex % 8) * 64,
			(row / 2) * 160 + (row % 2 == 0 ? 0 : 96), 64,
			row % 2 == 0 ? 96 : 64 };
		return true;
	}
	return regularTileSource(sheet, tileIndex, 512, 512, source);
}

bool RtpTilesetRenderer::floorQuarterSource(int quadrant, unsigned int connections,
	SDL_Point& sourceQuarter)
{
	bool north = (connections & North) != 0;
	bool east = (connections & East) != 0;
	bool south = (connections & South) != 0;
	bool west = (connections & West) != 0;
	if (quadrant == 0)
	{
		if (north && west)
			sourceQuarter = (connections & NorthWest) != 0 ? SDL_Point{ 2, 4 } : SDL_Point{ 2, 0 };
		else if (north) sourceQuarter = { 0, 4 };
		else if (west) sourceQuarter = { 2, 2 };
		else sourceQuarter = { 0, 2 };
	}
	else if (quadrant == 1)
	{
		if (north && east)
			sourceQuarter = (connections & NorthEast) != 0 ? SDL_Point{ 1, 4 } : SDL_Point{ 3, 0 };
		else if (north) sourceQuarter = { 3, 4 };
		else if (east) sourceQuarter = { 1, 2 };
		else sourceQuarter = { 3, 2 };
	}
	else if (quadrant == 2)
	{
		if (south && west)
			sourceQuarter = (connections & SouthWest) != 0 ? SDL_Point{ 2, 3 } : SDL_Point{ 2, 1 };
		else if (south) sourceQuarter = { 0, 3 };
		else if (west) sourceQuarter = { 2, 5 };
		else sourceQuarter = { 0, 5 };
	}
	else if (quadrant == 3)
	{
		if (south && east)
			sourceQuarter = (connections & SouthEast) != 0 ? SDL_Point{ 1, 3 } : SDL_Point{ 3, 1 };
		else if (south) sourceQuarter = { 3, 3 };
		else if (east) sourceQuarter = { 1, 5 };
		else sourceQuarter = { 3, 5 };
	}
	else return false;
	return true;
}

bool RtpTilesetRenderer::wallQuarterSource(int quadrant, unsigned int connections,
	SDL_Point& sourceQuarter)
{
	if (quadrant < 0 || quadrant > 3) return false;
	bool left = quadrant % 2 == 0;
	bool top = quadrant < 2;
	bool horizontal = left ? (connections & West) != 0 : (connections & East) != 0;
	bool vertical = top ? (connections & North) != 0 : (connections & South) != 0;
	sourceQuarter.x = left ? (horizontal ? 2 : 0) : (horizontal ? 1 : 3);
	sourceQuarter.y = top ? (vertical ? 2 : 0) : (vertical ? 1 : 3);
	return true;
}

bool RtpTilesetRenderer::waterfallQuarterSource(int quadrant,
	unsigned int connections, SDL_Point& sourceQuarter)
{
	if (quadrant < 0 || quadrant > 3) return false;
	bool left = quadrant % 2 == 0;
	bool connected = left ? (connections & West) != 0 : (connections & East) != 0;
	sourceQuarter.x = left ? (connected ? 2 : 0) : (connected ? 1 : 3);
	sourceQuarter.y = quadrant < 2 ? 0 : 1;
	return true;
}
