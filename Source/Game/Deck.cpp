#include "Deck.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
	std::string normalizedDeckCardName(const std::string& name)
	{
		std::string normalized;
		bool separatorPending = false;
		for (size_t i = 0; i < name.size(); ++i)
		{
			unsigned char character = static_cast<unsigned char>(name[i]);
			if (character == ',' || character == '-' || std::isspace(character))
			{
				separatorPending = !normalized.empty();
				continue;
			}
			if (separatorPending)
			{
				normalized.push_back(' ');
				separatorPending = false;
			}
			normalized.push_back(static_cast<char>(std::tolower(character)));
		}
		return normalized;
	}
}

std::string deckLineWithoutComment(const std::string& line)
{
	size_t comment = line.find('#');
	return comment == std::string::npos ? line : line.substr(0, comment);
}

bool resolveDeckPath(const std::string& requestedPath, std::string& resolvedPath)
{
	resolvedPath.clear();
	if (requestedPath.empty()) return false;
	std::ifstream exact(requestedPath.c_str());
	if (exact.good())
	{
		resolvedPath = requestedPath;
		return true;
	}
	const bool absolute = requestedPath[0] == '/' ||
		(requestedPath.size() > 1 && requestedPath[1] == ':');
	const bool alreadyInDecks = requestedPath.compare(0, 6, "Decks/") == 0;
	if (absolute || alreadyInDecks) return false;
	const std::string defaultPath = "Decks/" + requestedPath;
	std::ifstream fallback(defaultPath.c_str());
	if (!fallback.good()) return false;
	resolvedPath = defaultPath;
	return true;
}

int getDeckCardIdFromName(const std::string& name)
{
	int exactMatch = getCardIdFromName(name);
	if (exactMatch >= 0) return exactMatch;

	std::string normalizedName = normalizedDeckCardName(name);
	int match = -1;
	for (size_t card = 0; card < gCardDatabase.size(); ++card)
	{
		if (normalizedDeckCardName(gCardDatabase[card].Name) != normalizedName) continue;
		int canonicalMatch = getCardIdFromName(gCardDatabase[card].Name);
		if (match >= 0 && match != canonicalMatch) return -1;
		match = canonicalMatch;
	}
	return match;
}

bool loadDeckCardIds(const std::string& requestedPath, std::vector<int>& cardIds,
	int minimumCards, std::string* loadedPath)
{
	cardIds.clear();
	std::string resolvedPath;
	if (!resolveDeckPath(requestedPath, resolvedPath))
	{
		fprintf(stderr, "Unable to find deck '%s' directly or beneath Decks/.\n",
			requestedPath.c_str());
		return false;
	}
	std::ifstream file(resolvedPath.c_str());

	std::string line;
	int lineNumber = 0;
	while (std::getline(file, line))
	{
		++lineNumber;
		line = deckLineWithoutComment(line);
		if (!line.empty() && line.back() == '\r') line.pop_back();
		size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos) continue;

		std::istringstream input(line.substr(first));
		int count = 0;
		if (!(input >> count) || count <= 0)
		{
			fprintf(stderr, "Invalid card count in '%s' at line %d.\n",
				resolvedPath.c_str(), lineNumber);
			return false;
		}
		std::string name;
		std::getline(input, name);
		first = name.find_first_not_of(" \t");
		if (first == std::string::npos)
		{
			fprintf(stderr, "Missing card name in '%s' at line %d.\n",
				resolvedPath.c_str(), lineNumber);
			return false;
		}
		name.erase(0, first);
		size_t last = name.find_last_not_of(" \t");
		name.erase(last + 1);
		int cardId = getDeckCardIdFromName(name);
		if (cardId < 0)
		{
			fprintf(stderr, "Unknown card '%s' in '%s' at line %d.\n",
				name.c_str(), resolvedPath.c_str(), lineNumber);
			return false;
		}
		cardIds.insert(cardIds.end(), count, cardId);
	}
	if (file.bad())
	{
		fprintf(stderr, "Unable to finish reading deck '%s'.\n", resolvedPath.c_str());
		return false;
	}

	if ((int)cardIds.size() < minimumCards)
	{
		fprintf(stderr, "Deck '%s' has %d cards; at least %d are required.\n",
			resolvedPath.c_str(), (int)cardIds.size(), minimumCards);
		return false;
	}
	if (loadedPath != NULL) *loadedPath = resolvedPath;
	return true;
}

Deck::Deck()
{
}

Deck::~Deck()
{
}

Card* Deck::draw()
{
	if (mCards.size() == 0) return NULL;
	Card* c = mCards.at(mCards.size()-1);
	mCards.pop_back();
	return c;
}

int Deck::getTopCard()
{
	return mCards.at(mCards.size() - 1)->mUniqueId;
}

//void Deck::renderCards(int myPlayer)
//{
//	for (int i = 0; i < cards.size(); i++)
//	{
//		cards.at(i)->render(myPlayer);
//	}
//}

//void Deck::handleEvent(sf::Event event)
//{
//	for (int i = 0; i < cards.size(); i++)
//	{
//		cards.at(i)->handleEvent(event);
//	}
//}

void Deck::addCard(Card* c)
{
	//c->move(x + CARDZONEOFFSET + CARDORIGINOFFSET, y + CARDZONEOFFSET + CARDORIGINOFFSET);
	//c->setPosition(glm::vec3(mPos.x + (mPos.x + mWidth) / 2, mPos.y + CONST_CARDTHICKNESS*cards.size(), mPos.z + (mPos.z + mHeight) / 2));
	c->flip();
	c->untap();
	c->mIsVisible[0] = false;
	c->mIsVisible[1] = false;
	mCards.push_back(c);
}

void Deck::addCardToBottom(Card* c)
{
	c->flip();
	c->untap();
	c->mIsVisible[0] = false;
	c->mIsVisible[1] = false;
	mCards.insert(mCards.begin(), c);
}

void Deck::shuffle()
{
	for (int i = 0; i < mCards.size(); i++)
	{
		int x = mRandomGen->Random(mCards.size());
		Card* tmp = mCards.at(x);
		mCards.at(x) = mCards.at(i);
		mCards.at(i) = tmp;
	}
}

//void Deck::loadFromFile(string s, int uid)
//{
//	cards.empty();
//	fstream file;
//	file.open(s, ios::in | ios::out);
//	string str;
//
//	int cnt = 0;
//	
//	while (!file.eof())
//	{
//		getline(file, str);
//		Card* c = new Card(uid+cnt, getCardIdFromName(str), owner);
//		addCard(c);
//		cnt++;
//	}
//
//	file.close();
//}
