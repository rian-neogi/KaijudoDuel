#pragma once

#include <string>

enum class WeatherKind
{
	Clear,
	Rain,
	Snow
};

struct OverworldAtmosphereState
{
	int day;
	int minuteOfDay;
	WeatherKind weather;
	float weatherIntensity;
	unsigned int weatherRemaining;
	bool weatherFadingOut;
	unsigned int weatherSeed;
};

class OverworldAtmosphere
{
public:
	OverworldAtmosphere();

	void reset();
	void update(unsigned int deltaMilliseconds);
	void restore(const OverworldAtmosphereState& state);
	OverworldAtmosphereState state() const;

	int day() const;
	int minuteOfDay() const;
	float daylight() const;
	int nightOverlayAlpha() const;
	int warmOverlayAlpha() const;
	WeatherKind weather() const;
	float weatherIntensity() const;
	std::string clockText() const;

	static const char* weatherName(WeatherKind weather);
	static bool parseWeather(const std::string& name, WeatherKind& weather);

private:
	void updateWeather(unsigned int deltaMilliseconds);
	void advanceWeatherState();
	unsigned int nextRandom();

	int mDay;
	int mMinuteOfDay;
	unsigned int mMinuteAccumulator;
	WeatherKind mWeather;
	float mWeatherIntensity;
	unsigned int mWeatherRemaining;
	bool mWeatherFadingOut;
	unsigned int mWeatherSeed;
};
