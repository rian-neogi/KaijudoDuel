#include "HeuristicBot.h"

#include "AiParams.h"
#include "AiScoring.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace
{
	std::string messageType(const Message& message)
	{
		std::map<std::string, std::string>::const_iterator found = message.map.find("msgtype");
		return found == message.map.end() ? "" : found->second;
	}

	int messageInt(const Message& message, const std::string& key, int fallback = -1)
	{
		std::map<std::string, std::string>::const_iterator found = message.map.find(key);
		return found == message.map.end() ? fallback : std::atoi(found->second.c_str());
	}

	std::string lowerText(const std::string& value)
	{
		std::string result = value;
		for (size_t i = 0; i < result.size(); ++i)
			result[i] = (char)std::tolower((unsigned char)result[i]);
		return result;
	}
}

HeuristicBot::HeuristicBot(int player, const std::string& personality)
	: mPlayer(player), mPersonality(lowerText(personality))
{
}

Message HeuristicBot::chooseMove(Duel& duel, const std::vector<Message>& moves) const
{
	if (moves.empty()) return Message();
	int preferredPriority = 3;
	bool ordinaryTurn = !duel.mIsChoiceActive && duel.mAttackphase == PHASE_NONE &&
		duel.mCastingCard == -1 && duel.getPlayerToMove() == mPlayer;
	auto movePriority = [](const std::string& type)
	{
		if (type == "cardmana") return 0;
		if (type == "cardplay") return 1;
		if (type == "creatureusetapability") return 2;
		return 3;
	};
	if (ordinaryTurn)
	{
		for (size_t i = 0; i < moves.size(); ++i)
		{
			if (messageType(moves[i]) == "cardplay" &&
				duel.getCardAiCanCast(messageInt(moves[i], "card")) == 0)
				continue;
			preferredPriority = std::min(preferredPriority, movePriority(messageType(moves[i])));
		}
	}

	size_t bestIndex = moves.size();
	size_t fallbackIndex = moves.size();
	double bestScore = -std::numeric_limits<double>::infinity();
	for (size_t i = 0; i < moves.size(); ++i)
	{
		if (messageType(moves[i]) == "cardplay" &&
			duel.getCardAiCanCast(messageInt(moves[i], "card")) == 0)
			continue;
		if (ordinaryTurn && movePriority(messageType(moves[i])) != preferredPriority) continue;
		if (fallbackIndex == moves.size()) fallbackIndex = i;
		double score = scoreMove(duel, moves[i]);
		if (score == -std::numeric_limits<double>::infinity()) continue;
		if (score > bestScore)
		{
			bestScore = score;
			bestIndex = i;
		}
	}
	if (bestIndex < moves.size()) return moves[bestIndex];
	if (fallbackIndex < moves.size()) return moves[fallbackIndex];
	return moves.front();
}

bool HeuristicBot::chooseManaPlacement(Duel& duel, const std::vector<Message>& moves,
	Message& result) const
{
	if (duel.mTurnPhase != TURN_PHASE_MANA || duel.mManaUsed != 0 ||
		duel.mCastingCard != -1 || duel.mIsChoiceActive || duel.mAttackphase != PHASE_NONE)
		return false;
	if (duel.mHands[mPlayer].mCards.size() <=
		static_cast<size_t>(std::max(0, aiIntParam("heuristic.low_hand_card_count"))))
	{
		int maximumDeckCost = 0;
		for (std::vector<Card*>::const_iterator card = duel.mCardList.begin();
			card != duel.mCardList.end(); ++card)
		{
			if ((*card)->mOwner == mPlayer)
				maximumDeckCost = std::max(maximumDeckCost, (*card)->mManaCost);
		}
		if (maximumDeckCost > 0 &&
			duel.mManazones[mPlayer].mCards.size() > static_cast<size_t>(maximumDeckCost))
			return false;
	}
	double bestScore = -std::numeric_limits<double>::infinity();
	bool found = false;
	for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
	{
		if (messageType(*move) != "cardmana") continue;
		double score = scoreMove(duel, *move);
		if (!found || score > bestScore)
		{
			result = *move;
			bestScore = score;
			found = true;
		}
	}
	return found;
}

