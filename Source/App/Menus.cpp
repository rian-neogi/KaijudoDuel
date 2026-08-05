#include "Application.h"

#include "AppSupport.h"
#include "SoundManager.h"

#include <algorithm>

using namespace AppSupport;

namespace
{
	const SDL_Rect PAUSE_PANEL = { 40, 30, 1200, 740 };
	const SDL_Rect PAUSE_STATUS_PANEL = { 65, 88, 780, 652 };
	const SDL_Rect PAUSE_ACTION_PANEL = { 865, 88, 350, 652 };
	const SDL_Rect RESUME_BUTTON = { 890, 286, 300, 58 };
	const SDL_Rect DECK_BUILDER_BUTTON = { 890, 358, 300, 58 };
	const SDL_Rect SETTINGS_BUTTON = { 890, 430, 300, 58 };
	const SDL_Rect QUIT_BUTTON = { 890, 502, 300, 58 };
	const SDL_Rect SETTINGS_BACK_BUTTON = { 42, 718, 180, 50 };
	const SDL_Rect SETTINGS_PANEL = { 150, 126, 980, 520 };
	const SDL_Rect MUSIC_SLIDER = { 445, 246, 555, 12 };
	const SDL_Rect SOUND_SLIDER = { 445, 356, 555, 12 };
	const SDL_Rect AUTO_ACTION_TOGGLE = { 445, 472, 92, 42 };

	struct CrestDefinition
	{
		const char* id;
		const char* name;
	};

	const CrestDefinition CRESTS[] = {
		{ "dawn", "Dawn" },
		{ "tidal", "Tidal" },
		{ "forge", "Forge" },
		{ "verdant", "Verdant" },
		{ "confluence", "Confluence" },
		{ "tempest", "Tempest" },
		{ "ashen", "Ashen" },
		{ "mirror", "Mirror" },
		{ "unity", "Unity" }
	};
	const int CREST_COUNT = sizeof(CRESTS) / sizeof(CRESTS[0]);

	SDL_Rect sliderHitbox(const SDL_Rect& slider)
	{
		return { slider.x - 12, slider.y - 20, slider.w + 24, 52 };
	}
}

bool Application::hasCrest(const std::string& crestId) const
{
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].crestId == crestId && mNpcs[i].wins > 0) return true;
	return false;
}

SDL_Texture* Application::crestTexture(const std::string& crestId)
{
	std::map<std::string, SDL_Texture*>::iterator existing = mCrestTextures.find(crestId);
	if (existing != mCrestTextures.end()) return existing->second;
	std::string path = "Resources/Sprites/Crests/" + crestId + ".png";
	SDL_Texture* loaded = IMG_LoadTexture(mRenderer, path.c_str());
#if SDL_VERSION_ATLEAST(2, 0, 12)
	if (loaded != NULL) SDL_SetTextureScaleMode(loaded, SDL_ScaleModeNearest);
#endif
	mCrestTextures[crestId] = loaded;
	return loaded;
}

void Application::destroyCrestTextures()
{
	for (std::map<std::string, SDL_Texture*>::iterator item = mCrestTextures.begin();
		item != mCrestTextures.end(); ++item)
		if (item->second != NULL) SDL_DestroyTexture(item->second);
	mCrestTextures.clear();
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
	fillRect(PAUSE_PANEL, 17, 24, 38, 252);
	outlineRect(PAUSE_PANEL, 197, 151, 65, 255, 4);
	fillRect(PAUSE_STATUS_PANEL, 12, 19, 31, 245);
	outlineRect(PAUSE_STATUS_PANEL, 69, 91, 126, 255, 2);
	fillRect(PAUSE_ACTION_PANEL, 12, 19, 31, 245);
	outlineRect(PAUSE_ACTION_PANEL, 69, 91, 126, 255, 2);
	drawText("PAUSED", 72, 48, color(244, 207, 112), 30);
	drawText("PLAYER STATUS", 88, 106, color(178, 198, 226), 18);
	drawText("GOLD  " + std::to_string(std::max(0, mMoney)), 628, 104,
		color(245, 205, 88), 20, 190);

	int acquiredCrests = 0;
	for (int i = 0; i < CREST_COUNT; ++i)
		if (hasCrest(CRESTS[i].id)) ++acquiredCrests;
	drawText("CIRCUIT CRESTS  " + std::to_string(acquiredCrests) + "/" +
		std::to_string(CREST_COUNT), 88, 139, color(139, 169, 213), 15);
	for (int i = 0; i < CREST_COUNT; ++i)
	{
		int column = i % 3;
		int row = i / 3;
		SDL_Rect slot = { 82 + column * 250, 166 + row * 110, 238, 102 };
		bool acquired = hasCrest(CRESTS[i].id);
		fillRect(slot, acquired ? 30 : 20, acquired ? 39 : 27,
			acquired ? 56 : 38, 250);
		outlineRect(slot, acquired ? 163 : 65, acquired ? 129 : 74,
			acquired ? 62 : 91, 255, acquired ? 2 : 1);
		SDL_Texture* texture = crestTexture(CRESTS[i].id);
		SDL_Rect icon = { slot.x + 5, slot.y + 5, 92, 92 };
		if (texture != NULL)
		{
			SDL_SetTextureColorMod(texture, acquired ? 255 : 0,
				acquired ? 255 : 0, acquired ? 255 : 0);
			SDL_SetTextureAlphaMod(texture, acquired ? 255 : 220);
			SDL_RenderCopy(mRenderer, texture, NULL, &icon);
			SDL_SetTextureColorMod(texture, 255, 255, 255);
			SDL_SetTextureAlphaMod(texture, 255);
		}
		else
			fillRect({ icon.x + 18, icon.y + 18, 56, 56 }, 0, 0, 0, 220);
		drawText(CRESTS[i].name, slot.x + 105, slot.y + 29,
			acquired ? color(244, 207, 112) : color(134, 143, 158), 16, 124);
		drawText(acquired ? "ACQUIRED" : "NOT ACQUIRED", slot.x + 105, slot.y + 54,
			acquired ? color(102, 211, 135) : color(91, 101, 116), 11, 124);
	}

	std::vector<const MercerShard*> heldShards;
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		if (mCollectedShards.count(mMercerStock.shards[i].id) &&
			!mMercerShards.count(mMercerStock.shards[i].id))
			heldShards.push_back(&mMercerStock.shards[i]);
	drawText("SHARDS FOR MERCER  " + std::to_string(heldShards.size()),
		88, 511, color(190, 150, 225), 15);
	if (heldShards.empty())
		drawText("No shards currently held.", 88, 542, color(123, 135, 153), 14);
	else
	{
		for (size_t i = 0; i < heldShards.size(); ++i)
		{
			int column = (int)i / 10;
			int row = (int)i % 10;
			drawText("- " + heldShards[i]->name, 88 + column * 370, 541 + row * 18,
				color(218, 203, 233), 13, 350);
		}
	}
	drawText("MENU", 890, 113, color(178, 198, 226), 18);

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
	drawText("W/S/Arrows + Enter", 920, 690, color(161, 178, 202), 12);
	drawText("Esc: resume", 983, 713, color(161, 178, 202), 12);
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
