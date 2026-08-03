#include "SoundManager.h"

#include <cstring>
#include <iostream>

SoundManager* SoundMngr = NULL;

SoundManager::SoundManager()
	: mDevice(0), mOutputSpec()
{
	mLastPlayed.fill(0);
	if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		std::cerr << "Audio initialization failed; sound is disabled: " << SDL_GetError() << std::endl;
		return;
	}

	SDL_AudioSpec desired;
	SDL_zero(desired);
	desired.freq = 44100;
	desired.format = AUDIO_S16SYS;
	desired.channels = 1;
	desired.samples = 1024;
	mDevice = SDL_OpenAudioDevice(NULL, 0, &desired, &mOutputSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if (mDevice == 0)
	{
		std::cerr << "Audio device unavailable; sound is disabled: " << SDL_GetError() << std::endl;
		return;
	}

	loadSound(SOUND_CARD_MOVE, "Resources/Sounds/playcard.wav");
	loadSound(SOUND_ATTACK_SMALL, "Resources/Sounds/button2.wav");
	loadSound(SOUND_ATTACK_MEDIUM, "Resources/Sounds/tap.wav");
	loadSound(SOUND_ATTACK_LARGE, "Resources/Sounds/rolldie.wav");
	loadSound(SOUND_EVOLUTION, "Resources/Sounds/stagechangeoldnotification.wav");
	SDL_PauseAudioDevice(mDevice, 0);
}

SoundManager::~SoundManager()
{
	if (mDevice != 0)
		SDL_CloseAudioDevice(mDevice);
}

bool SoundManager::loadSound(int soundid, const std::string& path)
{
	SDL_AudioSpec sourceSpec;
	Uint8* sourceData = NULL;
	Uint32 sourceLength = 0;
	if (SDL_LoadWAV(path.c_str(), &sourceSpec, &sourceData, &sourceLength) == NULL)
	{
		std::cerr << "Unable to load sound '" << path << "': " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_AudioCVT converter;
	int conversion = SDL_BuildAudioCVT(&converter,
		sourceSpec.format, sourceSpec.channels, sourceSpec.freq,
		mOutputSpec.format, mOutputSpec.channels, mOutputSpec.freq);
	if (conversion < 0)
	{
		std::cerr << "Unable to convert sound '" << path << "': " << SDL_GetError() << std::endl;
		SDL_FreeWAV(sourceData);
		return false;
	}

	if (conversion == 0)
	{
		mSounds[soundid].assign(sourceData, sourceData + sourceLength);
	}
	else
	{
		converter.len = static_cast<int>(sourceLength);
		converter.buf = static_cast<Uint8*>(SDL_malloc(sourceLength * converter.len_mult));
		if (converter.buf == NULL)
		{
			SDL_FreeWAV(sourceData);
			return false;
		}
		std::memcpy(converter.buf, sourceData, sourceLength);
		if (SDL_ConvertAudio(&converter) != 0)
		{
			std::cerr << "Unable to convert sound '" << path << "': " << SDL_GetError() << std::endl;
			SDL_free(converter.buf);
			SDL_FreeWAV(sourceData);
			return false;
		}
		mSounds[soundid].assign(converter.buf, converter.buf + converter.len_cvt);
		SDL_free(converter.buf);
	}
	SDL_FreeWAV(sourceData);
	return true;
}

void SoundManager::playSound(int soundid)
{
	if (soundid < 0 || soundid >= SOUND_COUNT || mDevice == 0 || mSounds[soundid].empty())
		return;

	std::lock_guard<std::mutex> lock(mMutex);
	Uint32 now = SDL_GetTicks();
	if (mLastPlayed[soundid] != 0 && now - mLastPlayed[soundid] < 40)
		return;
	mLastPlayed[soundid] = now;

	int bytesPerSample = SDL_AUDIO_BITSIZE(mOutputSpec.format) / 8;
	Uint32 twoSeconds = static_cast<Uint32>(mOutputSpec.freq * mOutputSpec.channels * bytesPerSample * 2);
	if (SDL_GetQueuedAudioSize(mDevice) > twoSeconds)
		SDL_ClearQueuedAudio(mDevice);
	SDL_QueueAudio(mDevice, mSounds[soundid].data(), static_cast<Uint32>(mSounds[soundid].size()));
}

bool SoundManager::isAvailable() const
{
	return mDevice != 0;
}
