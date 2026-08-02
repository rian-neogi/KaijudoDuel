#pragma once

#include "Game/Duel.h"

class HeuristicBot
{
public:
	explicit HeuristicBot(int player);

	Message chooseMove(Duel& duel, const std::vector<Message>& moves) const;
	double scoreMove(Duel& duel, const Message& move) const;

private:
	int mPlayer;

	double cardValue(Duel& duel, int cardId, bool allowLuaQueries) const;
	double scoreChoice(Duel& duel, int selection) const;
	double scoreManaCharge(Duel& duel, int cardId) const;
	double scoreAttack(Duel& duel, const Message& move) const;
	double scoreBlock(Duel& duel, int blocker) const;
	int attackingPower(Duel& duel, int attacker) const;
	bool hasStrongerBlocker(Duel& duel, int attacker, int attackerPower, int attackedCreature) const;
};
