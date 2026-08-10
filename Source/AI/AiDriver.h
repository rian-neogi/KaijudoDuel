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
	ShieldTriggerHeuristic,
	Mcts,
	Heuristic
};

struct AiDecisionOutcome
{
	AiDecisionSource source;
	Message action;

	AiDecisionOutcome();
};

// The live rollout ceiling is still bounded by a wall-clock budget. Combat
// decisions receive extra time because their attacker/target branches are wider.
MctsConfig liveMctsConfig(bool combatPhase = false);

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

// Charges the legal hand card with the largest mana-and-hand evaluation delta.
// Returns None when mana placement is not currently available.
AiDecisionOutcome playHeuristicManaPlacement(Duel& duel, int player,
	const std::string& personality);

// Completes an in-progress spell payment using the heuristic mana selector.
AiDecisionOutcome playHeuristicManaPayment(Duel& duel, int player,
	const std::string& personality);

// Selects one legal shield target uniformly through the Duel RNG.
AiDecisionOutcome playRandomShieldTarget(Duel& duel, int player);

// Resolves shield-trigger use/skip decisions without MCTS. Creature triggers
// are always used. Spell triggers are used only when their Lua AI-cast probe
// approves them. Choices made while a trigger resolves prefer the highest-value
// valid opposing creature.
AiDecisionOutcome playHeuristicShieldTrigger(Duel& duel, int player,
	const std::string& personality);

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
	bool start(Duel& root, const MctsConfig& config);
	bool isActive() const;
	bool isFinished() const;
	bool finish(MctsResult& result);
	void cancelAndWait();

private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};
