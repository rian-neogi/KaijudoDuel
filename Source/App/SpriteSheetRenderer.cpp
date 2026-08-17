#include "SpriteSheetRenderer.h"

#include "AssetManager.h"

#include <algorithm>

namespace
{
	bool singleCharacterSheet(const std::string& path)
	{
		size_t slash = path.find_last_of("/\\");
		std::string filename = slash == std::string::npos ? path : path.substr(slash + 1);
		return filename.find('$') != std::string::npos;
	}

	int directionRow(int facingX, int facingY)
	{
		if (facingY < 0) return 3;
		if (facingX < 0) return 1;
		if (facingX > 0) return 2;
		return 0;
	}
}

SpriteSheetRenderer::SpriteSheetRenderer(SDL_Renderer* renderer, AssetManager* assets)
	: mRenderer(renderer), mAssets(assets)
{
}

bool SpriteSheetRenderer::characterSourceRect(const std::string& sheet,
	int characterIndex, int facingX, int facingY, bool walking, Uint32 ticks,
	int textureWidth, int textureHeight, SDL_Rect& source)
{
	const bool single = singleCharacterSheet(sheet);
	const int sheetColumns = single ? 3 : 12;
	const int sheetRows = single ? 4 : 8;
	const int characterCount = single ? 1 : 8;
	if (textureWidth <= 0 || textureHeight <= 0 || textureWidth % sheetColumns != 0 ||
		textureHeight % sheetRows != 0 || characterIndex < 0 ||
		characterIndex >= characterCount) return false;

	const int frameWidth = textureWidth / sheetColumns;
	const int frameHeight = textureHeight / sheetRows;
	const int walkFrames[4] = { 0, 1, 2, 1 };
	const int frame = walking ? walkFrames[(ticks / 120) % 4] : 1;
	const int characterColumn = single ? 0 : (characterIndex % 4) * 3;
	const int characterRow = single ? 0 : (characterIndex / 4) * 4;
	source = { (characterColumn + frame) * frameWidth,
		(characterRow + directionRow(facingX, facingY)) * frameHeight,
		frameWidth, frameHeight };
	return true;
}

bool SpriteSheetRenderer::drawCharacter(const CharacterSpriteDefinition& definition,
	int facingX, int facingY, bool walking, Uint32 ticks, const SDL_Rect& destination)
{
	if (mRenderer == NULL || mAssets == NULL || definition.sheet.empty()) return false;
	SDL_Texture* texture = mAssets->texture(definition.sheet, true);
	if (texture == NULL) return false;
	int textureWidth = 0;
	int textureHeight = 0;
	if (SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight) != 0) return false;
	SDL_Rect source;
	if (!characterSourceRect(definition.sheet, definition.characterIndex,
		facingX, facingY, walking, ticks, textureWidth, textureHeight, source)) return false;
	return SDL_RenderCopy(mRenderer, texture, &source, &destination) == 0;
}

bool SpriteSheetRenderer::drawMapObject(const CharacterSpriteDefinition& definition,
	int facingX, int facingY, bool animated, Uint32 ticks,
	const SDL_Rect& anchorTile)
{
	if (mRenderer == NULL || mAssets == NULL || definition.sheet.empty()) return false;
	SDL_Texture* texture = mAssets->texture(definition.sheet, true);
	if (texture == NULL) return false;
	int textureWidth = 0;
	int textureHeight = 0;
	if (SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight) != 0) return false;
	SDL_Rect source;
	if (!characterSourceRect(definition.sheet, definition.characterIndex,
		facingX, facingY, animated, ticks, textureWidth, textureHeight, source)) return false;
	const int height = std::max(1, anchorTile.w * source.h / source.w);
	const int width = std::max(1, anchorTile.w);
	SDL_Rect destination = { anchorTile.x + (anchorTile.w - width) / 2,
		anchorTile.y + anchorTile.h - height, width, height };
	return SDL_RenderCopy(mRenderer, texture, &source, &destination) == 0;
}
