#pragma once

#include "Game/Duel.h"

#include <string>

class HeuristicBot
{
public:
	explicit HeuristicBot(int player, const std::string& personality = "tempo");

	Message chooseMove(Duel& duel, const std::vector<Message>& moves) const;
	double scoreMove(Duel& duel, const Message& move) const;
	bool chooseManaPlacement(Duel& duel, const std::vector<Message>& moves,
		Message& result) const;
	int chooseManaPayment(Duel& duel, const std::vector<int>& options) const;

private:
	int mPlayer;
	std::string mPersonality;

	double cardValue(Duel& duel, int cardId, bool allowLuaQueries) const;
	double scoreChoice(Duel& duel, int selection) const;
	double scoreManaCharge(Duel& duel, int cardId) const;
	double scoreManaPayment(Duel& duel, int cardId) const;
	double scoreAttack(Duel& duel, const Message& move) const;
	double scoreBlock(Duel& duel, int blocker) const;
	int attackingPower(Duel& duel, int attacker) const;
};
