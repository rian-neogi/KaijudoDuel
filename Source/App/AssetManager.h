#pragma once

#include <SDL.h>

#include <map>
#include <string>

class AssetManager
{
public:
	explicit AssetManager(SDL_Renderer* renderer);
	~AssetManager();

	SDL_Texture* texture(const std::string& path, bool pixelArt = false);
	bool textureSize(const std::string& path, int& width, int& height,
		bool pixelArt = false);
	void clear();
	size_t cachedTextureCount() const;

private:
	AssetManager(const AssetManager&);
	AssetManager& operator=(const AssetManager&);

	SDL_Renderer* mRenderer;
	std::map<std::string, SDL_Texture*> mTextures;
};
