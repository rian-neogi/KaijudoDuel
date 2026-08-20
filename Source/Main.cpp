#include "App/Application.h"
#include "AI/AiMatchRunner.h"
#include "AI/AiParams.h"
#include "CRandom.h"
#include "Game/Card.h"
#include "LuaTrace.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
	void printUsage(const char* executable)
	{
		std::cout
			<< "Usage:\n"
			<< "  " << executable << "\n"
			<< "  " << executable << " [--lua-trace] [--full-visibility] --duel "
				"<player-deck> <ai-deck> [--ai-personality NAME] [--ai-difficulty NAME]\n"
			<< "  " << executable << " [--lua-trace] [--seed N] --ai-duel "
				"<deck-0> <deck-1> [--ai-personality NAME] [--ai-difficulty NAME]\n"
			<< "  " << executable << " [--lua-trace] [--seed N] [--max-actions N] "
				"--headless-ai-duel <deck-0> <deck-1> [--ai-personality NAME] "
				"[--ai-difficulty NAME]\n"
			<< "  " << executable << " --world-builder\n"
			<< "  " << executable << " [--lua-trace] --smoke-test\n"
			<< "  " << executable << " --help\n\n"
			<< "Decks are searched beneath Decks/ by default; quote paths containing spaces.\n"
			<< "The player deck is listed first.\n"
			<< "--full-visibility reveals both hands in direct-duel mode.\n"
			<< "AI personalities: default (base parameters), rush, tempo (preset), control.\n"
			<< "AI difficulties: easy, medium (default), hard.\n"
			<< "--ai-duel renders both AI players and reveals both hands.\n"
			<< "--headless-ai-duel runs without SDL and prints one AI_MATCH_RESULT line.\n"
			<< "Duels use a fresh random seed unless --seed is provided for an AI duel.\n"
			<< "--max-actions bounds headless matches (default 10000).\n"
			<< "--lua-trace writes real-duel Lua calls (excluding MCTS simulations) "
			<< "to Logs/lua-trace.log.\n";
	}

	bool parsePositiveUnsigned(const std::string& text, unsigned long maximum,
		unsigned long& value)
	{
		if (text.empty() || text[0] == '-') return false;
		errno = 0;
		char* end = NULL;
		unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
		if (errno != 0 || end == text.c_str() || *end != '\0' || parsed == 0 ||
			parsed > maximum)
			return false;
		value = parsed;
		return true;
	}
}

