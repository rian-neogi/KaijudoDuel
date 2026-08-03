#pragma once

#include <SDL.h>

#include <array>
#include <mutex>
#include <string>
#include <vector>

enum SoundId
{
	SOUND_CARD_MOVE,
	SOUND_ATTACK_SMALL,
	SOUND_ATTACK_MEDIUM,
	SOUND_ATTACK_LARGE,
	SOUND_EVOLUTION,
	SOUND_UI_CARD_ADD,
	SOUND_UI_CARD_REMOVE,
	SOUND_UI_SCROLL,
	SOUND_UI_PURCHASE,
	SOUND_COUNT
};

class SoundManager
{
public:
	SoundManager();
	~SoundManager();

	void playSound(int soundid);
	bool isAvailable() const;

private:
	bool loadSound(int soundid, const std::string& path);

	SDL_AudioDeviceID mDevice;
	SDL_AudioSpec mOutputSpec;
	std::array<std::vector<Uint8>, SOUND_COUNT> mSounds;
	std::array<Uint32, SOUND_COUNT> mLastPlayed;
	std::mutex mMutex;
};

extern SoundManager* SoundMngr;
