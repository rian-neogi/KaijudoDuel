#include "Application.h"

#include "AppSupport.h"
#include "SoundManager.h"

#include <algorithm>

using namespace AppSupport;

namespace
{
	const SDL_Rect RESUME_BUTTON = { 470, 282, 340, 58 };
	const SDL_Rect DECK_BUILDER_BUTTON = { 470, 354, 340, 58 };
	const SDL_Rect SETTINGS_BUTTON = { 470, 426, 340, 58 };
	const SDL_Rect QUIT_BUTTON = { 470, 498, 340, 58 };
	const SDL_Rect SETTINGS_BACK_BUTTON = { 42, 718, 180, 50 };
	const SDL_Rect SETTINGS_PANEL = { 150, 126, 980, 520 };
	const SDL_Rect MUSIC_SLIDER = { 445, 246, 555, 12 };
	const SDL_Rect SOUND_SLIDER = { 445, 356, 555, 12 };
	const SDL_Rect AUTO_ACTION_TOGGLE = { 445, 472, 92, 42 };

	SDL_Rect sliderHitbox(const SDL_Rect& slider)
	{
		return { slider.x - 12, slider.y - 20, slider.w + 24, 52 };
	}
}

void Application::handlePauseMenuEvent(const SDL_Event& event)
{
	auto activateSelection = [this]()
	{
		if (mPauseMenuSelection == 0) mPauseMenuOpen = false;
		else if (mPauseMenuSelection == 1) enterDeckBuilder();
		else if (mPauseMenuSelection == 2)
		{
			mPauseMenuOpen = false;
			mScreen = Screen::Settings;
		}
		else if (mPauseMenuSelection == 3) mRunning = false;
	};
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			mPauseMenuOpen = false;
			return;
		}
		if (key == SDLK_w || key == SDLK_UP)
		{
			mPauseMenuSelection = (mPauseMenuSelection + 3) % 4;
			return;
		}
		if (key == SDLK_s || key == SDLK_DOWN)
		{
			mPauseMenuSelection = (mPauseMenuSelection + 1) % 4;
			return;
		}
		if (key == SDLK_RETURN || key == SDLK_SPACE)
		{
			activateSelection();
			return;
		}
	}
	if (event.type == SDL_MOUSEMOTION)
	{
		int x, y;
		logicalMouse(event.motion.x, event.motion.y, x, y);
		if (contains(RESUME_BUTTON, x, y)) mPauseMenuSelection = 0;
		else if (contains(DECK_BUILDER_BUTTON, x, y)) mPauseMenuSelection = 1;
		else if (contains(SETTINGS_BUTTON, x, y)) mPauseMenuSelection = 2;
		else if (contains(QUIT_BUTTON, x, y)) mPauseMenuSelection = 3;
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(RESUME_BUTTON, x, y)) mPauseMenuSelection = 0;
	else if (contains(DECK_BUILDER_BUTTON, x, y)) mPauseMenuSelection = 1;
	else if (contains(SETTINGS_BUTTON, x, y)) mPauseMenuSelection = 2;
	else if (contains(QUIT_BUTTON, x, y)) mPauseMenuSelection = 3;
	else return;
	activateSelection();
}

void Application::renderPauseMenu()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 4, 7, 13, 190);
	fillRect({ 430, 190, 420, 410 }, 17, 24, 38, 250);
	outlineRect({ 430, 190, 420, 410 }, 197, 151, 65, 255, 4);
	drawText("PAUSED", 566, 219, color(244, 207, 112), 34);

	auto drawButton = [this](const SDL_Rect& rect, const std::string& label, bool danger,
		int selection)
	{
		bool selected = mPauseMenuSelection == selection;
		fillRect(rect, danger ? (selected ? 113 : 83) : (selected ? 54 : 34),
			danger ? (selected ? 48 : 39) : (selected ? 73 : 50),
			danger ? (selected ? 50 : 42) : (selected ? 106 : 75), 250);
		outlineRect(rect, danger ? (selected ? 244 : 210) : (selected ? 137 : 112),
			danger ? (selected ? 113 : 91) : (selected ? 186 : 149),
			danger ? (selected ? 101 : 82) : (selected ? 237 : 205), 255,
			selected ? 3 : 2);
		drawText(label, rect.x + 26, rect.y + 16, color(238, 241, 247), 19);
	};
	drawButton(RESUME_BUTTON, "Resume", false, 0);
	drawButton(DECK_BUILDER_BUTTON, "Deck Builder", false, 1);
	drawButton(SETTINGS_BUTTON, "Settings", false, 2);
	drawButton(QUIT_BUTTON, "Quit Game", true, 3);
	drawText("W/S/Arrows + Enter  •  Esc: resume", 500, 570,
		color(161, 178, 202), 12);
}

