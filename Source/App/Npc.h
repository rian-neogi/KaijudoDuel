#pragma once

#include <map>
#include <string>
#include <vector>

enum class NpcKind
{
	Duelist,
	Shopkeeper,
	Boss
};

enum class CharacterAppearance
{
	Player,
	Mira,
	Marin,
	Rook,
	Aurelia,
	Flint,
	Nyx,
	Tidal,
	Briar,
	Mercer,
	VeiledOne
};

struct NpcReward
{
	std::string card;
	int gold;
};

class Npc
{
public:
	static Npc duelist(int x, int y, const std::string& name,
		const std::vector<std::string>& decks, const std::string& challenge,
		const std::vector<NpcReward>& rewards, CharacterAppearance appearance);
	static Npc shopkeeper(int x, int y, const std::string& name, const std::string& greeting,
		CharacterAppearance appearance);
	static Npc boss(int x, int y, const std::string& name,
		const std::vector<std::string>& decks, const std::string& challenge,
		const std::vector<NpcReward>& rewards, CharacterAppearance appearance);

	bool isDuelist() const;
	bool isShopkeeper() const;
	bool isBoss() const;
	bool canWander() const;
	bool canBattle() const;
	bool isComplete() const;
	std::string statusText() const;
	std::string deckForBattle(int battleIndex) const;
	std::string battleDeck() const;
	NpcReward nextReward() const;
	std::string rankName() const;
	std::string dialogueText(const std::string& key, const std::string& fallback = "") const;
	bool isMoving() const;
	void setPosition(int x, int y);
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
	std::string id;
	std::string name;
	std::vector<std::string> decks;
	std::vector<NpcReward> rewards;
	std::string challenge;
	int wins;
	int maxWins;
	NpcKind kind;
	CharacterAppearance appearance;
	std::string aiPersonality;
	std::map<std::string, std::string> dialogue;

private:
	Npc(int x, int y, const std::string& name, const std::vector<std::string>& decks,
		const std::string& challenge, const std::vector<NpcReward>& rewards,
		NpcKind kind, CharacterAppearance appearance);
	unsigned int mWanderState;
};

bool loadNpcsFromLua(const std::string& path, std::vector<Npc>& npcs, std::string& error);
