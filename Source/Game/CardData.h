#pragma once

#include "Modifier.h"

#include <vector>

class CardData
{
public:
	int CardId;
	std::string Name;
	std::string Set;
	std::string Race;
	int Civilization;
	int Type;
	int ManaCost;
	int Power;
	int PriceTier;

	CardData();
	CardData(int id, std::string n, std::string s, std::string r, int civ, int type,
		int cost, int power, int priceTier);
	~CardData();
};

extern std::vector<CardData> gCardDatabase;