int HeuristicBot::chooseManaPayment(Duel& duel, const std::vector<int>& options) const
{
	int selected = -1;
	double bestScore = -std::numeric_limits<double>::infinity();
	for (std::vector<int>::const_iterator option = options.begin(); option != options.end(); ++option)
	{
		double score = scoreManaPayment(duel, *option);
		if (selected < 0 || score > bestScore || (score == bestScore && *option < selected))
		{
			selected = *option;
			bestScore = score;
		}
	}
	return selected;
}

double HeuristicBot::cardValue(Duel& duel, int cardId, bool allowLuaQueries) const
{
	if (cardId < 0 || cardId >= (int)duel.mCardList.size()) return 0.0;
	Card* card = duel.mCardList[cardId];
	// Never value an opponent's hidden card by identity. Legal choices that
	// point into hidden zones should remain strategically indistinguishable.
	if (card->mOwner != mPlayer &&
		(card->mZone == ZONE_HAND || card->mZone == ZONE_DECK || card->mZone == ZONE_SHIELD))
		return aiParam("heuristic.card.hidden_value");

	double value = aiParam("heuristic.card.base_value") +
		card->mManaCost * aiParam("heuristic.card.cost_weight");
	if (card->mType == TYPE_CREATURE)
	{
		int power = card->mPower;
		if (allowLuaQueries && card->mZone == ZONE_BATTLE) power = duel.getCreaturePower(cardId);
		value += std::max(0, power) /
			std::max(0.000001, aiParam("heuristic.card.power_divisor"));
		value += std::max(aiIntParam("heuristic.card.minimum_breaker"), card->mBreaker) *
			aiParam("heuristic.card.breaker_weight");
		if (card->mIsBlocker) value += aiParam("heuristic.card.blocker_bonus");
	}
	else
		value += card->mManaCost * aiParam("heuristic.card.spell_cost_weight");
	if (card->mIsShieldTrigger) value += aiParam("heuristic.card.shield_trigger_bonus");
	return value;
}

double HeuristicBot::scoreChoice(Duel& duel, int selection) const
{
	if (duel.mChoice != NULL && duel.mChoice->mAiPreferredSelection == selection)
		return aiParam("heuristic.choice.preferred_score");
	std::string prompt = duel.mChoice == NULL ? "" : lowerText(duel.mChoice->mInfotext);
	if (selection < 0)
	{
		if (selection == RETURN_BUTTON1)
		{
			if (prompt.find("draw") != std::string::npos)
				return aiParam("heuristic.choice.button_draw_score");
			if (prompt.find("use") != std::string::npos)
				return aiParam("heuristic.choice.button_use_score");
			return aiParam("heuristic.choice.button_default_score");
		}
		return aiParam("heuristic.choice.button_other_score");
	}
	if (selection >= (int)duel.mCardList.size())
		return aiParam("heuristic.choice.invalid_score");
	Card* target = duel.mCardList[selection];
	double value = cardValue(duel, selection, false);
	bool friendly = target->mOwner == mPlayer;
	if (prompt.find("destroy") != std::string::npos || prompt.find("discard") != std::string::npos)
		return aiParam("heuristic.choice.destroy_base") +
			value * aiParam("heuristic.choice.destroy_value_weight") * (friendly ? -1.0 : 1.0);
	if (prompt.find("graveyard") != std::string::npos)
		return aiParam("heuristic.choice.graveyard_base") + value * aiParam(friendly ?
			"heuristic.choice.graveyard_friendly_weight" :
			"heuristic.choice.graveyard_opponent_weight");
	if (prompt.find("return") != std::string::npos)
		return aiParam("heuristic.choice.return_base") + value * aiParam(friendly ?
			"heuristic.choice.return_friendly_weight" :
			"heuristic.choice.return_opponent_weight");
	if (!friendly || target->mZone == ZONE_GRAVEYARD)
		return aiParam("heuristic.choice.opponent_base") +
			value * aiParam("heuristic.choice.opponent_value_weight");
	return aiParam("heuristic.choice.friendly_base") +
		value * aiParam("heuristic.choice.friendly_value_weight");
}

