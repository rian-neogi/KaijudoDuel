cmake_minimum_required(VERSION 3.16)

project(KaijudoDuel LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(SDL2_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2)
find_library(SDL2_LIBRARY NAMES SDL2)
find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h PATH_SUFFIXES SDL2)
find_library(SDL2_IMAGE_LIBRARY NAMES SDL2_image)
find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h PATH_SUFFIXES SDL2)
find_library(SDL2_TTF_LIBRARY NAMES SDL2_ttf)
find_package(Lua REQUIRED)
find_package(Threads REQUIRED)

if(NOT SDL2_INCLUDE_DIR OR NOT SDL2_LIBRARY
   OR NOT SDL2_IMAGE_INCLUDE_DIR OR NOT SDL2_IMAGE_LIBRARY
   OR NOT SDL2_TTF_INCLUDE_DIR OR NOT SDL2_TTF_LIBRARY)
    message(FATAL_ERROR "SDL2, SDL2_image, and SDL2_ttf development packages are required")
endif()

set(GAME_SOURCES
    Source/Main.cpp
	Source/AI/HeuristicBot.cpp
    Source/App/Application.cpp
	Source/App/AssetManager.cpp
	Source/App/CardRenderer.cpp
	Source/App/CatalogMapStorage.cpp
    Source/App/DuelWindow.cpp
	Source/App/DeckBuilder.cpp
	Source/App/DeprecatedWorldTileRenderer.cpp
	Source/App/Menus.cpp
	Source/App/MercerStock.cpp
	Source/App/Npc.cpp
	Source/App/Overworld.cpp
	Source/App/RtpTilesetRenderer.cpp
	Source/App/SaveMenu.cpp
	Source/App/SmokeTests.cpp
	Source/App/Shop.cpp
	Source/App/SpriteSheetRenderer.cpp
	Source/App/Story.cpp
	Source/App/WorldBuilder.cpp
	Source/App/WorldData.cpp
	Source/App/WorldStorage.cpp
	Source/App/WorldObject.cpp
	Source/App/WorldTileRenderer.cpp
    Source/CRandom.cpp
    Source/LuaFunctions.cpp
	Source/LuaTrace.cpp
    Source/SoundManager.cpp
    Source/Game/BattleZone.cpp
    Source/Game/Card.cpp
    Source/Game/CardData.cpp
    Source/Game/Choice.cpp
    Source/Game/Deck.cpp
    Source/Game/Duel.cpp
    Source/Game/Graveyard.cpp
    Source/Game/Hand.cpp
    Source/Game/ManaZone.cpp
    Source/Game/Message.cpp
    Source/Game/MessageManager.cpp
    Source/Game/Modifier.cpp
    Source/Game/ShieldZone.cpp
    Source/Game/Zone.cpp
)

add_executable(KaijudoDuel ${GAME_SOURCES})

target_include_directories(KaijudoDuel PRIVATE
    Source
    ${SDL2_INCLUDE_DIR}
    ${SDL2_IMAGE_INCLUDE_DIR}
    ${SDL2_TTF_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(KaijudoDuel PRIVATE
    ${SDL2_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
    ${SDL2_TTF_LIBRARY}
    ${LUA_LIBRARIES}
    Threads::Threads
)

target_compile_options(KaijudoDuel PRIVATE -Wall -Wextra -Wpedantic)

set_target_properties(KaijudoDuel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin"
)

if(BUILD_TESTING)
	find_package(Python3 QUIET COMPONENTS Interpreter)
    add_test(NAME cli_help
        COMMAND KaijudoDuel --help
    )
    add_test(NAME smoke
        COMMAND KaijudoDuel --smoke-test
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(smoke PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
		TIMEOUT 60
    )
	if(Python3_Interpreter_FOUND)
		add_test(NAME legacy_world_converter
			COMMAND ${Python3_EXECUTABLE}
				"${CMAKE_SOURCE_DIR}/Tools/test_convert_legacy_world.py"
		)
	endif()
endif()
