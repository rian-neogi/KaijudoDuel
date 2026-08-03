#include "App/Application.h"
#include "Game/Card.h"
#include "LuaTrace.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
	void printUsage(const char* executable)
	{
		std::cout
			<< "Usage:\n"
			<< "  " << executable << "\n"
			<< "  " << executable << " [--lua-trace] --duel <player-deck> <ai-deck>\n"
			<< "  " << executable << " --world-builder\n"
			<< "  " << executable << " [--lua-trace] --smoke-test\n"
			<< "  " << executable << " --help\n\n"
			<< "Decks are searched beneath Decks/ by default; quote paths containing spaces.\n"
			<< "The player deck is listed first.\n"
			<< "--lua-trace writes a rolling trace to Logs/lua-trace.log.\n";
	}
}

int main(int argc, char* argv[])
{
	bool smokeTest = false;
	bool luaTrace = false;
	bool worldBuilder = false;
	std::string playerDeck;
	std::string aiDeck;
	std::vector<std::string> arguments;
	for (int i = 1; i < argc; i++)
	{
		if (std::string(argv[i]) == "--lua-trace") luaTrace = true;
		else arguments.push_back(argv[i]);
	}
	const char* traceEnvironment = std::getenv("KAIJUDO_LUA_TRACE");
	if (traceEnvironment != NULL && std::string(traceEnvironment) != "0") luaTrace = true;
	LuaTrace::setEnabled(luaTrace);
	if (luaTrace)
		std::cerr << "Lua trace enabled: Logs/lua-trace.log" << std::endl;

	if (arguments.size() == 1 && (arguments[0] == "--help" || arguments[0] == "-h"))
	{
		printUsage(argv[0]);
		return 0;
	}
	if (arguments.size() == 1 && arguments[0] == "--smoke-test")
		smokeTest = true;
	else if (arguments.size() == 1 && arguments[0] == "--world-builder")
		worldBuilder = true;
	else if (arguments.size() == 3 && arguments[0] == "--duel")
	{
		playerDeck = arguments[1];
		aiDeck = arguments[2];
	}
	else if (!arguments.empty())
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
		Application application(worldBuilder);
		result = application.run(smokeTest, playerDeck, aiDeck, worldBuilder);
	}
	cleanupCards();
	return result;
}
