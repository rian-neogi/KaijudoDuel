#include "OverworldAtmosphere.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
	const int MINUTES_PER_DAY = 24 * 60;
	const int STARTING_MINUTE = 8 * 60;
	const unsigned int MILLISECONDS_PER_GAME_MINUTE = 500;
	const unsigned int WEATHER_FADE_MILLISECONDS = 4000;

	float smoothStep(float value)
	{
		value = std::max(0.f, std::min(1.f, value));
		return value * value * (3.f - 2.f * value);
	}
}

OverworldAtmosphere::OverworldAtmosphere()
{
	reset();
}

void OverworldAtmosphere::reset()
{
	mDay = 1;
	mMinuteOfDay = STARTING_MINUTE;
	mMinuteAccumulator = 0;
	mWeather = WeatherKind::Clear;
	mWeatherIntensity = 0.f;
	mWeatherRemaining = 30000;
	mWeatherFadingOut = false;
	mWeatherSeed = 0x4b41494aU;
}

void OverworldAtmosphere::update(unsigned int deltaMilliseconds)
{
	const unsigned long long accumulated =
		(unsigned long long)mMinuteAccumulator + deltaMilliseconds;
	const int elapsedMinutes = (int)(accumulated / MILLISECONDS_PER_GAME_MINUTE);
	mMinuteAccumulator = (unsigned int)(accumulated % MILLISECONDS_PER_GAME_MINUTE);
	if (elapsedMinutes > 0)
	{
		const int totalMinutes = mMinuteOfDay + elapsedMinutes;
		mDay += totalMinutes / MINUTES_PER_DAY;
		mMinuteOfDay = totalMinutes % MINUTES_PER_DAY;
	}
	updateWeather(deltaMilliseconds);
}

void OverworldAtmosphere::restore(const OverworldAtmosphereState& saved)
{
	mDay = std::max(1, saved.day);
	mMinuteOfDay = std::max(0, std::min(MINUTES_PER_DAY - 1, saved.minuteOfDay));
	mMinuteAccumulator = 0;
	mWeather = saved.weather;
	mWeatherIntensity = std::max(0.f, std::min(1.f, saved.weatherIntensity));
	mWeatherRemaining = saved.weatherRemaining;
	mWeatherFadingOut = saved.weatherFadingOut;
	mWeatherSeed = saved.weatherSeed == 0 ? 0x4b41494aU : saved.weatherSeed;
	if (mWeather == WeatherKind::Clear)
	{
		mWeatherIntensity = 0.f;
		mWeatherFadingOut = false;
	}
}

OverworldAtmosphereState OverworldAtmosphere::state() const
{
	return { mDay, mMinuteOfDay, mWeather, mWeatherIntensity,
		mWeatherRemaining, mWeatherFadingOut, mWeatherSeed };
}

int OverworldAtmosphere::day() const
{
	return mDay;
}

int OverworldAtmosphere::minuteOfDay() const
{
	return mMinuteOfDay;
}

float OverworldAtmosphere::daylight() const
{
	const int dawnStart = 5 * 60 + 30;
	const int dayStart = 8 * 60;
	const int duskStart = 17 * 60 + 30;
	const int nightStart = 20 * 60 + 30;
	const float nightLight = 0.12f;
	if (mMinuteOfDay < dawnStart || mMinuteOfDay >= nightStart) return nightLight;
	if (mMinuteOfDay < dayStart)
	{
		const float progress = (mMinuteOfDay - dawnStart) /
			(float)(dayStart - dawnStart);
		return nightLight + (1.f - nightLight) * smoothStep(progress);
	}
	if (mMinuteOfDay < duskStart) return 1.f;
	const float progress = (mMinuteOfDay - duskStart) /
		(float)(nightStart - duskStart);
	return 1.f - (1.f - nightLight) * smoothStep(progress);
}

int OverworldAtmosphere::nightOverlayAlpha() const
{
	return (int)std::round((1.f - daylight()) * 165.f);
}

int OverworldAtmosphere::warmOverlayAlpha() const
{
	const int dawnStart = 5 * 60 + 30;
	const int dayStart = 8 * 60;
	const int duskStart = 17 * 60 + 30;
	const int nightStart = 20 * 60 + 30;
	float progress = -1.f;
	if (mMinuteOfDay >= dawnStart && mMinuteOfDay < dayStart)
		progress = (mMinuteOfDay - dawnStart) / (float)(dayStart - dawnStart);
	else if (mMinuteOfDay >= duskStart && mMinuteOfDay < nightStart)
		progress = (mMinuteOfDay - duskStart) / (float)(nightStart - duskStart);
	if (progress < 0.f) return 0;
	return (int)std::round(std::sin(progress * 3.14159265f) * 34.f);
}

WeatherKind OverworldAtmosphere::weather() const
{
	return mWeather;
}

float OverworldAtmosphere::weatherIntensity() const
{
	return mWeatherIntensity;
}

std::string OverworldAtmosphere::clockText() const
{
	std::ostringstream text;
	text << std::setfill('0') << std::setw(2) << mMinuteOfDay / 60 << ":"
		<< std::setw(2) << mMinuteOfDay % 60;
	return text.str();
}

const char* OverworldAtmosphere::weatherName(WeatherKind weather)
{
	if (weather == WeatherKind::Rain) return "Rain";
	if (weather == WeatherKind::Snow) return "Snow";
	return "Clear";
}

bool OverworldAtmosphere::parseWeather(const std::string& name, WeatherKind& weather)
{
	if (name == "clear") weather = WeatherKind::Clear;
	else if (name == "rain") weather = WeatherKind::Rain;
	else if (name == "snow") weather = WeatherKind::Snow;
	else return false;
	return true;
}

void OverworldAtmosphere::updateWeather(unsigned int deltaMilliseconds)
{
	unsigned int remainingDelta = deltaMilliseconds;
	do
	{
		if (mWeatherRemaining == 0) advanceWeatherState();
		if (remainingDelta == 0) break;
		const unsigned int step = std::min(remainingDelta, mWeatherRemaining);
		if (mWeather != WeatherKind::Clear)
		{
			const float change = step / (float)WEATHER_FADE_MILLISECONDS;
			mWeatherIntensity += mWeatherFadingOut ? -change : change;
			mWeatherIntensity = std::max(0.f, std::min(1.f, mWeatherIntensity));
		}
		mWeatherRemaining -= step;
		remainingDelta -= step;
	} while (remainingDelta > 0 || mWeatherRemaining == 0);
}

void OverworldAtmosphere::advanceWeatherState()
{
	if (mWeather == WeatherKind::Clear)
	{
		mWeather = nextRandom() % 3 == 0 ? WeatherKind::Snow : WeatherKind::Rain;
		mWeatherIntensity = 0.f;
		mWeatherFadingOut = false;
		mWeatherRemaining = 60000 + nextRandom() % 60001;
	}
	else if (!mWeatherFadingOut)
	{
		mWeatherFadingOut = true;
		mWeatherRemaining = WEATHER_FADE_MILLISECONDS;
	}
	else
	{
		mWeather = WeatherKind::Clear;
		mWeatherIntensity = 0.f;
		mWeatherFadingOut = false;
		mWeatherRemaining = 30000 + nextRandom() % 60001;
	}
}

unsigned int OverworldAtmosphere::nextRandom()
{
	mWeatherSeed = mWeatherSeed * 1664525U + 1013904223U;
	return mWeatherSeed;
}
