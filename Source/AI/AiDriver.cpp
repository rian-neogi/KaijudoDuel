#include "AiDriver.h"

#include "AiParams.h"
#include "HeuristicBot.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <iostream>
#include <limits>
#include <mutex>
#include <thread>

namespace
{
	std::mutex gBackgroundLuaMutex;
}

AiDecisionOutcome::AiDecisionOutcome() : source(AiDecisionSource::None)
{
}

MctsConfig liveMctsConfig(bool combatPhase, const std::string& difficulty,
	const std::string& personality)
{
	MctsConfig config(personality);
	config.timeBudgetMs = aiDifficultyIntParam(difficulty, combatPhase ?
		"combat_time_budget_ms" : "main_time_budget_ms");
	return config;
}

AiDecisionOutcome playAiDecision(Duel& duel, int player,
	const std::string& personality, const MctsConfig& config)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player)
		return outcome;
	AiDecisionOutcome shieldTrigger = playHeuristicShieldTrigger(
		duel, player, personality);
	if (shieldTrigger.source == AiDecisionSource::ShieldTriggerHeuristic)
		return shieldTrigger;
	AiDecisionOutcome shieldTarget = playRandomShieldTarget(duel, player);
	if (shieldTarget.source == AiDecisionSource::ShieldRandom)
		return shieldTarget;
	AiDecisionOutcome manaPayment = playHeuristicManaPayment(duel, player, personality);
	if (manaPayment.source == AiDecisionSource::ManaHeuristic)
		return manaPayment;
	AiDecisionOutcome manaPlacement = playHeuristicManaPlacement(
		duel, player, personality);
	if (manaPlacement.source == AiDecisionSource::ManaHeuristic)
		return manaPlacement;
	AiDecisionOutcome forced = playForcedAiDecision(duel, player);
	if (forced.source == AiDecisionSource::Forced) return forced;

	if (duel.isCloneable())
	{
		MctsConfig personalizedConfig = config;
		personalizedConfig.personality = personality;
		MctsSearch search(player, personalizedConfig);
		MctsResult result = search.search(duel);
		if (result.hasPlan &&
			commitDecisionPlan(duel, result.plan) == DecisionPlanCommitStatus::Committed)
		{
			outcome.source = AiDecisionSource::Mcts;
			outcome.action = result.plan.action;
			return outcome;
		}
	}

	return playHeuristicDecision(duel, player, personality);
}

AiDecisionOutcome playHeuristicDecision(Duel& duel, int player,
	const std::string& personality)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player)
		return outcome;
	std::vector<Message> moves = duel.getPossibleMoves();
	if (moves.empty()) return outcome;
	HeuristicBot fallback(player, personality);
	outcome.action = fallback.chooseMove(duel, moves);
	duel.handleInterfaceInput(outcome.action);
	outcome.source = AiDecisionSource::Heuristic;
	return outcome;
}

AiDecisionOutcome playForcedAiDecision(Duel& duel, int player)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player)
		return outcome;
	std::vector<Message> moves = duel.getPossibleMoves();
	if (moves.size() != 1) return outcome;
	outcome.source = AiDecisionSource::Forced;
	outcome.action = moves.front();
	duel.handleInterfaceInput(outcome.action);
	return outcome;
}

AiDecisionOutcome playHeuristicManaPlacement(Duel& duel, int player,
	const std::string& personality)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player)
		return outcome;
	std::vector<Message> moves = duel.getPossibleMoves();
	HeuristicBot bot(player, personality);
	Message placement;
	if (!bot.chooseManaPlacement(duel, moves, placement)) return outcome;
	outcome.source = AiDecisionSource::ManaHeuristic;
	outcome.action = placement;
	duel.handleInterfaceInput(outcome.action);
	return outcome;
}

AiDecisionOutcome playHeuristicManaPayment(Duel& duel, int player,
	const std::string& personality)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player || duel.mCastingCard == -1)
		return outcome;
	HeuristicBot bot(player, personality);
	for (int safety = 0; duel.mCastingCard != -1 &&
		safety < aiIntParam("search.max_mana_payment_steps", personality); ++safety)
	{
		std::vector<Message> moves = duel.getPossibleMoves();
		std::vector<int> options;
		for (std::vector<Message>::iterator move = moves.begin(); move != moves.end(); ++move)
			if (move->getType() == "manatap") options.push_back(move->getInt("card"));
		int selected = bot.chooseManaPayment(duel, options);
		std::vector<Message>::iterator payment = moves.end();
		for (std::vector<Message>::iterator move = moves.begin(); move != moves.end(); ++move)
		{
			if (move->getType() == "manatap" && move->getInt("card") == selected)
			{
				payment = move;
				break;
			}
		}
		if (payment == moves.end()) return outcome;
		if (outcome.source == AiDecisionSource::None) outcome.action = *payment;
		Message action = *payment;
		duel.handleInterfaceInput(action);
		outcome.source = AiDecisionSource::ManaHeuristic;
	}
	return outcome;
}

AiDecisionOutcome playRandomShieldTarget(Duel& duel, int player)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player || duel.mAttackphase != PHASE_TARGET)
		return outcome;
	std::vector<Message> moves = duel.getPossibleMoves();
	std::vector<Message> targets;
	for (std::vector<Message>::iterator move = moves.begin(); move != moves.end(); ++move)
		if (move->getType() == "targetshield") targets.push_back(*move);
	if (targets.empty()) return outcome;
	outcome.source = AiDecisionSource::ShieldRandom;
	outcome.action = targets[duel.mRandomGen.Random((unsigned int)targets.size())];
	duel.handleInterfaceInput(outcome.action);
	return outcome;
}

