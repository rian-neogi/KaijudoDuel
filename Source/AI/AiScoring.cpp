#include "AiScoring.h"

#include "AiParams.h"

#include <algorithm>
#include <string>

namespace
{
	struct KnockoutAttacker
	{
		int cardId;
		int breaker;
		std::vector<int> blockerSlots;
	};

	struct BlockAssignment
	{
		std::vector<bool> blockedAttackers;
		int blockedCount;
	};

	int civilizationCount(int civilizations)
	{
		if ((civilizations & (1 << CIV_HOLLOW)) != 0)
			return CIV_HOLLOW + 1;
		int count = 0;
		for (int civilization = CIV_LIGHT; civilization <= CIV_HOLLOW; ++civilization)
			if ((civilizations & (1 << civilization)) != 0) ++count;
		return count;
	}

	double manaValue(int cardCount, int civilizations)
	{
		return std::max(0, cardCount) * aiParam("evaluation.mana_card_value") +
			civilizationCount(civilizations) *
				aiParam("evaluation.mana_civilization_bonus");
	}

	bool assignBlocker(int attackerIndex, const std::vector<KnockoutAttacker>& attackers,
		std::vector<int>& blockerMatches, std::vector<bool>& visitedBlockers)
	{
		for (std::vector<int>::const_iterator blocker =
			attackers[attackerIndex].blockerSlots.begin();
			blocker != attackers[attackerIndex].blockerSlots.end(); ++blocker)
		{
			if (visitedBlockers[*blocker]) continue;
			visitedBlockers[*blocker] = true;
			int previousAttacker = blockerMatches[*blocker];
			if (previousAttacker < 0 || assignBlocker(previousAttacker, attackers,
				blockerMatches, visitedBlockers))
			{
				blockerMatches[*blocker] = attackerIndex;
				return true;
			}
		}
		return false;
	}

