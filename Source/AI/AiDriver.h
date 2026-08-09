#pragma once

#include "Mcts.h"

#include <memory>
#include <string>

enum class AiDecisionSource
{
	None,
	Forced,
	ManaHeuristic,
	ShieldRandom,
	Mcts,
	Heuristic
};

struct AiDecisionOutcome
{
	AiDecisionSource source;
	Message action;

	AiDecisionOutcome();
};

// The initial live budget is deliberately bounded. BackgroundMctsSearch runs
// it on a worker after the live duel hands that worker exclusive Lua ownership.
MctsConfig liveMctsConfig();

// Chooses and queues one complete MCTS decision. If search is unavailable or
// cannot commit a plan, queues one legal HeuristicBot action instead. The
// caller must hold gMutex.
AiDecisionOutcome playAiDecision(Duel& duel, int player,
	const std::string& personality, const MctsConfig& config);

// Queues one legal fallback action without starting another search.
AiDecisionOutcome playHeuristicDecision(Duel& duel, int player,
	const std::string& personality);

// Queues the move immediately when the acting player has exactly one legal
// engine action. Returns None without changing the duel otherwise.
AiDecisionOutcome playForcedAiDecision(Duel& duel, int player);

// Charges one card when the mana-placement heuristic recommends doing so.
// Returns None when placement should be skipped or is not currently available.
AiDecisionOutcome playHeuristicManaPlacement(Duel& duel, int player,
	const std::string& personality);

// Completes an in-progress spell payment using the heuristic mana selector.
AiDecisionOutcome playHeuristicManaPayment(Duel& duel, int player,
	const std::string& personality);

// Selects one legal shield target uniformly through the Duel RNG.
AiDecisionOutcome playRandomShieldTarget(Duel& duel, int player);

class BackgroundMctsSearch
{
public:
	BackgroundMctsSearch(int player, const MctsConfig& config);
	~BackgroundMctsSearch();

	BackgroundMctsSearch(const BackgroundMctsSearch&) = delete;
	BackgroundMctsSearch& operator=(const BackgroundMctsSearch&) = delete;

	// start() captures the root and must be called with gMutex held at a stable
	// boundary. Until finish() or cancelAndWait(), the caller must prevent every
	// other use of LuaCards and ActiveDuel.
	bool start(Duel& root);
	bool isFinished() const;
	bool finish(MctsResult& result);
	void cancelAndWait();

private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};