double HeuristicBot::scoreManaCharge(Duel& duel, int cardId) const
{
	return AiScoring::manaPlacementDelta(duel, mPlayer, cardId);
}

double HeuristicBot::scoreManaPayment(Duel& duel, int cardId) const
{
	if (cardId < 0 || cardId >= (int)duel.mCardList.size())
		return -std::numeric_limits<double>::infinity();
	Card* selected = duel.mCardList[cardId];
	if (selected->mOwner != mPlayer || selected->mZone != ZONE_MANA || selected->mIsTapped)
		return -std::numeric_limits<double>::infinity();

	int remainingCivilizations[CIV_HOLLOW + 1] = {};
	for (std::vector<Card*>::const_iterator mana = duel.mManazones[mPlayer].mCards.begin();
		mana != duel.mManazones[mPlayer].mCards.end(); ++mana)
	{
		if ((*mana)->mIsTapped || (*mana)->mUniqueId == cardId ||
			std::find(duel.mCastingManaCards.begin(), duel.mCastingManaCards.end(),
				(*mana)->mUniqueId) != duel.mCastingManaCards.end()) continue;
		bool hollowMana = ((*mana)->mCivilizations & (1 << CIV_HOLLOW)) != 0;
		for (int civilization = CIV_LIGHT; civilization <= CIV_HOLLOW; ++civilization)
			if (hollowMana || ((*mana)->mCivilizations & (1 << civilization)) != 0)
				remainingCivilizations[civilization]++;
	}

	double score = 0.0;
	for (int civilization = CIV_LIGHT; civilization <= CIV_HOLLOW; ++civilization)
	{
		if (remainingCivilizations[civilization] > 0)
			score += aiParam("heuristic.mana_payment.civilization_coverage_score");
		score += std::min(aiIntParam("heuristic.mana_payment.civilization_count_cap"),
			remainingCivilizations[civilization]) *
			aiParam("heuristic.mana_payment.civilization_count_weight");
	}

	// Preserve the civilization combinations needed by cards that could still
	// be cast later this turn. This is intentionally a cheap payment policy;
	// legality remains enforced by Duel::canTapManaForCasting.
	for (std::vector<Card*>::const_iterator card = duel.mHands[mPlayer].mCards.begin();
		card != duel.mHands[mPlayer].mCards.end(); ++card)
	{
		if ((*card)->mUniqueId == duel.mCastingCard) continue;
		bool hollowCard = ((*card)->mCivilizations & (1 << CIV_HOLLOW)) != 0;
		bool covered = true;
		for (int civilization = CIV_LIGHT; civilization <= CIV_HOLLOW; ++civilization)
		{
			if (!hollowCard &&
				((*card)->mCivilizations & (1 << civilization)) != 0 &&
				remainingCivilizations[civilization] == 0)
			{
				covered = false;
				break;
			}
		}
		if (covered)
			score += aiParam("heuristic.mana_payment.castable_card_base") +
				std::min(aiIntParam("heuristic.mana_payment.castable_cost_cap"),
					(*card)->mManaCost) *
				aiParam("heuristic.mana_payment.castable_cost_weight");
	}
	return score;
}

