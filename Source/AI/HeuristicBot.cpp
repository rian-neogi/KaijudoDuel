#include "HeuristicBot.h"

#include "AiScoring.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace
{
	const size_t LOW_HAND_CARD_COUNT = 2;

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
	if (duel.mHands[mPlayer].mCards.size() <= LOW_HAND_CARD_COUNT)
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
		return 1.0;

	double value = 1.0 + card->mManaCost * 0.7;
	if (card->mType == TYPE_CREATURE)
	{
		int power = card->mPower;
		if (allowLuaQueries && card->mZone == ZONE_BATTLE) power = duel.getCreaturePower(cardId);
		value += std::max(0, power) / 1000.0;
		value += std::max(1, card->mBreaker) * 0.8;
		if (card->mIsBlocker) value += 1.5;
	}
	else
		value += card->mManaCost * 0.5;
	if (card->mIsShieldTrigger) value += 1.0;
	return value;
}

double HeuristicBot::scoreChoice(Duel& duel, int selection) const
{
	if (duel.mChoice != NULL && duel.mChoice->mAiPreferredSelection == selection)
		return 100000.0;
	std::string prompt = duel.mChoice == NULL ? "" : lowerText(duel.mChoice->mInfotext);
	if (selection < 0)
	{
		if (selection == RETURN_BUTTON1)
		{
			if (prompt.find("draw") != std::string::npos) return 40.0;
			if (prompt.find("use") != std::string::npos) return 25.0;
			return 5.0;
		}
		return 0.0;
	}
	if (selection >= (int)duel.mCardList.size()) return -1000.0;
	Card* target = duel.mCardList[selection];
	double value = cardValue(duel, selection, false);
	bool friendly = target->mOwner == mPlayer;
	if (prompt.find("destroy") != std::string::npos || prompt.find("discard") != std::string::npos)
		return friendly ? 30.0 - value * 5.0 : 30.0 + value * 5.0;
	if (prompt.find("graveyard") != std::string::npos)
		return friendly ? 20.0 + value * 4.0 : 20.0 - value * 3.0;
	if (prompt.find("return") != std::string::npos)
		return friendly ? 20.0 - value * 2.0 : 20.0 + value * 4.0;
	if (!friendly) return 15.0 + value * 3.0;
	if (target->mZone == ZONE_GRAVEYARD) return 15.0 + value * 3.0;
	return 10.0 + value;
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

	int remainingCivilizations[CIV_DARKNESS + 1] = {};
	for (std::vector<Card*>::const_iterator mana = duel.mManazones[mPlayer].mCards.begin();
		mana != duel.mManazones[mPlayer].mCards.end(); ++mana)
	{
		if ((*mana)->mIsTapped || (*mana)->mUniqueId == cardId ||
			std::find(duel.mCastingManaCards.begin(), duel.mCastingManaCards.end(),
				(*mana)->mUniqueId) != duel.mCastingManaCards.end()) continue;
		for (int civilization = CIV_LIGHT; civilization <= CIV_DARKNESS; ++civilization)
			if (((*mana)->mCivilizations & (1 << civilization)) != 0)
				remainingCivilizations[civilization]++;
	}

	double score = 0.0;
	for (int civilization = CIV_LIGHT; civilization <= CIV_DARKNESS; ++civilization)
	{
		if (remainingCivilizations[civilization] > 0) score += 30.0;
		score += std::min(3, remainingCivilizations[civilization]) * 3.0;
	}

	// Preserve the civilization combinations needed by cards that could still
	// be cast later this turn. This is intentionally a cheap payment policy;
	// legality remains enforced by Duel::canTapManaForCasting.
	for (std::vector<Card*>::const_iterator card = duel.mHands[mPlayer].mCards.begin();
		card != duel.mHands[mPlayer].mCards.end(); ++card)
	{
		if ((*card)->mUniqueId == duel.mCastingCard) continue;
		bool covered = true;
		for (int civilization = CIV_LIGHT; civilization <= CIV_DARKNESS; ++civilization)
		{
			if (((*card)->mCivilizations & (1 << civilization)) != 0 &&
				remainingCivilizations[civilization] == 0)
			{
				covered = false;
				break;
			}
		}
		if (covered) score += 2.0 + std::min(8, (*card)->mManaCost) * 0.25;
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
		if (duel.mShields[1 - mPlayer].mCards.empty()) return 100000.0;
		double score = 35.0 + duel.getCreatureBreaker(attacker) * 8.0;
		for (size_t i = 0; i < duel.mBattlezones[1 - mPlayer].mCards.size(); ++i)
			if (duel.getCreatureIsBlocker(duel.mBattlezones[1 - mPlayer].mCards[i]->mUniqueId))
				score -= 4.0;
		return score;
	}

	if (defenderType != DEFENDER_CREATURE || defender < 0 || defender >= (int)duel.mCardList.size())
		return -std::numeric_limits<double>::infinity();
	int defenderPower = duel.getCreaturePower(defender);
	if (defenderPower > attackerPower) return -std::numeric_limits<double>::infinity();
	double attackerValue = cardValue(duel, attacker, true);
	double defenderValue = cardValue(duel, defender, true);
	if (attackerPower > defenderPower) return 65.0 + defenderValue * 5.0;
	return 35.0 + (defenderValue - attackerValue) * 5.0;
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
		duel.mAttacker < 0 || duel.mAttacker >= (int)duel.mCardList.size()) return -1000.0;
	int blockerPower = duel.getCreaturePower(blocker);
	int attackerPower = duel.getCreaturePower(duel.mAttacker);
	double score = duel.mShields[mPlayer].mCards.size() <= 1 ? 80.0 : 20.0;
	if (blockerPower > attackerPower)
	{
		// Any surviving block is better than a trade or losing block. Within
		// that group, conserve stronger blockers by using the weakest winner.
		return 1000.0 + 1000000.0 / (std::max(0, blockerPower) + 1.0);
	}
	else if (blockerPower == attackerPower) score += 25.0;
	else score -= cardValue(duel, blocker, true) * 5.0;
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
		double score = 42.0 + cardValue(duel, card, true) * 4.0;
		if (messageInt(move, "evobait") >= 0) score += 8.0;
		if (messageInt(move, "evobait2") >= 0) score += 8.0;
		return adjusted(score);
	}
	if (type == "manatap") return adjusted(20.0);
	if (type == "creatureattack") return adjusted(scoreAttack(duel, move));
	if (type == "creatureblock") return adjusted(scoreBlock(duel, messageInt(move, "blocker")));
	if (type == "blockskip") return adjusted(1.0);
	if (type == "targetshield") return adjusted(20.0);
	if (type == "triggeruse") return adjusted(70.0 + cardValue(duel, messageInt(move, "trigger"), false));
	if (type == "triggerskip") return adjusted(0.0);
	if (type == "creatureusetapability") return adjusted(35.0);
	if (type == "endturn") return adjusted(-100.0);
	return adjusted(0.0);
}

double HeuristicBot::adjustForPersonality(const std::string& moveType, double score) const
{
	if (!std::isfinite(score)) return score;
	if (mPersonality == "aggressive")
	{
		if (moveType == "creatureattack") score += 16.0;
		else if (moveType == "cardplay") score += 6.0;
		else if (moveType == "blockskip") score += 3.0;
	}
	else if (mPersonality == "defensive")
	{
		if (moveType == "creatureblock" || moveType == "triggeruse") score += 12.0;
		else if (moveType == "creatureattack") score -= 5.0;
	}
	else if (mPersonality == "control")
	{
		if (moveType == "choiceselect" || moveType == "creatureusetapability") score += 5.0;
		else if (moveType == "triggeruse") score += 7.0;
	}
	else if (mPersonality == "tempo")
	{
		if (moveType == "cardplay" || moveType == "creatureattack") score += 7.0;
		else if (moveType == "cardmana") score -= 2.0;
	}
	else if (mPersonality == "ramp")
	{
		if (moveType == "cardmana") score += 10.0;
		else if (moveType == "cardplay") score += 2.0;
	}
	else if (mPersonality == "sacrifice")
	{
		if (moveType == "choiceselect" || moveType == "cardplay") score += 4.0;
	}
	return score;
}
