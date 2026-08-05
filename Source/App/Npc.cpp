#include "Npc.h"

#include "Game/Card.h"
#include "Game/Deck.h"
#include "LuaInclude.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace
{
	std::string luaStringField(lua_State* state, int table, const char* key,
		const std::string& fallback = "")
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		std::string value = lua_isstring(state, -1) ? lua_tostring(state, -1) : fallback;
		lua_pop(state, 1);
		return value;
	}

	int luaIntegerField(lua_State* state, int table, const char* key, int fallback)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		int value = lua_isnumber(state, -1) ? (int)lua_tointeger(state, -1) : fallback;
		lua_pop(state, 1);
		return value;
	}

	bool luaBooleanField(lua_State* state, int table, const char* key, bool fallback)
	{
		table = lua_absindex(state, table);
		lua_getfield(state, table, key);
		bool value = lua_isboolean(state, -1) ? lua_toboolean(state, -1) != 0 : fallback;
		lua_pop(state, 1);
		return value;
	}

	bool parseKind(const std::string& value, NpcKind& result)
	{
		if (value == "town_npc") result = NpcKind::Town;
		else if (value == "route_duelist") result = NpcKind::RouteDuelist;
		else if (value == "boss") result = NpcKind::Boss;
		else return false;
		return true;
	}

	bool parseFacing(const std::string& value, int& x, int& y)
	{
		x = 0;
		y = 0;
		if (value == "up") y = -1;
		else if (value == "down") y = 1;
		else if (value == "left") x = -1;
		else if (value == "right") x = 1;
		else return false;
		return true;
	}

	bool validCrestId(const std::string& value)
	{
		static const char* ids[] = { "dawn", "tidal", "forge", "verdant",
			"confluence", "tempest", "ashen", "mirror", "unity" };
		for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
			if (value == ids[i]) return true;
		return false;
	}

	bool parseAppearance(const std::string& value, CharacterAppearance& result)
	{
		if (value == "mira") result = CharacterAppearance::Mira;
		else if (value == "marin") result = CharacterAppearance::Marin;
		else if (value == "rook") result = CharacterAppearance::Rook;
		else if (value == "aurelia") result = CharacterAppearance::Aurelia;
		else if (value == "flint") result = CharacterAppearance::Flint;
		else if (value == "nyx") result = CharacterAppearance::Nyx;
		else if (value == "tidal") result = CharacterAppearance::Tidal;
		else if (value == "briar") result = CharacterAppearance::Briar;
		else if (value == "mercer") result = CharacterAppearance::Mercer;
		else if (value == "veiled_one") result = CharacterAppearance::VeiledOne;
		else if (value == "neris") result = CharacterAppearance::Neris;
		else if (value == "oren") result = CharacterAppearance::Oren;
		else if (value.compare(0, 7, "generic") == 0)
		{
			const std::string suffix = value.substr(7);
			int variant = suffix == "10" ? 10 :
				(suffix.size() == 1 && suffix[0] >= '1' && suffix[0] <= '9' ? suffix[0] - '0' : 0);
			if (variant == 0) return false;
			result = static_cast<CharacterAppearance>(
				static_cast<int>(CharacterAppearance::Generic1) + variant - 1);
		}
		else return false;
		return true;
	}

	void readDialogue(lua_State* state, int npcTable,
		std::map<std::string, std::string>& dialogue)
	{
		npcTable = lua_absindex(state, npcTable);
		lua_getfield(state, npcTable, "dialogue");
		if (!lua_istable(state, -1))
		{
			lua_pop(state, 1);
			return;
		}
		lua_pushnil(state);
		while (lua_next(state, -2) != 0)
		{
			if (lua_isstring(state, -2) && lua_isstring(state, -1))
				dialogue[lua_tostring(state, -2)] = lua_tostring(state, -1);
			lua_pop(state, 1);
		}
		lua_pop(state, 1);
	}
}

