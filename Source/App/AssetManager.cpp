#include "AssetManager.h"

#include <SDL_image.h>

#include <iostream>

AssetManager::AssetManager(SDL_Renderer* renderer)
	: mRenderer(renderer)
{
}

AssetManager::~AssetManager()
{
	clear();
}

SDL_Texture* AssetManager::texture(const std::string& path, bool pixelArt)
{
	std::map<std::string, SDL_Texture*>::iterator existing = mTextures.find(path);
	if (existing != mTextures.end()) return existing->second;

	SDL_Texture* loaded = mRenderer == NULL ? NULL : IMG_LoadTexture(mRenderer, path.c_str());
	if (loaded == NULL)
		std::cerr << "Unable to load texture '" << path << "': " << IMG_GetError() << std::endl;
	else
	{
		SDL_SetTextureBlendMode(loaded, SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_SetTextureScaleMode(loaded, pixelArt ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
#else
		(void)pixelArt;
#endif
	}
	mTextures[path] = loaded;
	return loaded;
}

bool AssetManager::textureSize(const std::string& path, int& width, int& height,
	bool pixelArt)
{
	SDL_Texture* loaded = texture(path, pixelArt);
	return loaded != NULL && SDL_QueryTexture(loaded, NULL, NULL, &width, &height) == 0;
}

void AssetManager::clear()
{
	for (std::map<std::string, SDL_Texture*>::iterator item = mTextures.begin();
		item != mTextures.end(); ++item)
		if (item->second != NULL) SDL_DestroyTexture(item->second);
	mTextures.clear();
}

size_t AssetManager::cachedTextureCount() const
{
	return mTextures.size();
}
