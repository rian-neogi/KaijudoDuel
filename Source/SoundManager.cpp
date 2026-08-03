#include "SoundManager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

SoundManager* SoundMngr = NULL;

SoundManager::SoundManager()
	: mDevice(0), mOutputSpec(), mMusicPosition(0), mMusicPlaying(false),
	  mMusicVolume(35), mSoundVolume(100)
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
	desired.callback = &SoundManager::audioCallback;
	desired.userdata = this;
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
	loadSound(SOUND_UI_CARD_ADD, "Resources/Sounds/playcard.wav");
	loadSound(SOUND_UI_CARD_REMOVE, "Resources/Sounds/button2.wav");
	loadSound(SOUND_UI_SCROLL, "Resources/Sounds/button1.wav");
	loadSound(SOUND_UI_PURCHASE, "Resources/Sounds/draw.wav");
	buildEmberglenTheme();
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

	bool converted = convertAudio(sourceSpec, sourceData, sourceLength, mSounds[soundid]);
	SDL_FreeWAV(sourceData);
	return converted;
}

bool SoundManager::convertAudio(const SDL_AudioSpec& sourceSpec, const Uint8* sourceData,
	Uint32 sourceLength, std::vector<Uint8>& destination)
{
	SDL_AudioCVT converter;
	int conversion = SDL_BuildAudioCVT(&converter, sourceSpec.format, sourceSpec.channels,
		sourceSpec.freq, mOutputSpec.format, mOutputSpec.channels, mOutputSpec.freq);
	if (conversion < 0)
	{
		std::cerr << "Unable to convert audio: " << SDL_GetError() << std::endl;
		return false;
	}

	if (conversion == 0)
	{
		destination.assign(sourceData, sourceData + sourceLength);
	}
	else
	{
		converter.len = static_cast<int>(sourceLength);
		converter.buf = static_cast<Uint8*>(SDL_malloc(sourceLength * converter.len_mult));
		if (converter.buf == NULL) return false;
		std::memcpy(converter.buf, sourceData, sourceLength);
		if (SDL_ConvertAudio(&converter) != 0)
		{
			std::cerr << "Unable to convert audio: " << SDL_GetError() << std::endl;
			SDL_free(converter.buf);
			return false;
		}
		destination.assign(converter.buf, converter.buf + converter.len_cvt);
		SDL_free(converter.buf);
	}
	return true;
}

void SoundManager::buildEmberglenTheme()
{
	const int sourceRate = 44100;
	const int tempo = 132;
	const int stepSamples = sourceRate * 60 / tempo / 4;
	const int bars = 16;
	const int totalSteps = bars * 16;
	const int totalSamples = totalSteps * stepSamples;
	const int phrases[4][16] = {
		{ 76, 80, 83, 80, 78, 76, 73, 71, 76, 80, 81, 83, 80, 78, 76, -1 },
		{ 78, 80, 83, 85, 83, 80, 78, 76, 73, 76, 78, 80, 78, 76, 73, -1 },
		{ 80, 81, 83, 88, 85, 83, 81, 80, 78, 80, 81, 85, 83, 81, 80, -1 },
		{ 76, 78, 80, 83, 81, 80, 78, 73, 76, 80, 78, 76, 73, 71, 76, -1 }
	};
	const int phraseOrder[8] = { 0, 1, 0, 2, 0, 1, 3, 2 };
	const int chordRoots[4] = { 52, 47, 49, 45 };
	const int arpeggioSteps[4] = { 0, 1, 2, 1 };
	std::vector<Sint16> samples(totalSamples, 0);
	Uint32 noise = 0x51f15e5d;
	auto frequency = [](int midi) -> double
	{
		return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
	};
	auto pulse = [](double phase, double duty) -> double
	{
		phase -= std::floor(phase);
		return phase < duty ? 1.0 : -1.0;
	};

	for (int sample = 0; sample < totalSamples; ++sample)
	{
		int step = sample / stepSamples;
		int bar = step / 16;
		int withinStep = sample % stepSamples;
		int leadSlot = (step / 2) % 16;
		int phrase = phraseOrder[bar / 2];
		int leadNote = phrases[phrase][leadSlot];
		double mixed = 0.0;

		if (leadNote >= 0)
		{
			int noteSample = sample % (stepSamples * 2);
			double envelope = std::min(1.0, noteSample / 120.0) *
				std::min(1.0, (stepSamples * 2 - noteSample) / 420.0);
			mixed += pulse(noteSample * frequency(leadNote) / sourceRate, 0.25) *
				5200.0 * envelope;
		}

		int chord = bar % 4;
		int third = chord == 2 ? 3 : 4;
		const int intervals[4] = { 0, third, 7, 12 };
		int arpeggioNote = chordRoots[chord] + 12 + intervals[arpeggioSteps[step % 4]];
		double arpEnvelope = std::min(1.0, withinStep / 70.0) *
			std::min(1.0, (stepSamples - withinStep) / 180.0);
		mixed += pulse(withinStep * frequency(arpeggioNote) / sourceRate, 0.125) *
			1900.0 * arpEnvelope;

		int beatSample = sample % (stepSamples * 4);
		double bassEnvelope = std::min(1.0, beatSample / 100.0) *
			std::min(1.0, (stepSamples * 4 - beatSample) / 600.0);
		mixed += pulse(beatSample * frequency(chordRoots[chord] - 12) / sourceRate, 0.5) *
			2500.0 * bassEnvelope;

		noise = noise * 1664525u + 1013904223u;
		double noiseSample = (noise & 0x80000000u) != 0 ? 1.0 : -1.0;
		if (step % 2 == 0 && withinStep < stepSamples / 7)
			mixed += noiseSample * 550.0 * (1.0 - withinStep / (stepSamples / 7.0));
		if ((step % 16 == 4 || step % 16 == 12) && withinStep < stepSamples / 3)
			mixed += noiseSample * 1300.0 * (1.0 - withinStep / (stepSamples / 3.0));
		if ((step % 16 == 0 || step % 16 == 8) && withinStep < stepSamples / 2)
		{
			double kickFrequency = 95.0 - 45.0 * withinStep / (stepSamples / 2.0);
			mixed += pulse(withinStep * kickFrequency / sourceRate, 0.5) * 1800.0 *
				(1.0 - withinStep / (stepSamples / 2.0));
		}

		samples[sample] = (Sint16)std::max(-30000.0, std::min(30000.0, mixed));
	}

	SDL_AudioSpec sourceSpec;
	SDL_zero(sourceSpec);
	sourceSpec.freq = sourceRate;
	sourceSpec.format = AUDIO_S16SYS;
	sourceSpec.channels = 1;
	convertAudio(sourceSpec, reinterpret_cast<const Uint8*>(samples.data()),
		(Uint32)(samples.size() * sizeof(Sint16)), mEmberglenTheme);
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
	if (mVoices.size() >= 12) mVoices.erase(mVoices.begin());
	mVoices.push_back({ soundid, 0 });
}

