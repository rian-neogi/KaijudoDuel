#include "AiScoring.h"

#include "AiParams.h"

#include <algorithm>

namespace
{
	int civilizationCount(int civilizations)
	{
		int count = 0;
		for (int civilization = CIV_LIGHT; civilization <= CIV_DARKNESS; ++civilization)
			if ((civilizations & (1 << civilization)) != 0) ++count;
		return count;
	}

	double manaValue(int cardCount, int civilizations)
	{
		return std::max(0, cardCount) * aiParam("evaluation.mana_card_value") +
			civilizationCount(civilizations) *
				aiParam("evaluation.mana_civilization_bonus");
	}
}

double AiScoring::battleCreatureValue(Duel& duel, int cardId)
{
	if (cardId < 0 || cardId >= static_cast<int>(duel.mCardList.size())) return 0.0;
	Card* card = duel.mCardList[cardId];
	if (card->mType != TYPE_CREATURE || card->mZone != ZONE_BATTLE) return 0.0;
	return std::max(0, duel.getCreaturePower(cardId)) /
			aiParam("evaluation.creature_power_divisor") +
		std::max(0, duel.getCreatureBreaker(cardId)) *
			aiParam("evaluation.creature_breaker_value");
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
	double baseValue = aiParam("evaluation.hand_base_value");
	double value = baseValue + cost * aiParam("evaluation.hand_cost_bonus") -
		missingMana * aiParam("evaluation.hand_missing_mana_penalty");
	return std::max(baseValue, value);
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
	double value = duel.mShields[player].mCards.size() *
		aiParam("evaluation.shield_value");
	value += manaZoneValue(duel, player);
	value += handZoneValue(duel, player,
		static_cast<int>(duel.mManazones[player].mCards.size()));
	value += duel.mDecks[player].mCards.size() * aiParam("evaluation.deck_card_value");
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
		return aiParam("evaluation.invalid_mana_delta");
	Card* candidate = duel.mCardList[cardId];
	if (candidate->mOwner != player || candidate->mZone != ZONE_HAND)
		return aiParam("evaluation.invalid_mana_delta");

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
