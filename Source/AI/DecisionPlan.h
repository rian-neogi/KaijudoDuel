#pragma once

#include "Game/Duel.h"

#include <functional>
#include <vector>

struct DecisionChoice
{
	int player;
	int selection;

	DecisionChoice();
	DecisionChoice(int choicePlayer, int choiceSelection);
	bool operator==(const DecisionChoice& other) const;
};

struct DecisionPlan
{
	int player;
	Message action;
	std::vector<int> manaCards;
	std::vector<DecisionChoice> choices;

	DecisionPlan();
	DecisionPlan(int actingPlayer, const Message& primaryAction);
	bool operator==(const DecisionPlan& other) const;
};

enum class DecisionPlanStatus
{
	Complete,
	NeedsMana,
	NeedsChoice,
	Illegal
};

enum class DecisionPlanCommitStatus
{
	Committed,
	Illegal
};

struct DecisionPlanResult
{
	DecisionPlanStatus status;
	int choicePlayer;
	int aiPreferredChoice;
	std::vector<int> options;

	DecisionPlanResult();
};

struct DecisionPlanEnumerationOptions
{
	// MCTS uses one heuristic mana placement/payment path. The default remains
	// exhaustive for engine tests and callers that require every complete plan.
	bool heuristicMana;
	// Optional Lua advice may suppress strategically empty card plays for AI
	// planning without changing whether the card is legally castable.
	bool heuristicCardPlay;
	// A Lua choice may supply one strategic preference. MCTS follows only those
	// explicit preferences; choices without one retain their full branching.
	bool heuristicChoices;
	// Rollout policy samples one legal answer at each ordered choice instead of
	// enumerating every complete choice path before selecting one uniformly.
	// Tree-node enumeration leaves this disabled so strategic choices still branch.
	bool randomChoices;
	// MCTS treats shield targeting as a random policy rather than a tree branch.
	bool randomShieldTarget;
	std::function<size_t(size_t)> randomIndex;
	// Optional deadline/cancellation predicate checked between recursive plans.
	std::function<bool()> shouldStop;

	DecisionPlanEnumerationOptions();
};

// Both functions may enter Lua. The caller must serialize access to the Duel,
// LuaCards, and ActiveDuel and provide a stable, cloneable decision boundary.
// Execution is simulation-only.
DecisionPlanResult executeDecisionPlan(Duel& duel, const DecisionPlan& plan);
std::vector<DecisionPlan> enumerateDecisionPlans(Duel& root);
std::vector<DecisionPlan> enumerateDecisionPlans(Duel& root,
	const DecisionPlanEnumerationOptions& options);

// Preflights the complete plan on a clone, then queues its live primary action
// and mana payment. Leading choices owned by plan.player are answered from the
// plan; a choice owned by another player returns to normal interactive handling.
DecisionPlanCommitStatus commitDecisionPlan(Duel& duel, const DecisionPlan& plan);