double HeuristicBot::scoreAttack(Duel& duel, const Message& move) const
{
	int attacker = messageInt(move, "attacker");
	if (attacker < 0 || attacker >= (int)duel.mCardList.size())
		return -std::numeric_limits<double>::infinity();
	int defenderType = messageInt(move, "defendertype");
	int defender = defenderType == DEFENDER_CREATURE ? messageInt(move, "defender") : -1;
	int attackerPower = attackingPower(duel, attacker);
	if (hasStrongerBlocker(duel, attacker, attackerPower, defender))
		return -std::numeric_limits<double>::infinity();

	if (defenderType == DEFENDER_PLAYER)
	{
		if (duel.mShields[1 - mPlayer].mCards.empty())
			return aiParam("heuristic.attack.lethal_score");
		double score = aiParam("heuristic.attack.player_base") +
			duel.getCreatureBreaker(attacker) * aiParam("heuristic.attack.breaker_weight");
		for (size_t i = 0; i < duel.mBattlezones[1 - mPlayer].mCards.size(); ++i)
			if (duel.getCreatureIsBlocker(duel.mBattlezones[1 - mPlayer].mCards[i]->mUniqueId))
				score -= aiParam("heuristic.attack.blocker_penalty");
		return score;
	}

	if (defenderType != DEFENDER_CREATURE || defender < 0 || defender >= (int)duel.mCardList.size())
		return -std::numeric_limits<double>::infinity();
	int defenderPower = duel.getCreaturePower(defender);
	if (defenderPower > attackerPower) return -std::numeric_limits<double>::infinity();
	double attackerValue = cardValue(duel, attacker, true);
	double defenderValue = cardValue(duel, defender, true);
	if (attackerPower > defenderPower)
		return aiParam("heuristic.attack.winning_creature_base") +
			defenderValue * aiParam("heuristic.attack.winning_target_weight");
	return aiParam("heuristic.attack.trade_base") +
		(defenderValue - attackerValue) * aiParam("heuristic.attack.trade_value_weight");
}

int HeuristicBot::attackingPower(Duel& duel, int attacker) const
{
	int previousAttacker = duel.mAttacker;
	duel.mAttacker = attacker;
	int power = duel.getCreaturePower(attacker);
	duel.mAttacker = previousAttacker;
	return power;
}

bool HeuristicBot::hasStrongerBlocker(
	Duel& duel, int attacker, int attackerPower, int attackedCreature) const
{
	const std::vector<Card*>& defenders = duel.mBattlezones[1 - mPlayer].mCards;
	for (size_t i = 0; i < defenders.size(); ++i)
	{
		Card* blocker = defenders[i];
		if (blocker->mUniqueId == attackedCreature || blocker->mIsTapped) continue;
		if (!duel.getCreatureCanBlock(attacker, blocker->mUniqueId)) continue;
		if (duel.getCreaturePower(blocker->mUniqueId) > attackerPower) return true;
	}
	return false;
}

double HeuristicBot::scoreBlock(Duel& duel, int blocker) const
{
	if (blocker < 0 || blocker >= (int)duel.mCardList.size() ||
		duel.mAttacker < 0 || duel.mAttacker >= (int)duel.mCardList.size())
		return aiParam("heuristic.block.invalid_score");
	int blockerPower = duel.getCreaturePower(blocker);
	int attackerPower = duel.getCreaturePower(duel.mAttacker);
	double score = duel.mShields[mPlayer].mCards.size() <=
			static_cast<size_t>(std::max(0, aiIntParam("heuristic.block.urgent_shield_count"))) ?
		aiParam("heuristic.block.urgent_base") : aiParam("heuristic.block.normal_base");
	if (blockerPower > attackerPower)
	{
		// Any surviving block is better than a trade or losing block. Within
		// that group, conserve stronger blockers by using the weakest winner.
		return aiParam("heuristic.block.surviving_base") +
			aiParam("heuristic.block.weakest_winner_numerator") /
				(std::max(0, blockerPower) + 1.0);
	}
	else if (blockerPower == attackerPower)
		score += aiParam("heuristic.block.trade_bonus");
	else
		score -= cardValue(duel, blocker, true) *
			aiParam("heuristic.block.losing_card_penalty");
	return score;
}