	BlockAssignment optimalBlockAssignment(const std::vector<KnockoutAttacker>& attackers,
		int blockerCount, int forcedUnblockedAttacker)
	{
		std::vector<int> order;
		for (size_t attacker = 0; attacker < attackers.size(); ++attacker)
			if (static_cast<int>(attacker) != forcedUnblockedAttacker)
				order.push_back(static_cast<int>(attacker));
		std::sort(order.begin(), order.end(),
			[&attackers](int left, int right)
			{
				if (attackers[left].breaker != attackers[right].breaker)
					return attackers[left].breaker > attackers[right].breaker;
				return attackers[left].cardId < attackers[right].cardId;
			});

		std::vector<int> blockerMatches(std::max(0, blockerCount), -1);
		for (std::vector<int>::const_iterator attacker = order.begin();
			attacker != order.end(); ++attacker)
		{
			std::vector<bool> visitedBlockers(blockerMatches.size(), false);
			assignBlocker(*attacker, attackers, blockerMatches, visitedBlockers);
		}

		BlockAssignment result;
		result.blockedAttackers.assign(attackers.size(), false);
		result.blockedCount = 0;
		for (std::vector<int>::const_iterator attacker = blockerMatches.begin();
			attacker != blockerMatches.end(); ++attacker)
		{
			if (*attacker < 0) continue;
			result.blockedAttackers[*attacker] = true;
			result.blockedCount++;
		}
		return result;
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

double AiScoring::shieldZoneValue(int shieldCount)
{
	if (shieldCount <= 0) return 0.0;
	int configuredCount = std::min(shieldCount, 5);
	double value = aiParam("evaluation.shield_count_" +
		std::to_string(configuredCount) + "_value");
	if (shieldCount > 5)
		value += (shieldCount - 5) * aiParam("evaluation.shield_above_5_value");
	return value;
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

bool AiScoring::hasKnockout(Duel& duel, int player, bool requireStableState)
{
	if ((player != 0 && player != 1) || duel.mWinner != -1 || duel.mTurn != player)
		return false;
	if (requireStableState && (duel.mAttackphase != PHASE_NONE ||
		duel.mIsChoiceActive || duel.mCastingCard != -1))
		return false;
	ActiveDuelGuard activeGuard(duel);

	std::vector<int> actualBlockers;
	for (std::vector<Card*>::const_iterator card = duel.mBattlezones[1 - player].mCards.begin();
		card != duel.mBattlezones[1 - player].mCards.end(); ++card)
	{
		if ((*card)->mType == TYPE_CREATURE && !(*card)->mIsTapped)
			actualBlockers.push_back((*card)->mUniqueId);
	}

	std::vector<KnockoutAttacker> attackers;
	int previousAttacker = duel.mAttacker;
	for (std::vector<Card*>::const_iterator card = duel.mBattlezones[player].mCards.begin();
		card != duel.mBattlezones[player].mCards.end(); ++card)
	{
		if ((*card)->mType != TYPE_CREATURE || (*card)->mIsTapped) continue;
		int cardId = (*card)->mUniqueId;
		duel.mAttacker = cardId;
		int canAttackPlayers = duel.getCreatureCanAttackPlayers(cardId);
		bool canAttackThisTurn = (*card)->mSummoningSickness == 0 ||
			duel.getIsSpeedAttacker(cardId) == 1;
		if (canAttackPlayers != CANATTACK_ALWAYS &&
			(!canAttackThisTurn || canAttackPlayers > CANATTACK_UNTAPPED))
			continue;

		KnockoutAttacker attacker;
		attacker.cardId = cardId;
		attacker.breaker = std::max(1, duel.getCreatureBreaker(cardId));
		attackers.push_back(attacker);
	}
	duel.mAttacker = previousAttacker;

	if (attackers.empty()) return false;
	int shieldCount = static_cast<int>(duel.mShields[1 - player].mCards.size());
	int totalBreaker = 0;
	int smallestBreaker = attackers.front().breaker;
	for (std::vector<KnockoutAttacker>::const_iterator attacker = attackers.begin();
		attacker != attackers.end(); ++attacker)
	{
		totalBreaker += attacker->breaker;
		smallestBreaker = std::min(smallestBreaker, attacker->breaker);
	}
	if (shieldCount > 0 &&
		(attackers.size() < 2 || totalBreaker - smallestBreaker < shieldCount))
		return false;
	if (actualBlockers.empty()) return true;

	std::vector<int> blockers;
	for (std::vector<int>::const_iterator blocker = actualBlockers.begin();
		blocker != actualBlockers.end(); ++blocker)
	{
		int copies = duel.getCreatureCanBlockRepeatedly(*blocker) == 1 ?
			static_cast<int>(attackers.size()) : 1;
		for (int copy = 0; copy < copies; ++copy) blockers.push_back(*blocker);
	}
	for (size_t attacker = 0; attacker < attackers.size(); ++attacker)
	{
		duel.mAttacker = attackers[attacker].cardId;
		for (size_t blocker = 0; blocker < blockers.size(); ++blocker)
			if (duel.getCreatureCanBlock(attackers[attacker].cardId, blockers[blocker]) == 1)
				attackers[attacker].blockerSlots.push_back(static_cast<int>(blocker));
	}
	duel.mAttacker = previousAttacker;

	if (shieldCount == 0)
	{
		BlockAssignment defense = optimalBlockAssignment(attackers,
			static_cast<int>(blockers.size()), -1);
		return defense.blockedCount < static_cast<int>(attackers.size());
	}

	// Give the defender advance knowledge of every attack and let it choose the
	// best legal one-blocker-per-attacker matching. If every such defense still
	// leaves enough shield breaks plus a final unblocked attack, KO is guaranteed.
	for (size_t forcedUnblocked = 0; forcedUnblocked < attackers.size(); ++forcedUnblocked)
	{
		BlockAssignment defense = optimalBlockAssignment(attackers,
			static_cast<int>(blockers.size()), static_cast<int>(forcedUnblocked));
		int unblockedCount = 0;
		int unblockedBreaker = 0;
		int weakestUnblocked = 0;
		for (size_t attacker = 0; attacker < attackers.size(); ++attacker)
		{
			if (defense.blockedAttackers[attacker]) continue;
			if (unblockedCount == 0)
				weakestUnblocked = attackers[attacker].breaker;
			else
				weakestUnblocked = std::min(weakestUnblocked, attackers[attacker].breaker);
			unblockedBreaker += attackers[attacker].breaker;
			unblockedCount++;
		}
		if (unblockedCount < 2 || unblockedBreaker - weakestUnblocked < shieldCount)
			return false;
	}
	return true;
}

double AiScoring::playerValue(Duel& duel, int player)
{
	if (player != 0 && player != 1) return 0.0;
	ActiveDuelGuard activeGuard(duel);
	double value = shieldZoneValue(static_cast<int>(duel.mShields[player].mCards.size()));
	value += manaZoneValue(duel, player);
	value += handZoneValue(duel, player,
		static_cast<int>(duel.mManazones[player].mCards.size()));
	value += duel.mDecks[player].mCards.size() * aiParam("evaluation.deck_card_value");
	for (std::vector<Card*>::const_iterator card = duel.mBattlezones[player].mCards.begin();
		card != duel.mBattlezones[player].mCards.end(); ++card)
	{
		value += battleCreatureValue(duel, (*card)->mUniqueId);
	}
	if (hasKnockout(duel, player)) value += aiParam("evaluation.knockout_bonus");
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