int main(int argc, char* argv[])
{
	bool smokeTest = false;
	bool luaTrace = false;
	bool fullVisibility = false;
	bool worldBuilder = false;
	bool aiDuel = false;
	bool headlessAiDuel = false;
	bool seedSpecified = false;
	bool maxActionsSpecified = false;
	bool aiPersonalitySpecified = false;
	bool aiDifficultySpecified = false;
	bool invalidOption = false;
	std::uint32_t duelSeed = 0;
	int maxActions = 10000;
	std::string playerDeck;
	std::string aiDeck;
	std::string aiPersonality = "tempo";
	std::string aiDifficulty = "medium";
	std::vector<std::string> arguments;
	for (int i = 1; i < argc; i++)
	{
		if (std::string(argv[i]) == "--lua-trace") luaTrace = true;
		else if (std::string(argv[i]) == "--full-visibility") fullVisibility = true;
		else if (std::string(argv[i]) == "--seed")
		{
			unsigned long parsed = 0;
			if (i + 1 >= argc || !parsePositiveUnsigned(argv[++i],
				std::numeric_limits<std::uint32_t>::max(), parsed))
				invalidOption = true;
			else
			{
				duelSeed = static_cast<std::uint32_t>(parsed);
				seedSpecified = true;
			}
		}
		else if (std::string(argv[i]) == "--max-actions")
		{
			unsigned long parsed = 0;
			if (i + 1 >= argc || !parsePositiveUnsigned(argv[++i],
				static_cast<unsigned long>(std::numeric_limits<int>::max()), parsed))
				invalidOption = true;
			else
			{
				maxActions = static_cast<int>(parsed);
				maxActionsSpecified = true;
			}
		}
		else if (std::string(argv[i]) == "--ai-personality" ||
			std::string(argv[i]) == "--personality")
		{
			if (i + 1 >= argc) invalidOption = true;
			else
			{
				aiPersonality = argv[++i];
				aiPersonalitySpecified = true;
			}
		}
		else if (std::string(argv[i]) == "--ai-difficulty" ||
			std::string(argv[i]) == "--difficulty")
		{
			if (i + 1 >= argc) invalidOption = true;
			else
			{
				aiDifficulty = argv[++i];
				aiDifficultySpecified = true;
			}
		}
		else arguments.push_back(argv[i]);
	}
	const char* traceEnvironment = std::getenv("KAIJUDO_LUA_TRACE");
	if (traceEnvironment != NULL && std::string(traceEnvironment) != "0") luaTrace = true;
	LuaTrace::setEnabled(luaTrace);
	if (luaTrace)
		std::cerr << "Lua trace enabled: Logs/lua-trace.log" << std::endl;
	if (invalidOption)
	{
		std::cerr << "Invalid command-line option value.\n\n";
		printUsage(argv[0]);
		return 2;
	}

	if (arguments.size() == 1 &&
		(arguments[0] == "--help" || arguments[0] == "-h"))
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
	else if (arguments.size() == 3 && arguments[0] == "--ai-duel")
	{
		aiDuel = true;
		playerDeck = arguments[1];
		aiDeck = arguments[2];
	}
	else if (arguments.size() == 3 && arguments[0] == "--headless-ai-duel")
	{
		headlessAiDuel = true;
		playerDeck = arguments[1];
		aiDeck = arguments[2];
	}
	else if (!arguments.empty() || invalidOption)
	{
		std::cerr << "Invalid command-line arguments.\n\n";
		printUsage(argv[0]);
		return 2;
	}
	if (fullVisibility && (playerDeck.empty() || aiDeck.empty() || headlessAiDuel))
	{
		std::cerr << "--full-visibility is available only with rendered direct duels.\n\n";
		printUsage(argv[0]);
		return 2;
	}
	if (seedSpecified && !aiDuel && !headlessAiDuel)
	{
		std::cerr << "--seed is available only with AI-vs-AI modes.\n\n";
		printUsage(argv[0]);
		return 2;
	}
	if (maxActionsSpecified && !headlessAiDuel)
	{
		std::cerr << "--max-actions is available only with --headless-ai-duel.\n\n";
		printUsage(argv[0]);
		return 2;
	}
	if ((aiPersonalitySpecified || aiDifficultySpecified) &&
		playerDeck.empty() && aiDeck.empty())
	{
		std::cerr << "AI profile options require a direct duel mode.\n\n";
		printUsage(argv[0]);
		return 2;
	}
	if (!seedSpecified && !playerDeck.empty() && !aiDeck.empty())
		duelSeed = CRandom::GenerateRandomSeed();

	if (!initCards())
	{
		std::cerr << "Unable to initialize the Lua card database." << std::endl;
		return 1;
	}
	if (!hasAiPersonality(aiPersonality) || !hasAiDifficulty(aiDifficulty))
	{
		std::cerr << "Unknown AI personality or difficulty. Use default/rush/tempo/control "
			"and easy/medium/hard.\n\n";
		printUsage(argv[0]);
		cleanupCards();
		return 2;
	}

	if (headlessAiDuel)
	{
		AiMatchResult match = runHeadlessAiMatch(
			playerDeck, aiDeck, duelSeed, maxActions, aiPersonality, aiDifficulty);
		if (!match.started)
		{
			std::cerr << "Unable to start headless AI duel: " << match.error << std::endl;
			cleanupCards();
			return 3;
		}
		const char* status = match.completed ? "complete" :
			(match.stalled ? "stalled" : "move-limit");
		std::cout << "AI_MATCH_RESULT status=" << status <<
			" winner=" << match.winner << " actions=" << match.actions <<
			" seed=" << duelSeed << " elapsed_ms=" << match.elapsedMs << std::endl;
		if (match.stalled)
		{
			std::cerr << "Headless AI duel stalled: " << match.error << std::endl;
			cleanupCards();
			return 4;
		}
		cleanupCards();
		return 0;
	}

	int result = 0;
	{
		Application application(worldBuilder);
		result = application.run(smokeTest, playerDeck, aiDeck, worldBuilder,
			fullVisibility, aiDuel, duelSeed, aiPersonality, aiDifficulty);
	}
	cleanupCards();
	return result;
}
