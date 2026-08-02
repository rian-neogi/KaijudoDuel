#include "App/Application.h"
#include "Game/Card.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	if (!initCards())
	{
		std::cerr << "Unable to initialize the Lua card database." << std::endl;
		return 1;
	}

	int result = 0;
	{
		Application application;
		bool smokeTest = argc > 1 && std::string(argv[1]) == "--smoke-test";
		result = application.run(smokeTest);
	}
	cleanupCards();
	return result;
}
