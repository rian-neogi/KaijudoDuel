#pragma once

#include <cstdint>
#include <string>

struct AiMatchResult
{
	bool started;
	bool completed;
	bool stalled;
	int winner;
	int actions;
	long long elapsedMs;
	std::string error;

	AiMatchResult();
};

// Runs one AI-vs-AI duel without initializing SDL or creating a window.
// maxActions prevents broken card interactions from hanging automated runs.
AiMatchResult runHeadlessAiMatch(const std::string& deck0, const std::string& deck1,
	std::uint32_t seed, int maxActions, const std::string& personality = "tempo",
	const std::string& difficulty = "medium");
