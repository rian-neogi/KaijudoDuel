#include "Npc.h"

#include <algorithm>
#include <cmath>

Npc::Npc(int xValue, int yValue, const std::string& npcName, const std::string& deckPath,
	const std::string& dialogue, const std::string& reward, int rewardGold,
	int rewardCount, NpcKind npcKind, CharacterAppearance characterAppearance,
	const std::string& upgradedDeck)
	: x(xValue), y(yValue), homeX(xValue), homeY(yValue),
	  visualX((float)xValue), visualY((float)yValue), nextMoveAt(0),
	  name(npcName), deck(deckPath), advancedDeck(upgradedDeck),
	  challenge(dialogue), rewardCard(reward), goldReward(rewardGold), wins(0),
	  maxWins(rewardCount), kind(npcKind), appearance(characterAppearance),
	  mWanderState((unsigned int)(xValue * 73856093u) ^
		(unsigned int)(yValue * 19349663u) ^ (unsigned int)npcName.size() * 83492791u)
{
}

Npc Npc::duelist(int x, int y, const std::string& name, const std::string& deck,
	const std::string& challenge, const std::string& rewardCard, int goldReward,
	CharacterAppearance appearance, const std::string& advancedDeck)
{
	return Npc(x, y, name, deck, challenge, rewardCard, goldReward, 4,
		NpcKind::Duelist, appearance, advancedDeck);
}

Npc Npc::shopkeeper(int x, int y, const std::string& name, const std::string& greeting,
	CharacterAppearance appearance)
{
	return Npc(x, y, name, "", greeting, "", 0, 0, NpcKind::Shopkeeper, appearance, "");
}

Npc Npc::boss(int x, int y, const std::string& name, const std::string& deck,
	const std::string& challenge, const std::string& rewardCard, int goldReward,
	CharacterAppearance appearance)
{
	return Npc(x, y, name, deck, challenge, rewardCard, goldReward, 1,
		NpcKind::Boss, appearance, "");
}

bool Npc::isDuelist() const
{
	return kind == NpcKind::Duelist || kind == NpcKind::Boss;
}

bool Npc::isShopkeeper() const
{
	return kind == NpcKind::Shopkeeper;
}

bool Npc::isBoss() const
{
	return kind == NpcKind::Boss;
}

bool Npc::canWander() const
{
	return kind == NpcKind::Duelist;
}

bool Npc::canBattle() const
{
	return isDuelist() && wins < maxWins;
}

bool Npc::isComplete() const
{
	return isDuelist() && wins >= maxWins;
}

std::string Npc::statusText() const
{
	if (isShopkeeper()) return "CARD SHOP";
	if (isComplete()) return "COMPLETE";
	return isBoss() ? (isComplete() ? "DEFEATED" : "ACT I BOSS") :
		"WINS " + std::to_string(std::max(0, wins)) + "/" + std::to_string(maxWins);
}

std::string Npc::battleDeck() const
{
	return wins >= 1 && !advancedDeck.empty() ? advancedDeck : deck;
}

std::string Npc::rankName() const
{
	if (isBoss()) return "Boss Duel";
	const char* ranks[] = { "First Trial", "Adaptation", "Veteran Duel", "Master Duel" };
	return ranks[std::max(0, std::min(3, wins))];
}

bool Npc::isMoving() const
{
	return std::fabs(visualX - x) > 0.001f || std::fabs(visualY - y) > 0.001f;
}

void Npc::updateMovement(unsigned int deltaMilliseconds)
{
	float dx = x - visualX;
	float dy = y - visualY;
	float distance = std::sqrt(dx * dx + dy * dy);
	if (distance <= 0.001f)
	{
		visualX = (float)x;
		visualY = (float)y;
		return;
	}
	const float step = 2.8f * deltaMilliseconds / 1000.f;
	if (step >= distance)
	{
		visualX = (float)x;
		visualY = (float)y;
	}
	else
	{
		visualX += dx / distance * step;
		visualY += dy / distance * step;
	}
}

void Npc::scheduleWander(unsigned int now)
{
	mWanderState = mWanderState * 1664525u + 1013904223u;
	nextMoveAt = now + 900u + (mWanderState % 1800u);
}

int Npc::nextWanderDirection()
{
	mWanderState = mWanderState * 1664525u + 1013904223u;
	return (int)(mWanderState % 4u);
}
