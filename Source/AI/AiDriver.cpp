#include "AiDriver.h"

#include "HeuristicBot.h"

#include <atomic>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>

namespace
{
	std::mutex gBackgroundLuaMutex;
}

AiDecisionOutcome::AiDecisionOutcome() : source(AiDecisionSource::None)
{
}

MctsConfig liveMctsConfig(bool combatPhase)
{
	MctsConfig config;
	config.iterations = 1024;
	config.maxDepth = 12;
	config.timeBudgetMs = combatPhase ? 2500 : 1500;
	return config;
}

AiDecisionOutcome playAiDecision(Duel& duel, int player,
	const std::string& personality, const MctsConfig& config)
{
	AiDecisionOutcome outcome;
	if ((player != 0 && player != 1) || duel.mWinner != -1 ||
		duel.getPlayerToMove() != player)
		return outcome;
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
		MctsSearch search(player, config);
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
	for (int safety = 0; duel.mCastingCard != -1 && safety < 40; ++safety)
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

struct BackgroundMctsSearch::Impl
{
	int player;
	MctsConfig config;
	std::thread worker;
	std::atomic<bool> cancel;
	std::atomic<bool> finished;
	MctsResult result;

	Impl(int rootPlayer, const MctsConfig& searchConfig)
		: player(rootPlayer), config(searchConfig), cancel(false), finished(false)
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
	if (mImpl->worker.joinable() || mImpl->finished.load()) return false;
	MctsSession* session = new MctsSession(mImpl->player, mImpl->config);
	if (!session->start(root))
	{
		delete session;
		return false;
	}
	mImpl->worker = std::thread(
		[this, session]()
		{
			try
			{
				std::lock_guard<std::mutex> luaLock(gBackgroundLuaMutex);
				while (!mImpl->cancel.load() && !session->isComplete())
					session->advance(1);
				if (!mImpl->cancel.load() && session->isComplete())
					mImpl->result = session->result();
			}
			catch (const std::exception& error)
			{
				std::cerr << "Background MCTS search failed: " << error.what() << std::endl;
			}
			catch (...)
			{
				std::cerr << "Background MCTS search failed with an unknown exception." << std::endl;
			}
			delete session;
			mImpl->finished.store(true);
		});
	return true;
}

bool BackgroundMctsSearch::isFinished() const
{
	return mImpl->finished.load();
}

bool BackgroundMctsSearch::finish(MctsResult& result)
{
	if (!mImpl->finished.load()) return false;
	if (mImpl->worker.joinable()) mImpl->worker.join();
	result = mImpl->result;
	return true;
}

void BackgroundMctsSearch::cancelAndWait()
{
	mImpl->cancel.store(true);
	if (mImpl->worker.joinable()) mImpl->worker.join();
}
