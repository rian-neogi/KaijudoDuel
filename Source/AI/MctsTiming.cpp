#include "MctsTiming.h"

namespace
{
	thread_local long long* gLuaAccumulator = nullptr;
	thread_local int gLuaCallbackDepth = 0;
}

MctsTiming::SearchScope::SearchScope(long long& accumulator)
	: mPreviousAccumulator(gLuaAccumulator), mPreviousDepth(gLuaCallbackDepth)
{
	gLuaAccumulator = &accumulator;
	gLuaCallbackDepth = 0;
}

MctsTiming::SearchScope::~SearchScope()
{
	gLuaAccumulator = mPreviousAccumulator;
	gLuaCallbackDepth = mPreviousDepth;
}

MctsTiming::LuaCallbackTimer::LuaCallbackTimer()
	: mAccumulator(gLuaAccumulator), mOutermost(false)
{
	if (mAccumulator == nullptr) return;
	mOutermost = gLuaCallbackDepth == 0;
	gLuaCallbackDepth++;
	if (mOutermost) mStarted = std::chrono::steady_clock::now();
}

MctsTiming::LuaCallbackTimer::~LuaCallbackTimer()
{
	if (mAccumulator == nullptr) return;
	gLuaCallbackDepth--;
	if (mOutermost)
	{
		*mAccumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now() - mStarted).count();
	}
}
