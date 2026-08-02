#include "Application.h"

#include "AppSupport.h"

using namespace AppSupport;

namespace
{
	const SDL_Rect RESUME_BUTTON = { 470, 282, 340, 58 };
	const SDL_Rect DECK_BUILDER_BUTTON = { 470, 354, 340, 58 };
	const SDL_Rect SETTINGS_BUTTON = { 470, 426, 340, 58 };
	const SDL_Rect QUIT_BUTTON = { 470, 498, 340, 58 };
	const SDL_Rect SETTINGS_BACK_BUTTON = { 42, 718, 180, 50 };
}

void Application::handlePauseMenuEvent(const SDL_Event& event)
{
	if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym == SDLK_ESCAPE)
	{
		mPauseMenuOpen = false;
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(RESUME_BUTTON, x, y)) mPauseMenuOpen = false;
	else if (contains(DECK_BUILDER_BUTTON, x, y)) enterDeckBuilder();
	else if (contains(SETTINGS_BUTTON, x, y))
	{
		mPauseMenuOpen = false;
		mScreen = Screen::Settings;
	}
	else if (contains(QUIT_BUTTON, x, y)) mRunning = false;
}

void Application::renderPauseMenu()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 4, 7, 13, 190);
	fillRect({ 430, 190, 420, 410 }, 17, 24, 38, 250);
	outlineRect({ 430, 190, 420, 410 }, 197, 151, 65, 255, 4);
	drawText("PAUSED", 566, 219, color(244, 207, 112), 34);

	auto drawButton = [this](const SDL_Rect& rect, const std::string& label, bool danger)
	{
		fillRect(rect, danger ? 83 : 34, danger ? 39 : 50, danger ? 42 : 75, 250);
		outlineRect(rect, danger ? 210 : 112, danger ? 91 : 149, danger ? 82 : 205, 255, 2);
		drawText(label, rect.x + 26, rect.y + 16, color(238, 241, 247), 19);
	};
	drawButton(RESUME_BUTTON, "Resume", false);
	drawButton(DECK_BUILDER_BUTTON, "Deck Builder", false);
	drawButton(SETTINGS_BUTTON, "Settings", false);
	drawButton(QUIT_BUTTON, "Quit Game", true);
	drawText("Esc: resume", 586, 570, color(161, 178, 202), 13);
}

void Application::handleSettingsEvent(const SDL_Event& event)
{
	if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym == SDLK_ESCAPE)
	{
		mScreen = Screen::Overworld;
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(SETTINGS_BACK_BUTTON, x, y)) mScreen = Screen::Overworld;
}

void Application::renderSettings()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 12, 19, 31);
	drawText("SETTINGS", 52, 44, color(244, 207, 112), 38);
	fillRect({ 42, 112, 1196, 565 }, 21, 29, 45, 245);
	outlineRect({ 42, 112, 1196, 565 }, 115, 145, 190, 255, 2);
	drawText("Settings will be added here later.", 420, 365, color(178, 194, 217), 20);
	fillRect(SETTINGS_BACK_BUTTON, 35, 50, 75, 250);
	outlineRect(SETTINGS_BACK_BUTTON, 112, 149, 205, 255, 2);
	drawText("Back", SETTINGS_BACK_BUTTON.x + 58, SETTINGS_BACK_BUTTON.y + 13,
		color(238, 241, 247), 18);
}

bool Application::exerciseMenuScreensSmoke()
{
	mScreen = Screen::Overworld;
	mPauseMenuOpen = true;
	renderOverworld();
	SDL_Event openDecks = {};
	openDecks.type = SDL_MOUSEBUTTONDOWN;
	openDecks.button.button = SDL_BUTTON_LEFT;
	openDecks.button.x = DECK_BUILDER_BUTTON.x + DECK_BUILDER_BUTTON.w / 2;
	openDecks.button.y = DECK_BUILDER_BUTTON.y + DECK_BUILDER_BUTTON.h / 2;
	handleOverworldEvent(openDecks);
	if (mScreen != Screen::DeckBuilder) return false;
	renderDeckBuilder();
	mMouseX = 300;
	mMouseY = 200;
	renderDeckBuilder();
	if (mDeckHoveredCard < 0) return false;
	mMouseX = 1000;
	mMouseY = 210;
	renderDeckBuilder();
	if (mDeckHoveredCard < 0) return false;
	SDL_Event back = {};
	back.type = SDL_KEYDOWN;
	back.key.keysym.sym = SDLK_ESCAPE;
	handleDeckBuilderEvent(back);
	if (mScreen != Screen::Overworld) return false;

	mPauseMenuOpen = true;
	SDL_Event openSettings = {};
	openSettings.type = SDL_MOUSEBUTTONDOWN;
	openSettings.button.button = SDL_BUTTON_LEFT;
	openSettings.button.x = SETTINGS_BUTTON.x + SETTINGS_BUTTON.w / 2;
	openSettings.button.y = SETTINGS_BUTTON.y + SETTINGS_BUTTON.h / 2;
	handleOverworldEvent(openSettings);
	if (mScreen != Screen::Settings) return false;
	renderSettings();
	handleSettingsEvent(back);
	if (mScreen != Screen::Overworld) return false;

	enterShop();
	renderShop();
	if (mScreen != Screen::Shop || mShopCardHitboxes.size() != 10) return false;
	handleShopEvent(back);
	return mScreen == Screen::Overworld;
}
