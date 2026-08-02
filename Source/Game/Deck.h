#pragma once

#include "Hand.h"

#include <string>

bool resolveDeckPath(const std::string& requestedPath, std::string& resolvedPath);
std::string deckLineWithoutComment(const std::string& line);

class Deck : public Zone
{
public:
	Deck();
	~Deck();

	CRandom* mRandomGen;

	Card* draw();
	int getTopCard();

	//void renderCards(int myPlayer);
	//void handleEvent(sf::Event event);
	void addCard(Card* c);
	void addCardToBottom(Card* c);
	void shuffle();
	//void loadFromFile(string s, int uid);
};

