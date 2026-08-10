#pragma once

#include <chrono>

namespace MctsTiming
{
	class SearchScope
	{
	public:
		explicit SearchScope(long long& accumulator);
		~SearchScope();

		SearchScope(const SearchScope&) = delete;
		SearchScope& operator=(const SearchScope&) = delete;

	private:
		long long* mPreviousAccumulator;
		int mPreviousDepth;
	};

	class LuaCallbackTimer
	{
	public:
		LuaCallbackTimer();
		~LuaCallbackTimer();

		LuaCallbackTimer(const LuaCallbackTimer&) = delete;
		LuaCallbackTimer& operator=(const LuaCallbackTimer&) = delete;

	private:
		long long* mAccumulator;
		bool mOutermost;
		std::chrono::steady_clock::time_point mStarted;
	};
}