AiDecisionOutcome playHeuristicShieldTrigger(Duel& duel, int player,
	const std::string& personality)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player || duel.mAttackphase != PHASE_TRIGGER)
		return outcome;

	ActiveDuelGuard activeGuard(duel);
	std::vector<Message> moves = duel.getPossibleMoves();
	if (moves.empty()) return outcome;
	HeuristicBot bot(player, personality);

	if (duel.mIsChoiceActive)
	{
		std::vector<Message>::iterator best = moves.end();
		double bestScore = -std::numeric_limits<double>::infinity();
		for (std::vector<Message>::iterator move = moves.begin();
			move != moves.end(); ++move)
		{
			if (move->getType() != "choiceselect") continue;
			int selection = move->getInt("selection");
			if (selection < 0 || selection >= static_cast<int>(duel.mCardList.size())) continue;
			Card* target = duel.mCardList[selection];
			if (target->mOwner == player || target->mZone != ZONE_BATTLE ||
				target->mType != TYPE_CREATURE)
				continue;
			double score = bot.scoreMove(duel, *move);
			if (best == moves.end() || score > bestScore)
			{
				best = move;
				bestScore = score;
			}
		}
		outcome.action = best == moves.end() ? bot.chooseMove(duel, moves) : *best;
	}
	else
	{
		// Creature triggers are unconditional. Prefer them before spells so every
		// creature broken by a multi-breaker enters the battle zone.
		for (std::vector<Message>::iterator move = moves.begin();
			move != moves.end(); ++move)
		{
			if (move->getType() != "triggeruse") continue;
			int trigger = move->getInt("trigger");
			if (trigger >= 0 && trigger < static_cast<int>(duel.mCardList.size()) &&
				duel.mCardList[trigger]->mType == TYPE_CREATURE)
			{
				outcome.action = *move;
				break;
			}
		}

		if (outcome.action.getType().empty())
		{
			for (std::vector<Message>::iterator move = moves.begin();
				move != moves.end(); ++move)
			{
				if (move->getType() != "triggeruse") continue;
				int trigger = move->getInt("trigger");
				if (trigger >= 0 && trigger < static_cast<int>(duel.mCardList.size()) &&
					duel.mCardList[trigger]->mType == TYPE_SPELL &&
					duel.getCardAiCanCast(trigger) == 1)
				{
					outcome.action = *move;
					break;
				}
			}
		}

		if (outcome.action.getType().empty())
		{
			for (std::vector<Message>::iterator move = moves.begin();
				move != moves.end(); ++move)
			{
				if (move->getType() == "triggerskip")
				{
					outcome.action = *move;
					break;
				}
			}
		}
	}

	if (outcome.action.getType().empty()) return outcome;
	Message action = outcome.action;
	duel.handleInterfaceInput(action);
	outcome.source = AiDecisionSource::ShieldTriggerHeuristic;
	return outcome;
}

struct BackgroundMctsSearch::Impl
{
	int player;
	MctsConfig config;
	std::unique_ptr<MctsSession> session;
	std::thread worker;
	std::atomic<bool> cancel;
	std::atomic<bool> active;
	std::atomic<bool> finished;
	MctsResult result;

	Impl(int rootPlayer, const MctsConfig& searchConfig)
		: player(rootPlayer), config(searchConfig),
		  session(new MctsSession(rootPlayer, searchConfig)), cancel(false),
		  active(false), finished(false)
	{
	}
};

BackgroundMctsSearch::BackgroundMctsSearch(int player, const MctsConfig& config)
	: mImpl(new Impl(player, config))
{
}

BackgroundMctsSearch::~BackgroundMctsSearch()
{
	cancelAndWait();
}

bool BackgroundMctsSearch::start(Duel& root)
{
	return start(root, mImpl->config);
}

bool BackgroundMctsSearch::start(Duel& root, const MctsConfig& config)
{
	if (mImpl->worker.joinable() || mImpl->active.load()) return false;
	bool started = false;
	if (mImpl->session->isStarted())
		started = mImpl->session->restart(root, config);
	else
	{
		mImpl->session.reset(new MctsSession(mImpl->player, config));
		started = mImpl->session->start(root);
	}
	if (!started)
	{
		mImpl->session.reset(new MctsSession(mImpl->player, config));
		if (!mImpl->session->start(root)) return false;
	}
	mImpl->config = config;
	mImpl->result = MctsResult();
	mImpl->cancel.store(false);
	mImpl->finished.store(false);
	mImpl->active.store(true);
	mImpl->worker = std::thread(
		[this]()
		{
			try
			{
				std::lock_guard<std::mutex> luaLock(gBackgroundLuaMutex);
				while (!mImpl->cancel.load() && !mImpl->session->isComplete())
					mImpl->session->advance(1);
				if (!mImpl->cancel.load() && mImpl->session->isComplete())
					mImpl->result = mImpl->session->result();
			}
			catch (const std::exception& error)
			{
				std::cerr << "Background MCTS search failed: " << error.what() << std::endl;
			}
			catch (...)
			{
				std::cerr << "Background MCTS search failed with an unknown exception." << std::endl;
			}
			mImpl->finished.store(true);
		});
	return true;
}

bool BackgroundMctsSearch::isActive() const
{
	return mImpl->active.load();
}

bool BackgroundMctsSearch::isFinished() const
{
	return mImpl->finished.load();
}

bool BackgroundMctsSearch::finish(MctsResult& result)
{
	if (!mImpl->active.load() || !mImpl->finished.load()) return false;
	if (mImpl->worker.joinable()) mImpl->worker.join();
	result = mImpl->result;
	mImpl->active.store(false);
	return true;
}

void BackgroundMctsSearch::cancelAndWait()
{
	mImpl->cancel.store(true);
	if (mImpl->worker.joinable()) mImpl->worker.join();
	mImpl->active.store(false);
}
