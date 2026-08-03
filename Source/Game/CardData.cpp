#include "CardData.h"

std::vector<CardData> gCardDatabase;

CardData::CardData()
{
}

CardData::CardData(int id, std::string n, std::string s, std::string r, int civ,
	int type, int cost, int power, int priceTier)
	: CardId(id), Name(n), Set(s), Race(r), Civilization(civ), Type(type),
	  ManaCost(cost), Power(power), PriceTier(priceTier)
{
}

CardData::~CardData()
{
}
