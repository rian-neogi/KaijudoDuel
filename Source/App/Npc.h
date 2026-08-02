#pragma once

#include <string>

enum class NpcKind
{
	Duelist,
	Shopkeeper,
	Boss
};

class Npc
{
public:
	static Npc duelist(int x, int y, const std::string& name, const std::string& deck,
		const std::string& challenge, const std::string& rewardCard, int goldReward,
		const std::string& advancedDeck = "");
	static Npc shopkeeper(int x, int y, const std::string& name, const std::string& greeting);
	static Npc boss(int x, int y, const std::string& name, const std::string& deck,
		const std::string& challenge, const std::string& rewardCard, int goldReward);

	bool isDuelist() const;
	bool isShopkeeper() const;
	bool isBoss() const;
	bool canWander() const;
	bool canBattle() const;
	bool isComplete() const;
	std::string statusText() const;
	std::string battleDeck() const;
	std::string rankName() const;
	bool isMoving() const;
	void updateMovement(unsigned int deltaMilliseconds);
	void scheduleWander(unsigned int now);
	int nextWanderDirection();

	int x;
	int y;
	int homeX;
	int homeY;
	float visualX;
	float visualY;
	unsigned int nextMoveAt;
	std::string name;
	std::string deck;
	std::string advancedDeck;
	std::string challenge;
	std::string rewardCard;
	int goldReward;
	int wins;
	int maxWins;
	NpcKind kind;

private:
	Npc(int x, int y, const std::string& name, const std::string& deck,
		const std::string& challenge, const std::string& rewardCard,
		int goldReward, int maxWins, NpcKind kind, const std::string& advancedDeck);
	unsigned int mWanderState;
};
