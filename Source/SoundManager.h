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
	void playEmberglenTheme();
	void stopMusic();
	void setMusicVolume(int percent);
	void setSoundVolume(int percent);
	bool isAvailable() const;

private:
	struct Voice
	{
		int soundId;
		Uint32 position;
	};

	bool loadSound(int soundid, const std::string& path);
	bool convertAudio(const SDL_AudioSpec& sourceSpec, const Uint8* sourceData,
		Uint32 sourceLength, std::vector<Uint8>& destination);
	void buildEmberglenTheme();
	static void audioCallback(void* userdata, Uint8* stream, int length);
	void mixAudio(Uint8* stream, int length);

	SDL_AudioDeviceID mDevice;
	SDL_AudioSpec mOutputSpec;
	std::array<std::vector<Uint8>, SOUND_COUNT> mSounds;
	std::array<Uint32, SOUND_COUNT> mLastPlayed;
	std::vector<Uint8> mEmberglenTheme;
	std::vector<Voice> mVoices;
	Uint32 mMusicPosition;
	bool mMusicPlaying;
	int mMusicVolume;
	int mSoundVolume;
	std::mutex mMutex;
};

extern SoundManager* SoundMngr;
