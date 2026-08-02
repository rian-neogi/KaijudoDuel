#pragma once

#include <string>

enum class NpcKind
{
	Duelist,
	Shopkeeper
};

class Npc
{
public:
	static Npc duelist(int x, int y, const std::string& name, const std::string& deck,
		const std::string& challenge, const std::string& rewardCard, int goldReward);
	static Npc shopkeeper(int x, int y, const std::string& name, const std::string& greeting);

	bool isDuelist() const;
	bool isShopkeeper() const;
	bool canBattle() const;
	bool isComplete() const;
	std::string statusText() const;

	int x;
	int y;
	std::string name;
	std::string deck;
	std::string challenge;
	std::string rewardCard;
	int goldReward;
	int wins;
	NpcKind kind;

private:
	Npc(int x, int y, const std::string& name, const std::string& deck,
		const std::string& challenge, const std::string& rewardCard,
		int goldReward, NpcKind kind);
};