double HeuristicBot::scoreMove(Duel& duel, const Message& move) const
{
	const std::string type = messageType(move);
	auto adjusted = [this, &type](double score)
	{
		return adjustForPersonality(type, score);
	};
	if (type == "choiceselect") return adjusted(scoreChoice(duel, messageInt(move, "selection")));
	if (type == "cardmana") return adjusted(scoreManaCharge(duel, messageInt(move, "card")));
	if (type == "cardplay")
	{
		int card = messageInt(move, "card");
		if (duel.getCardAiCanCast(card) == 0)
			return -std::numeric_limits<double>::infinity();
		double score = aiParam("heuristic.move.cardplay_base") +
			cardValue(duel, card, true) * aiParam("heuristic.move.cardplay_value_weight");
		if (messageInt(move, "evobait") >= 0)
			score += aiParam("heuristic.move.evolution_bait_bonus");
		if (messageInt(move, "evobait2") >= 0)
			score += aiParam("heuristic.move.evolution_bait_bonus");
		return adjusted(score);
	}
	if (type == "manatap") return adjusted(aiParam("heuristic.move.mana_tap_score"));
	if (type == "creatureattack") return adjusted(scoreAttack(duel, move));
	if (type == "creatureblock") return adjusted(scoreBlock(duel, messageInt(move, "blocker")));
	if (type == "blockskip") return adjusted(aiParam("heuristic.move.block_skip_score"));
	if (type == "targetshield") return adjusted(aiParam("heuristic.move.shield_target_score"));
	if (type == "triggeruse")
		return adjusted(aiParam("heuristic.move.trigger_base") +
			cardValue(duel, messageInt(move, "trigger"), false));
	if (type == "triggerskip") return adjusted(aiParam("heuristic.move.trigger_skip_score"));
	if (type == "creatureusetapability")
		return adjusted(aiParam("heuristic.move.tap_ability_score"));
	if (type == "endturn") return adjusted(aiParam("heuristic.move.end_turn_score"));
	return adjusted(aiParam("heuristic.move.default_score"));
}

double HeuristicBot::adjustForPersonality(const std::string& moveType, double score) const
{
	if (!std::isfinite(score)) return score;
	if (mPersonality == "aggressive")
	{
		if (moveType == "creatureattack") score += aiParam("heuristic.personality.aggressive_attack");
		else if (moveType == "cardplay") score += aiParam("heuristic.personality.aggressive_cardplay");
		else if (moveType == "blockskip") score += aiParam("heuristic.personality.aggressive_blockskip");
	}
	else if (mPersonality == "defensive")
	{
		if (moveType == "creatureblock" || moveType == "triggeruse")
			score += aiParam("heuristic.personality.defensive_block_or_trigger");
		else if (moveType == "creatureattack") score += aiParam("heuristic.personality.defensive_attack");
	}
	else if (mPersonality == "control")
	{
		if (moveType == "choiceselect" || moveType == "creatureusetapability")
			score += aiParam("heuristic.personality.control_choice_or_tap");
		else if (moveType == "triggeruse") score += aiParam("heuristic.personality.control_trigger");
	}
	else if (mPersonality == "tempo")
	{
		if (moveType == "cardplay" || moveType == "creatureattack")
			score += aiParam("heuristic.personality.tempo_cardplay_or_attack");
		else if (moveType == "cardmana") score += aiParam("heuristic.personality.tempo_mana");
	}
	else if (mPersonality == "ramp")
	{
		if (moveType == "cardmana") score += aiParam("heuristic.personality.ramp_mana");
		else if (moveType == "cardplay") score += aiParam("heuristic.personality.ramp_cardplay");
	}
	else if (mPersonality == "sacrifice")
	{
		if (moveType == "choiceselect" || moveType == "cardplay")
			score += aiParam("heuristic.personality.sacrifice_choice_or_cardplay");
	}
	return score;
}