Npc::Npc(int xValue, int yValue, const std::string& npcName,
	const std::vector<std::string>& deckPaths, const std::string& greeting,
	const std::vector<NpcReward>& battleRewards, NpcKind npcKind,
	CharacterAppearance characterAppearance)
	: x(xValue), y(yValue), homeX(xValue), homeY(yValue),
	  visualX((float)xValue), visualY((float)yValue), nextMoveAt(0),
	  name(npcName), decks(deckPaths), rewards(battleRewards), challenge(greeting), wins(0),
	  maxWins((int)battleRewards.size()), kind(npcKind), appearance(characterAppearance),
	  duelEnabled(npcKind != NpcKind::Town), tradeEnabled(false), wanders(false),
	  sightRange(0), facingX(0), facingY(1),
	  mWanderState((unsigned int)(xValue * 73856093u) ^
		(unsigned int)(yValue * 19349663u) ^ (unsigned int)npcName.size() * 83492791u)
{
}

Npc Npc::town(int x, int y, const std::string& name,
	const std::vector<std::string>& decks, const std::string& challenge,
	const std::vector<NpcReward>& rewards, CharacterAppearance appearance,
	bool canDuel, bool canOfferTrade, bool canWander)
{
	Npc npc(x, y, name, decks, challenge, rewards, NpcKind::Town, appearance);
	npc.duelEnabled = canDuel;
	npc.tradeEnabled = canOfferTrade;
	npc.wanders = canWander;
	return npc;
}

Npc Npc::routeDuelist(int x, int y, const std::string& name,
	const std::vector<std::string>& decks, const std::string& challenge,
	const std::vector<NpcReward>& rewards, CharacterAppearance appearance,
	int trainerSightRange, int trainerFacingX, int trainerFacingY)
{
	Npc npc(x, y, name, decks, challenge, rewards, NpcKind::RouteDuelist, appearance);
	npc.duelEnabled = true;
	npc.sightRange = trainerSightRange;
	npc.facingX = trainerFacingX;
	npc.facingY = trainerFacingY;
	return npc;
}

Npc Npc::boss(int x, int y, const std::string& name,
	const std::vector<std::string>& decks, const std::string& challenge,
	const std::vector<NpcReward>& rewards, CharacterAppearance appearance)
{
	return Npc(x, y, name, decks, challenge, rewards, NpcKind::Boss, appearance);
}

bool Npc::isDuelist() const
{
	return duelEnabled;
}

bool Npc::isTownNpc() const
{
	return kind == NpcKind::Town;
}

bool Npc::isRouteDuelist() const
{
	return kind == NpcKind::RouteDuelist;
}

bool Npc::isBoss() const
{
	return kind == NpcKind::Boss;
}

bool Npc::canTrade() const
{
	return tradeEnabled;
}

bool Npc::canWander() const
{
	return wanders;
}

bool Npc::canBattle() const
{
	return duelEnabled && wins < maxWins;
}

bool Npc::isComplete() const
{
	return duelEnabled && wins >= maxWins;
}

std::string Npc::deckForBattle(int battleIndex) const
{
	if (decks.empty()) return "";
	return decks[std::max(0, std::min((int)decks.size() - 1, battleIndex))];
}

std::string Npc::battleDeck() const
{
	return deckForBattle(wins);
}

NpcReward Npc::nextReward() const
{
	if (wins < 0 || wins >= (int)rewards.size()) return { "", 0 };
	return rewards[wins];
}

std::string Npc::rankName() const
{
	if (isBoss()) return "Boss Duel";
	if (isRouteDuelist()) return wins == 0 ? "Route Challenge" : "Route Rematch";
	const char* ranks[] = { "First Trial", "Adaptation", "Veteran Duel", "Master Duel" };
	return ranks[std::max(0, std::min(3, wins))];
}

std::string Npc::dialogueText(const std::string& key, const std::string& fallback) const
{
	std::map<std::string, std::string>::const_iterator found = dialogue.find(key);
	return found == dialogue.end() ? fallback : found->second;
}