void Application::handleSettingsEvent(const SDL_Event& event)
{
	if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym == SDLK_ESCAPE)
	{
		mSettingsDraggingSlider = 0;
		saveSettings();
		mScreen = Screen::Overworld;
		return;
	}

	auto updateSlider = [this](int slider, int mouseX)
	{
		const SDL_Rect& track = slider == 1 ? MUSIC_SLIDER : SOUND_SLIDER;
		int value = std::max(0, std::min(100,
			(mouseX - track.x) * 100 / track.w));
		if (slider == 1)
		{
			mMusicVolume = value;
			if (mSoundManager != NULL) mSoundManager->setMusicVolume(value);
		}
		else
		{
			mSoundVolume = value;
			if (mSoundManager != NULL) mSoundManager->setSoundVolume(value);
		}
	};

	if (event.type == SDL_MOUSEMOTION && mSettingsDraggingSlider != 0)
	{
		int x, y;
		logicalMouse(event.motion.x, event.motion.y, x, y);
		updateSlider(mSettingsDraggingSlider, x);
		return;
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		if (mSettingsDraggingSlider != 0)
		{
			mSettingsDraggingSlider = 0;
			saveSettings();
		}
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(SETTINGS_BACK_BUTTON, x, y))
	{
		mSettingsDraggingSlider = 0;
		saveSettings();
		mScreen = Screen::Overworld;
		return;
	}
	if (contains(sliderHitbox(MUSIC_SLIDER), x, y))
	{
		mSettingsDraggingSlider = 1;
		updateSlider(1, x);
		return;
	}
	if (contains(sliderHitbox(SOUND_SLIDER), x, y))
	{
		mSettingsDraggingSlider = 2;
		updateSlider(2, x);
		return;
	}
	if (contains(AUTO_ACTION_TOGGLE, x, y))
	{
		mAutoChooseOnlyAction = !mAutoChooseOnlyAction;
		saveSettings();
	}
}

void Application::renderSettings()
{
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 12, 19, 31);
	drawText("SETTINGS", 52, 44, color(244, 207, 112), 38);
	fillRect(SETTINGS_PANEL, 21, 29, 45, 245);
	outlineRect(SETTINGS_PANEL, 115, 145, 190, 255, 2);

	auto drawSlider = [this](const SDL_Rect& track, int value, const std::string& label)
	{
		drawText(label, 220, track.y - 12, color(232, 237, 246), 20);
		fillRect(track, 47, 60, 82, 255);
		SDL_Rect filled = track;
		filled.w = track.w * value / 100;
		fillRect(filled, 100, 166, 225, 255);
		int knobX = track.x + track.w * value / 100;
		fillRect({ knobX - 8, track.y - 9, 16, 30 }, 221, 232, 245, 255);
		outlineRect({ knobX - 8, track.y - 9, 16, 30 }, 78, 112, 151, 255, 2);
		drawText(std::to_string(value) + "%", 1025, track.y - 12,
			color(177, 200, 226), 18);
	};
	drawSlider(MUSIC_SLIDER, mMusicVolume, "Music volume");
	drawSlider(SOUND_SLIDER, mSoundVolume, "Sound volume");

	drawText("Auto-choose only duel action", 220, 480, color(232, 237, 246), 20);
	fillRect(AUTO_ACTION_TOGGLE,
		mAutoChooseOnlyAction ? 47 : 55,
		mAutoChooseOnlyAction ? 137 : 64,
		mAutoChooseOnlyAction ? 89 : 78, 255);
	outlineRect(AUTO_ACTION_TOGGLE,
		mAutoChooseOnlyAction ? 104 : 112,
		mAutoChooseOnlyAction ? 213 : 119,
		mAutoChooseOnlyAction ? 144 : 133, 255, 2);
	drawText(mAutoChooseOnlyAction ? "ON" : "OFF",
		AUTO_ACTION_TOGGLE.x + (mAutoChooseOnlyAction ? 29 : 22),
		AUTO_ACTION_TOGGLE.y + 11, color(241, 245, 249), 17);
	drawText("When exactly one legal action is available, the duel will choose it after a short pause.",
		565, 477, color(161, 178, 202), 15, 485);
	drawText("Drag either slider to adjust it. Changes are saved automatically.",
		220, 580, color(139, 159, 185), 14);
	fillRect(SETTINGS_BACK_BUTTON, 35, 50, 75, 250);
	outlineRect(SETTINGS_BACK_BUTTON, 112, 149, 205, 255, 2);
	drawText("Back", SETTINGS_BACK_BUTTON.x + 58, SETTINGS_BACK_BUTTON.y + 13,
		color(238, 241, 247), 18);
}
