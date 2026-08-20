#include "AiMatchRunner.h"

#include "AiDriver.h"
#include "Game/Duel.h"

#include <chrono>
#include <thread>

AiMatchResult::AiMatchResult()
	: started(false), completed(false), stalled(false), winner(-1), actions(0),
	  elapsedMs(0)
{
}

AiMatchResult runHeadlessAiMatch(const std::string& deck0, const std::string& deck1,
	std::uint32_t seed, int maxActions, const std::string& personality,
	const std::string& difficulty)
{
	AiMatchResult result;
	Duel duel;
	ActiveDuelGuard activeGuard(duel);
	duel.mRandomGen.SetRandomSeed(seed);
	duel.mPlayerType[0] = PLAYER_AI;
	duel.mPlayerType[1] = PLAYER_AI;
	duel.mAiPersonality[0] = personality;
	duel.mAiPersonality[1] = personality;
	if (!duel.setDecks(deck0, deck1))
	{
		result.error = "unable to load one or both decks";
		return result;
	}

	result.started = true;
	duel.startDuel();
	std::thread inputThread(&Duel::loopInput, &duel);
	std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
	int consecutiveStalls = 0;
	const int stallLimit = 100;

	while (result.actions < maxActions)
	{
		bool shouldWait = true;
		{
			std::lock_guard<std::mutex> lock(gMutex);
			if (duel.mWinner != -1)
			{
				result.completed = true;
				result.winner = duel.mWinner;
				break;
			}
			if (!duel.mMsgMngr.hasMoreMessages())
			{
				int player = duel.getPlayerToMove();
				MctsConfig config = liveMctsConfig(
					duel.mTurnPhase == TURN_PHASE_ATTACK, difficulty, personality);
				AiDecisionOutcome decision = playAiDecision(
					duel, player, personality, config);
				if (decision.source == AiDecisionSource::None)
				{
					consecutiveStalls++;
					if (consecutiveStalls >= stallLimit)
					{
						result.stalled = true;
						result.error = "AI produced no action at a stable decision boundary";
						break;
					}
				}
				else
				{
					consecutiveStalls = 0;
					result.actions = duel.mCurrentMoveCount;
				}
				shouldWait = false;
			}
		}
		if (result.completed || result.stalled) break;
		if (shouldWait)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		else
			std::this_thread::yield();
	}

	duel.stopInputLoop();
	if (inputThread.joinable()) inputThread.join();
	if (!result.completed && !result.stalled)
	{
		std::lock_guard<std::mutex> lock(gMutex);
		if (duel.mWinner != -1)
		{
			result.completed = true;
			result.winner = duel.mWinner;
		}
	}
	result.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - began).count();
	return result;
}
