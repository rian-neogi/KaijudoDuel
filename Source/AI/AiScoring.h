#pragma once

#include "Game/Duel.h"

namespace AiScoring
{
	double battleCreatureValue(Duel& duel, int cardId);
	double manaZoneValue(const Duel& duel, int player);
	double handCardValue(const Card& card, int manaCount);
	double handZoneValue(const Duel& duel, int player, int manaCount);
	double playerValue(Duel& duel, int player);
	double manaPlacementDelta(const Duel& duel, int player, int cardId);
}