bool Npc::isMoving() const
{
	return std::fabs(visualX - x) > 0.001f || std::fabs(visualY - y) > 0.001f;
}

void Npc::setPosition(int xValue, int yValue)
{
	x = homeX = xValue;
	y = homeY = yValue;
	visualX = (float)xValue;
	visualY = (float)yValue;
	nextMoveAt = 0;
	mWanderState = (unsigned int)(xValue * 73856093u) ^
		(unsigned int)(yValue * 19349663u) ^ (unsigned int)name.size() * 83492791u;
}

void Npc::updateMovement(unsigned int deltaMilliseconds, float tilesPerSecond)
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
	const float step = tilesPerSecond * deltaMilliseconds / 1000.f;
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

bool loadNpcsFromLua(const std::string& path, std::vector<Npc>& npcs, std::string& error)
{
	npcs.clear();
	error.clear();
	lua_State* state = luaL_newstate();
	if (state == NULL)
	{
		error = "could not create Lua state";
		return false;
	}
	luaL_openlibs(state);
	if (luaL_loadfile(state, path.c_str()) != LUA_OK || lua_pcall(state, 0, 1, 0) != LUA_OK)
	{
		const char* message = lua_tostring(state, -1);
		error = message == NULL ? "unknown Lua error" : message;
		lua_close(state);
		return false;
	}
	if (!lua_istable(state, -1))
	{
		error = "metadata file must return an array of NPC tables";
		lua_close(state);
		return false;
	}

	std::set<std::string> ids;
	std::set<std::string> names;
	std::set<std::string> crests;
	const size_t count = lua_rawlen(state, -1);
	for (size_t index = 1; index <= count; ++index)
	{
		lua_rawgeti(state, -1, (lua_Integer)index);
		if (!lua_istable(state, -1))
		{
			error = "entry " + std::to_string(index) + " is not a table";
			lua_close(state);
			return false;
		}
		const int entry = lua_gettop(state);
		const std::string id = luaStringField(state, entry, "id");
		const std::string crestId = luaStringField(state, entry, "crest");
		const std::string name = luaStringField(state, entry, "name");
		const std::string kindName = luaStringField(state, entry, "kind");
		const std::string appearanceName = luaStringField(state, entry, "appearance");
		NpcKind kind = NpcKind::Town;
		CharacterAppearance appearance = CharacterAppearance::Mira;
		if (id.empty() || name.empty() || !parseKind(kindName, kind) ||
			!parseAppearance(appearanceName, appearance))
		{
			error = "entry " + std::to_string(index) +
				" needs valid id, name, kind, and appearance fields";
			lua_close(state);
			return false;
		}
		if (!ids.insert(id).second || !names.insert(name).second)
		{
			error = "duplicate NPC id or name at entry " + std::to_string(index);
			lua_close(state);
			return false;
		}
		bool duelEnabled = kind != NpcKind::Town;
		bool tradeEnabled = false;
		bool wanders = kind == NpcKind::Town;
		lua_getfield(state, entry, "options");
		if (lua_istable(state, -1))
		{
			duelEnabled = luaBooleanField(state, -1, "duel", duelEnabled);
			tradeEnabled = luaBooleanField(state, -1, "trade", false);
			wanders = luaBooleanField(state, -1, "wander", wanders);
		}
		lua_pop(state, 1);
		if (kind != NpcKind::Town && (!duelEnabled || tradeEnabled))
		{
			error = "NPC '" + id + "' reserves duel/trade options for town_npc entries";
			lua_close(state);
			return false;
		}
		if (!crestId.empty() && (!duelEnabled || !validCrestId(crestId) ||
			!crests.insert(crestId).second))
		{
			error = "NPC '" + id + "' has an invalid or duplicate crest";
			lua_close(state);
			return false;
		}

		const int x = 0;
		const int y = 0;

		int sightRange = 0;
		int facingX = 0;
		int facingY = 1;
		if (kind == NpcKind::RouteDuelist)
		{
			lua_getfield(state, entry, "sight");
			if (!lua_istable(state, -1))
			{
				error = "route duelist '" + id + "' needs a sight table";
				lua_close(state);
				return false;
			}
			sightRange = luaIntegerField(state, -1, "range", 6);
			std::string direction = luaStringField(state, -1, "direction");
			lua_pop(state, 1);
			if (sightRange < 1 || sightRange > 12 || !parseFacing(direction, facingX, facingY))
			{
				error = "route duelist '" + id + "' needs sight range 1-12 and a cardinal direction";
				lua_close(state);
				return false;
			}
		}

		const int defaultBattles = kind == NpcKind::Boss ? 1 : (duelEnabled ? 4 : 0);
		const int maxBattles = luaIntegerField(state, entry, "max_battles", defaultBattles);
		std::vector<std::string> decks;
		std::vector<NpcReward> rewards;
		std::string latestDeck;
		if (duelEnabled)
		{
			if (maxBattles <= 0 || maxBattles > 4)
			{
				error = "NPC '" + id + "' must have between one and four battles";
				lua_close(state);
				return false;
			}
			for (int battle = 1; battle <= maxBattles; ++battle)
			{
				const std::string suffix = std::to_string(battle);
				const std::string configuredDeck = luaStringField(
					state, entry, ("deck" + suffix).c_str());
				if (!configuredDeck.empty())
				{
					std::string resolvedDeck;
					if (!resolveDeckPath(configuredDeck, resolvedDeck))
					{
						error = "NPC '" + id + "' cannot find deck for battle " + suffix +
							" directly or beneath Decks/: " + configuredDeck;
						lua_close(state);
						return false;
					}
					latestDeck = resolvedDeck;
				}
				if (latestDeck.empty())
				{
					error = "NPC '" + id + "' must define deck1";
					lua_close(state);
					return false;
				}
				decks.push_back(latestDeck);

				const std::string rewardKey = "reward" + suffix;
				lua_getfield(state, entry, rewardKey.c_str());
				if (!lua_istable(state, -1))
				{
					error = "NPC '" + id + "' needs " + rewardKey;
					lua_close(state);
					return false;
				}
				NpcReward reward = {
					luaStringField(state, -1, "card"),
					luaIntegerField(state, -1, "gold", 0)
				};
				lua_pop(state, 1);
				if (reward.card.empty() || getCardIdFromName(reward.card) < 0 || reward.gold < 0)
				{
					error = "NPC '" + id + "' has invalid " + rewardKey;
					lua_close(state);
					return false;
				}
				rewards.push_back(reward);
			}
		}

		lua_getfield(state, entry, "ai");
		const std::string aiPersonality = lua_istable(state, -1) ?
			luaStringField(state, -1, "personality", "balanced") : "none";
		lua_pop(state, 1);
		std::map<std::string, std::string> dialogue;
		readDialogue(state, entry, dialogue);
		const std::string greeting = dialogue.count("greeting") ? dialogue["greeting"] : "";

		Npc npc = kind == NpcKind::Town ?
			Npc::town(x, y, name, decks, greeting, rewards, appearance,
				duelEnabled, tradeEnabled, wanders) :
			(kind == NpcKind::Boss ?
				Npc::boss(x, y, name, decks, greeting, rewards, appearance) :
				Npc::routeDuelist(x, y, name, decks, greeting, rewards, appearance,
					sightRange, facingX, facingY));
		npc.id = id;
		npc.crestId = crestId;
		npc.aiPersonality = aiPersonality;
		npc.dialogue = dialogue;
		npcs.push_back(npc);
		lua_pop(state, 1);
	}
	lua_close(state);
	if (npcs.empty())
	{
		error = "metadata file contains no NPCs";
		return false;
	}
	return true;
}
