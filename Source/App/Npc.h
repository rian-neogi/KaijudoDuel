#pragma once

#include <map>
#include <string>
#include <vector>

enum class NpcKind
{
	Town,
	RouteDuelist,
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
	VeiledOne,
	Neris,
	Oren,
	GenericMale1,
	GenericMale2,
	GenericMale3,
	GenericMale4,
	GenericMale5,
	GenericMale6,
	GenericMale7,
	GenericMale8,
	GenericMale9,
	GenericMale10,
	GenericFemale1,
	GenericFemale2,
	GenericFemale3,
	GenericFemale4,
	GenericFemale5,
	GenericFemale6,
	GenericFemale7,
	GenericFemale8,
	GenericFemale9,
	GenericFemale10
};

struct NpcReward
{
	std::string card;
	int goldTier;
};

int npcGoldRewardValue(int tier);

class Npc
{
public:
	static Npc town(int x, int y, const std::string& name,
		const std::vector<std::string>& decks, const std::string& challenge,
		const std::vector<NpcReward>& rewards, CharacterAppearance appearance,
		bool duelEnabled, bool tradeEnabled, bool wanders);
	static Npc routeDuelist(int x, int y, const std::string& name,
		const std::vector<std::string>& decks, const std::string& challenge,
		const std::vector<NpcReward>& rewards, CharacterAppearance appearance,
		int sightRange);
	static Npc boss(int x, int y, const std::string& name,
		const std::vector<std::string>& decks, const std::string& challenge,
		const std::vector<NpcReward>& rewards, CharacterAppearance appearance);

	bool isDuelist() const;
	bool isTownNpc() const;
	bool isRouteDuelist() const;
	bool isBoss() const;
	bool canTrade() const;
	bool canWander() const;
	bool canBattle() const;
	bool isComplete() const;
	std::string deckForBattle(int battleIndex) const;
	std::string battleDeck() const;
	NpcReward nextReward() const;
	std::string rankName() const;
	std::string dialogueText(const std::string& key, const std::string& fallback = "") const;
	bool isMoving() const;
	void setPosition(int x, int y);
	void moveTo(int x, int y);
	void updateMovement(unsigned int deltaMilliseconds, float tilesPerSecond = 2.8f);
	void scheduleWander(unsigned int now);
	int nextWanderDirection();

	int x;
	int y;
	int homeX;
	int homeY;
	float visualX;
	float visualY;
	int facingX;
	int facingY;
	unsigned int nextMoveAt;
	std::string id;
	std::string crestId;
	std::string mapId;
	std::string shopStockId;
	std::string name;
	std::vector<std::string> decks;
	std::vector<NpcReward> rewards;
	std::string challenge;
	int wins;
	int maxWins;
	NpcKind kind;
	CharacterAppearance appearance;
	std::string spriteSheet;
	int spriteIndex;
	bool duelEnabled;
	bool tradeEnabled;
	bool wanders;
	int sightRange;
	std::string aiPersonality;
	std::string aiDifficulty;
	std::map<std::string, std::string> dialogue;

private:
	Npc(int x, int y, const std::string& name, const std::vector<std::string>& decks,
		const std::string& challenge, const std::vector<NpcReward>& rewards,
		NpcKind kind, CharacterAppearance appearance);
	unsigned int mWanderState;
};

bool loadNpcsFromLua(const std::string& path, std::vector<Npc>& npcs, std::string& error);
