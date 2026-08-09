#include "AiScoring.h"

#include <algorithm>

namespace
{
	const double SHIELD_VALUE = 6.0;
	const double MANA_CARD_VALUE = 2.0;
	const double MANA_CIVILIZATION_BONUS = 0.1;
	const double HAND_BASE_VALUE = 1.0;
	const double HAND_COST_BONUS = 0.25;
	const double HAND_MISSING_MANA_PENALTY = 0.5;
	const double DECK_CARD_VALUE = 0.05;

	int civilizationCount(int civilizations)
	{
		int count = 0;
		for (int civilization = CIV_LIGHT; civilization <= CIV_DARKNESS; ++civilization)
			if ((civilizations & (1 << civilization)) != 0) ++count;
		return count;
	}

	double manaValue(int cardCount, int civilizations)
	{
		return std::max(0, cardCount) * MANA_CARD_VALUE +
			civilizationCount(civilizations) * MANA_CIVILIZATION_BONUS;
	}
}

double AiScoring::battleCreatureValue(Duel& duel, int cardId)
{
	if (cardId < 0 || cardId >= static_cast<int>(duel.mCardList.size())) return 0.0;
	Card* card = duel.mCardList[cardId];
	if (card->mType != TYPE_CREATURE || card->mZone != ZONE_BATTLE) return 0.0;
	return std::max(0, duel.getCreaturePower(cardId)) / 1000.0 +
		std::max(0, duel.getCreatureBreaker(cardId)) * 2.0;
}

double AiScoring::manaZoneValue(const Duel& duel, int player)
{
	if (player != 0 && player != 1) return 0.0;
	int civilizations = 0;
	for (std::vector<Card*>::const_iterator card = duel.mManazones[player].mCards.begin();
		card != duel.mManazones[player].mCards.end(); ++card)
	{
		civilizations |= (*card)->mCivilizations;
	}
	return manaValue(static_cast<int>(duel.mManazones[player].mCards.size()), civilizations);
}

double AiScoring::handCardValue(const Card& card, int manaCount)
{
	int cost = std::max(0, card.mManaCost);
	int missingMana = std::max(0, cost - std::max(0, manaCount));
	double value = HAND_BASE_VALUE + cost * HAND_COST_BONUS -
		missingMana * HAND_MISSING_MANA_PENALTY;
	return std::max(HAND_BASE_VALUE, value);
}

double AiScoring::handZoneValue(const Duel& duel, int player, int manaCount)
{
	if (player != 0 && player != 1) return 0.0;
	double value = 0.0;
	for (std::vector<Card*>::const_iterator card = duel.mHands[player].mCards.begin();
		card != duel.mHands[player].mCards.end(); ++card)
	{
		value += handCardValue(**card, manaCount);
	}
	return value;
}

double AiScoring::playerValue(Duel& duel, int player)
{
	if (player != 0 && player != 1) return 0.0;
	ActiveDuelGuard activeGuard(duel);
	double value = duel.mShields[player].mCards.size() * SHIELD_VALUE;
	value += manaZoneValue(duel, player);
	value += handZoneValue(duel, player,
		static_cast<int>(duel.mManazones[player].mCards.size()));
	value += duel.mDecks[player].mCards.size() * DECK_CARD_VALUE;
	for (std::vector<Card*>::const_iterator card = duel.mBattlezones[player].mCards.begin();
		card != duel.mBattlezones[player].mCards.end(); ++card)
	{
		value += battleCreatureValue(duel, (*card)->mUniqueId);
	}
	return value;
}

double AiScoring::manaPlacementDelta(const Duel& duel, int player, int cardId)
{
	if ((player != 0 && player != 1) || cardId < 0 ||
		cardId >= static_cast<int>(duel.mCardList.size()))
		return -1000000.0;
	Card* candidate = duel.mCardList[cardId];
	if (candidate->mOwner != player || candidate->mZone != ZONE_HAND)
		return -1000000.0;

	int manaCount = static_cast<int>(duel.mManazones[player].mCards.size());
	int civilizations = 0;
	for (std::vector<Card*>::const_iterator mana = duel.mManazones[player].mCards.begin();
		mana != duel.mManazones[player].mCards.end(); ++mana)
	{
		civilizations |= (*mana)->mCivilizations;
	}
	double manaBefore = manaValue(manaCount, civilizations);
	double manaAfter = manaValue(manaCount + 1, civilizations | candidate->mCivilizations);

	double handBefore = handZoneValue(duel, player, manaCount);
	double handAfter = 0.0;
	for (std::vector<Card*>::const_iterator card = duel.mHands[player].mCards.begin();
		card != duel.mHands[player].mCards.end(); ++card)
	{
		if ((*card)->mUniqueId != cardId)
			handAfter += handCardValue(**card, manaCount + 1);
	}
	return (manaAfter - manaBefore) - (handBefore - handAfter);
}
