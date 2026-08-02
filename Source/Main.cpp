#include "App/Application.h"
#include "Game/Card.h"

#include <iostream>
#include <string>

namespace
{
	void printUsage(const char* executable)
	{
		std::cout
			<< "Usage:\n"
			<< "  " << executable << "\n"
			<< "  " << executable << " --duel <player-deck> <ai-deck>\n"
			<< "  " << executable << " --smoke-test\n"
			<< "  " << executable << " --help\n\n"
			<< "Decks are searched beneath Decks/ by default; quote paths containing spaces.\n"
			<< "The player deck is listed first.\n";
	}
}

int main(int argc, char* argv[])
{
	bool smokeTest = false;
	std::string playerDeck;
	std::string aiDeck;
	if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))
	{
		printUsage(argv[0]);
		return 0;
	}
	if (argc == 2 && std::string(argv[1]) == "--smoke-test")
		smokeTest = true;
	else if (argc == 4 && std::string(argv[1]) == "--duel")
	{
		playerDeck = argv[2];
		aiDeck = argv[3];
	}
	else if (argc != 1)
	{
		std::cerr << "Invalid command-line arguments.\n\n";
		printUsage(argv[0]);
		return 2;
	}

	if (!initCards())
	{
		std::cerr << "Unable to initialize the Lua card database." << std::endl;
		return 1;
	}

	int result = 0;
	{
		Application application;
		result = application.run(smokeTest, playerDeck, aiDeck);
	}
	cleanupCards();
	return result;
}
