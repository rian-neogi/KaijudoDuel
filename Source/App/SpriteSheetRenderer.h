#pragma once

#include <SDL.h>

#include <string>

class AssetManager;

struct CharacterSpriteDefinition
{
	std::string sheet;
	int characterIndex;
};

class SpriteSheetRenderer
{
public:
	SpriteSheetRenderer(SDL_Renderer* renderer, AssetManager* assets);

	bool drawCharacter(const CharacterSpriteDefinition& definition,
		int facingX, int facingY, bool walking, Uint32 ticks,
		const SDL_Rect& destination);
	static bool characterSourceRect(const std::string& sheet, int characterIndex,
		int facingX, int facingY, bool walking, Uint32 ticks,
		int textureWidth, int textureHeight, SDL_Rect& source);

private:
	SDL_Renderer* mRenderer;
	AssetManager* mAssets;
};