void SoundManager::playEmberglenTheme()
{
	if (mDevice == 0 || mEmberglenTheme.empty()) return;
	std::lock_guard<std::mutex> lock(mMutex);
	if (mMusicPlaying) return;
	mMusicPosition = 0;
	mMusicPlaying = true;
}

void SoundManager::stopMusic()
{
	if (mDevice == 0) return;
	std::lock_guard<std::mutex> lock(mMutex);
	if (!mMusicPlaying) return;
	mMusicPlaying = false;
	mMusicPosition = 0;
}

void SoundManager::setMusicVolume(int percent)
{
	std::lock_guard<std::mutex> lock(mMutex);
	mMusicVolume = std::max(0, std::min(100, percent));
}

void SoundManager::setSoundVolume(int percent)
{
	std::lock_guard<std::mutex> lock(mMutex);
	mSoundVolume = std::max(0, std::min(100, percent));
}

void SoundManager::audioCallback(void* userdata, Uint8* stream, int length)
{
	static_cast<SoundManager*>(userdata)->mixAudio(stream, length);
}

void SoundManager::mixAudio(Uint8* stream, int length)
{
	SDL_memset(stream, mOutputSpec.silence, length);
	std::lock_guard<std::mutex> lock(mMutex);
	if (mMusicPlaying && mMusicVolume > 0 && !mEmberglenTheme.empty())
	{
		int destination = 0;
		while (destination < length)
		{
			Uint32 remaining = (Uint32)mEmberglenTheme.size() - mMusicPosition;
			Uint32 chunk = std::min<Uint32>((Uint32)(length - destination), remaining);
			SDL_MixAudioFormat(stream + destination, mEmberglenTheme.data() + mMusicPosition,
				mOutputSpec.format, chunk, mMusicVolume * SDL_MIX_MAXVOLUME / 100);
			destination += (int)chunk;
			mMusicPosition += chunk;
			if (mMusicPosition >= mEmberglenTheme.size()) mMusicPosition = 0;
		}
	}

	for (size_t voice = 0; voice < mVoices.size();)
	{
		Voice& active = mVoices[voice];
		const std::vector<Uint8>& sound = mSounds[active.soundId];
		Uint32 remaining = (Uint32)sound.size() - active.position;
		Uint32 chunk = std::min<Uint32>((Uint32)length, remaining);
		if (mSoundVolume > 0)
			SDL_MixAudioFormat(stream, sound.data() + active.position, mOutputSpec.format,
				chunk, mSoundVolume * SDL_MIX_MAXVOLUME / 100);
		active.position += chunk;
		if (active.position >= sound.size()) mVoices.erase(mVoices.begin() + voice);
		else ++voice;
	}
}

bool SoundManager::isAvailable() const
{
	return mDevice != 0;
}
