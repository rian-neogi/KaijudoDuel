#include "Npc.h"

#include <algorithm>

Npc::Npc(int xValue, int yValue, const std::string& npcName, const std::string& deckPath,
	const std::string& dialogue, const std::string& reward, int rewardGold, NpcKind npcKind)
	: x(xValue), y(yValue), name(npcName), deck(deckPath), challenge(dialogue),
	  rewardCard(reward), goldReward(rewardGold), wins(0), kind(npcKind)
{
}

Npc Npc::duelist(int x, int y, const std::string& name, const std::string& deck,
	const std::string& challenge, const std::string& rewardCard, int goldReward)
{
	return Npc(x, y, name, deck, challenge, rewardCard, goldReward, NpcKind::Duelist);
}

Npc Npc::shopkeeper(int x, int y, const std::string& name, const std::string& greeting)
{
	return Npc(x, y, name, "", greeting, "", 0, NpcKind::Shopkeeper);
}

bool Npc::isDuelist() const
{
	return kind == NpcKind::Duelist;
}

bool Npc::isShopkeeper() const
{
	return kind == NpcKind::Shopkeeper;
}

bool Npc::canBattle() const
{
	return isDuelist() && wins < 4;
}

bool Npc::isComplete() const
{
	return isDuelist() && wins >= 4;
}

std::string Npc::statusText() const
{
	if (isShopkeeper()) return "CARD SHOP";
	if (isComplete()) return "COMPLETE";
	return "WINS " + std::to_string(std::max(0, wins)) + "/4";
}
