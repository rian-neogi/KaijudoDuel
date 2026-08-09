#pragma once

#include "DecisionPlan.h"

#include <cstdint>
#include <memory>
#include <vector>

struct MctsConfig
{
	int iterations;
	int maxDepth;
	// Zero disables the deadline. Live play uses a positive soft wall-clock
	// budget; a single in-progress Lua callback is allowed to return normally.
	int timeBudgetMs;
	double exploration;
	std::uint32_t seed;

	MctsConfig();
};

struct MctsChildStatistics
{
	DecisionPlan plan;
	int visits;
	double meanValue;

	MctsChildStatistics();
};

struct MctsResult
{
	bool hasPlan;
	DecisionPlan plan;
	int iterationsCompleted;
	int failedIterations;
	bool timeBudgetExpired;
	int turnHorizonCutoffs;
	int forcedMovesApplied;
	double meanValue;
	int selectedVisits;
	double selectedMeanValue;
	std::vector<MctsChildStatistics> rootChildren;

	MctsResult();
};

class MctsSession
{
public:
	MctsSession(int rootPlayer, const MctsConfig& config = MctsConfig());
	~MctsSession();

	MctsSession(const MctsSession&) = delete;
	MctsSession& operator=(const MctsSession&) = delete;

	// Captures a stable root and preserves its tree between advance() calls.
	// start() must serialize access to the source Duel. advance() may enter Lua,
	// so its caller must own exclusive access to LuaCards and ActiveDuel.
	bool start(Duel& root);
	bool advance(int iterationBudget);
	bool isStarted() const;
	bool isComplete() const;
	MctsResult result() const;

private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};

class MctsSearch
{
public:
	MctsSearch(int rootPlayer, const MctsConfig& config = MctsConfig());

	// May enter Lua. The caller must own exclusive access to LuaCards and
	// ActiveDuel and supply a stable, cloneable boundary owned by rootPlayer.
	MctsResult search(Duel& root);

private:
	int mRootPlayer;
	MctsConfig mConfig;
};
