#pragma once

#include "Game/Duel.h"

namespace AiScoring
{
	double battleCreatureValue(Duel& duel, int cardId);
	double shieldZoneValue(int shieldCount);
	double manaZoneValue(const Duel& duel, int player);
	double handCardValue(const Card& card, int manaCount);
	double handZoneValue(const Duel& duel, int player, int manaCount);
	// Shield-trigger target selection probes the attacker's remaining board while
	// the attack/cast is transient; evaluation callers keep the stable-state guard.
	bool hasKnockout(Duel& duel, int player, bool requireStableState = true);
	double playerValue(Duel& duel, int player);
	double manaPlacementDelta(const Duel& duel, int player, int cardId);
}
