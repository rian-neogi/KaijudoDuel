#include "Application.h"

#include "AppSupport.h"
#include "Game/Card.h"
#include "SoundManager.h"

#include <algorithm>
#include <iostream>

using namespace AppSupport;

namespace
{
	const SDL_Rect REWARD_OK_BUTTON = { 520, 665, 240, 56 };
}

Application::Application(bool worldBuilder)
	: mWindow(NULL), mRenderer(NULL), mBoardTexture(NULL), mCardBackTexture(NULL), mSoundManager(NULL), mRunning(false),
	  mScreen(Screen::Overworld), mPauseMenuOpen(false), mPauseMenuSelection(0), mCurrentWorldArea(0),
	  mOpeningPortal(-1), mPortalAnimationStarted(0),
	  mWorldStartX(2), mWorldStartY(10), mPlayerX(2), mPlayerY(10), mFacingX(1), mFacingY(0),
	  mMoveUp(false), mMoveDown(false), mMoveLeft(false), mMoveRight(false), mMoveIntentX(0), mMoveIntentY(0),
	  mVisualX(2.f), mVisualY(10.f), mDialogueNpc(-1), mDialogueVisibleBytes(0),
	  mDialogueCharacterAccumulator(0), mDialogueAction(DialogueAction::None), mNoticeUntil(0),
	  mStoryStage(0), mStoryClues(0), mStoryScene(StoryScene::None), mStoryScenePage(0),
	  mWorldBuilderTab(WorldBuilderTab::Tiles), mWorldBuilderTile('.'), mWorldBuilderSelectedNpc(-1),
	  mWorldBuilderSelectedShard(-1), mWorldBuilderListScroll(0), mWorldBuilderCameraX(0),
	  mWorldBuilderCameraY(0), mWorldBuilderPainting(false),
	  mWorldBuilderDragging(false), mWorldBuilderDirty(false), mWorldBuilderNoticeError(false),
	  mWorldBuilderNoticeUntil(0),
	  mDuel(NULL), mActiveNpc(-1), mDirectDuelMode(false), mSelectedCard(-1), mActionScroll(0),
	  mOpenGraveyardPlayer(-1), mGraveyardOffset(0), mActionLogOpen(false), mActionLogScroll(0),
	  mNextAiMove(0), mDuelResult(-1), mDuelResultAt(0), mRewardCardId(-1),
	  mRewardGold(0), mPendingRewardCardId(-1), mPendingRewardGold(0), mDraggingCard(-1),
	  mDragFromZone(-1), mDragOrigin({ 0, 0, 0, 0 }), mDragMouseX(0), mDragMouseY(0),
	  mMouseX(-100), mMouseY(-100), mHoveredCard(-1), mHoverCandidateCard(-1),
	  mHoverCandidateSince(0), mOnlyActionCandidateSince(0), mOnlyActionDispatched(false),
	  mPlayerDataLoaded(false), mSettingsLoaded(false), mMusicVolume(35), mSoundVolume(100),
	  mAutoChooseOnlyAction(false), mSettingsDraggingSlider(0), mMoney(0), mActiveDeckIndex(-1),
	  mEditingDeckIndex(-1), mDeckCollectionPage(0), mDeckListScroll(0),
	  mDeckContentsScroll(0), mDeckSearchFocused(false), mDeckRenameFocused(false),
	  mDeckNoticeUntil(0), mDeckHoveredCard(-1), mShopHoveredCard(-1), mShopPage(0),
	  mShopNoticeUntil(0)
{
	std::string npcError;
	if (!loadNpcsFromLua("Lua/Npcs.lua", mNpcs, npcError))
	{
		if (!mNpcMetadataError.empty()) mNpcMetadataError += "\n";
		mNpcMetadataError += "Unable to load NPC metadata: " + npcError;
		std::cerr << "Unable to load NPC metadata: " << npcError << std::endl;
	}
	std::string stockError;
	if (!loadMercerStockFromLua("Lua/MercerStock.lua", mMercerStock, stockError))
	{
		if (!mNpcMetadataError.empty()) mNpcMetadataError += "\n";
		mNpcMetadataError += "Unable to load Mercer stock: " + stockError;
		std::cerr << "Unable to load Mercer stock: " << stockError << std::endl;
	}
	std::string worldError;
	if (!loadWorldMap("Lua/World.lua", worldError, worldBuilder))
	{
		if (!mNpcMetadataError.empty()) mNpcMetadataError += "\n";
		mNpcMetadataError += "Unable to load world map: " + worldError;
		std::cerr << "Unable to load world map: " << worldError << std::endl;
	}
}

Application::~Application()
{
	shutdown();
}

bool Application::initialize()
{
	if (!mNpcMetadataError.empty()) return false;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
		return false;
	}
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
	{
		std::cerr << "SDL_image PNG support failed: " << IMG_GetError() << std::endl;
		return false;
	}
	if (TTF_Init() != 0)
	{
		std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
		return false;
	}
	mSoundManager = new SoundManager();
	SoundMngr = mSoundManager;

	mWindow = SDL_CreateWindow(
		"Kaijudo Duel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (mWindow == NULL)
	{
		std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
		return false;
	}

	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (mRenderer == NULL)
		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_SOFTWARE);
	if (mRenderer == NULL)
	{
		std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
		return false;
	}
	SDL_RenderSetLogicalSize(mRenderer, LOGICAL_WIDTH, LOGICAL_HEIGHT);
	SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);

	mBoardTexture = IMG_LoadTexture(mRenderer, "Resources/Textures/duel-board.png");
	if (mBoardTexture == NULL)
		std::cerr << "Board texture unavailable; using fallback colors: " << IMG_GetError() << std::endl;
	mCardBackTexture = IMG_LoadTexture(mRenderer, "Resources/cardback.png");
	if (mCardBackTexture == NULL)
		std::cerr << "Card-back texture unavailable; using fallback colors: " << IMG_GetError() << std::endl;

	loadSettings();
	mRunning = true;
	return true;
}

void Application::shutdown()
{
	stopDuel();
	destroyCardTextures();
	for (std::map<int, TTF_Font*>::iterator item = mFonts.begin(); item != mFonts.end(); ++item)
		TTF_CloseFont(item->second);
	mFonts.clear();
	if (mBoardTexture != NULL) SDL_DestroyTexture(mBoardTexture);
	if (mCardBackTexture != NULL) SDL_DestroyTexture(mCardBackTexture);
	if (SoundMngr == mSoundManager) SoundMngr = NULL;
	delete mSoundManager;
	mSoundManager = NULL;
	if (mRenderer != NULL) SDL_DestroyRenderer(mRenderer);
	if (mWindow != NULL) SDL_DestroyWindow(mWindow);
	mBoardTexture = NULL;
	mCardBackTexture = NULL;
	mRenderer = NULL;
	mWindow = NULL;
	if (TTF_WasInit()) TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

int Application::run(bool smokeTest, const std::string& directPlayerDeck,
	const std::string& directAiDeck, bool worldBuilder)
{
	if (!initialize())
		return 1;
	if (worldBuilder)
	{
		mScreen = Screen::WorldBuilder;
		SDL_SetWindowTitle(mWindow, "Kaijudo Duel - World Builder");
	}
	mDirectDuelMode = !directPlayerDeck.empty() || !directAiDeck.empty();
	if (mDirectDuelMode)
	{
		if (directPlayerDeck.empty() || directAiDeck.empty() ||
			!startDuelWithDecks(directPlayerDeck, directAiDeck, -1))
		{
			std::cerr << "Unable to start direct duel. Check both deck paths and deck contents."
				<< std::endl;
			return 3;
		}
	}
	else if (smokeTest)
		return runSmokeTests();

	Uint32 previous = SDL_GetTicks();
	while (mRunning)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
			handleEvent(event);

		Uint32 now = SDL_GetTicks();
		Uint32 delta = std::min<Uint32>(now - previous, 100);
		previous = now;
		update(delta);
		render();
	}
	return 0;
}

void Application::handleEvent(const SDL_Event& event)
{
	if (event.type == SDL_QUIT)
	{
		mRunning = false;
		return;
	}
	if (mRewardCardId >= 0 && handleRewardPopupEvent(event)) return;
	if (mScreen == Screen::Overworld) handleOverworldEvent(event);
	else if (mScreen == Screen::Duel) handleDuelEvent(event);
	else if (mScreen == Screen::DeckBuilder) handleDeckBuilderEvent(event);
	else if (mScreen == Screen::Shop) handleShopEvent(event);
	else if (mScreen == Screen::WorldBuilder) handleWorldBuilderEvent(event);
	else handleSettingsEvent(event);
}

void Application::update(Uint32 deltaTime)
{
	if (mSoundManager != NULL)
	{
		const std::string& mapId = currentMapId();
		bool inEmberglen = mapId == "emberglen" || mapId == "mercers_house" ||
			mapId == "rook_mira_home";
		bool townScreen = mScreen == Screen::Overworld || mScreen == Screen::DeckBuilder ||
			mScreen == Screen::Settings || mScreen == Screen::Shop;
		if (inEmberglen && townScreen) mSoundManager->playEmberglenTheme();
		else mSoundManager->stopMusic();
	}
	if (mRewardCardId >= 0) return;
	if (mScreen == Screen::Overworld) updateOverworld(deltaTime);
	else if (mScreen == Screen::Duel) updateDuel(deltaTime);
}

void Application::render()
{
	setColor(14, 18, 28);
	SDL_RenderClear(mRenderer);
	if (mScreen == Screen::Overworld) renderOverworld();
	else if (mScreen == Screen::Duel) renderDuel();
	else if (mScreen == Screen::DeckBuilder) renderDeckBuilder();
	else if (mScreen == Screen::Shop) renderShop();
	else if (mScreen == Screen::WorldBuilder) renderWorldBuilder();
	else renderSettings();
	if (mRewardCardId >= 0) renderRewardPopup();
	SDL_RenderPresent(mRenderer);
}

bool Application::handleRewardPopupEvent(const SDL_Event& event)
{
	if (mRewardCardId < 0) return false;
	bool dismiss = false;
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
		dismiss = event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE;
	else if (event.type == SDL_MOUSEMOTION)
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
	else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		int x, y;
		logicalMouse(event.button.x, event.button.y, x, y);
		dismiss = contains(REWARD_OK_BUTTON, x, y);
	}
	if (dismiss)
	{
		mRewardCardId = -1;
		mRewardGold = 0;
	}
	return true;
}

void Application::renderRewardPopup()
{
	if (mRewardCardId < 0 || mRewardCardId >= (int)gCardDatabase.size()) return;
	const CardData& card = gCardDatabase[mRewardCardId];
	const SDL_Rect panel = { 330, 55, 620, 690 };
	const SDL_Rect cardRect = { 500, 175, 280, 390 };
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 3, 6, 12, 220);
	fillRect(panel, 18, 27, 43, 252);
	outlineRect(panel, 218, 171, 74, 255, 4);
	drawText("CARD REWARD", 487, 82, color(247, 211, 113), 32);
	drawText("You won a new card!", 487, 128, color(218, 228, 242), 18);

	SDL_Texture* texture = cardTextureById(mRewardCardId);
	if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &cardRect);
	else
	{
		fillRect(cardRect, 220, 207, 176);
		drawText(card.Name, cardRect.x + 16, cardRect.y + 145,
			color(39, 30, 23), 18, cardRect.w - 32);
	}
	SDL_Color civ = civilizationColor(card.Civilization);
	outlineRect(cardRect, civ.r, civ.g, civ.b, 255, 4);
	drawText("+1 " + card.Name, 390, 584, color(241, 235, 216), 20, 500);
	drawText("Added to your collection" +
		(mRewardGold > 0 ? "  •  +" + std::to_string(mRewardGold) + " gold" : ""),
		390, 620, color(164, 190, 220), 15, 500);

	bool hovered = contains(REWARD_OK_BUTTON, mMouseX, mMouseY);
	fillRect(REWARD_OK_BUTTON, hovered ? 66 : 42, hovered ? 103 : 70,
		hovered ? 148 : 108, 255);
	outlineRect(REWARD_OK_BUTTON, hovered ? 158 : 112, hovered ? 204 : 157,
		hovered ? 248 : 214, 255, 3);
	drawText("OK", REWARD_OK_BUTTON.x + 101, REWARD_OK_BUTTON.y + 15,
		color(244, 247, 251), 21);
}


SDL_Color Application::civilizationColor(int civilization) const
{
	switch (civilization)
	{
		case CIV_LIGHT: return color(212, 173, 63);
		case CIV_NATURE: return color(49, 137, 66);
		case CIV_WATER: return color(42, 116, 177);
		case CIV_FIRE: return color(190, 63, 42);
		case CIV_DARKNESS: return color(101, 53, 125);
		default: return color(105, 105, 105);
	}
}

void Application::setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_SetRenderDrawColor(mRenderer, r, g, b, a);
}

void Application::fillRect(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	setColor(r, g, b, a);
	SDL_RenderFillRect(mRenderer, &rect);
}

void Application::outlineRect(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int thickness)
{
	setColor(r, g, b, a);
	for (int i = 0; i < thickness; ++i)
	{
		SDL_Rect line = { rect.x + i, rect.y + i, rect.w - i * 2, rect.h - i * 2 };
		SDL_RenderDrawRect(mRenderer, &line);
	}
}

TTF_Font* Application::font(int size)
{
	std::map<int, TTF_Font*>::iterator existing = mFonts.find(size);
	if (existing != mFonts.end()) return existing->second;
	TTF_Font* loaded = TTF_OpenFont("Resources/OxygenMono.ttf", size);
	if (loaded == NULL) std::cerr << "Unable to load font: " << TTF_GetError() << std::endl;
	mFonts[size] = loaded;
	return loaded;
}

void Application::drawText(const std::string& text, int x, int y, SDL_Color textColor, int size, int maxWidth)
{
	if (text.empty()) return;
	TTF_Font* selectedFont = font(size);
	if (selectedFont == NULL) return;
	SDL_Surface* surface = maxWidth > 0
		? TTF_RenderUTF8_Blended_Wrapped(selectedFont, text.c_str(), textColor, maxWidth)
		: TTF_RenderUTF8_Blended(selectedFont, text.c_str(), textColor);
	if (surface == NULL) return;
	SDL_Texture* texture = SDL_CreateTextureFromSurface(mRenderer, surface);
	SDL_Rect destination = { x, y, surface->w, surface->h };
	SDL_FreeSurface(surface);
	if (texture != NULL)
	{
		SDL_RenderCopy(mRenderer, texture, NULL, &destination);
		SDL_DestroyTexture(texture);
	}
}

void Application::logicalMouse(int windowX, int windowY, int& logicalX, int& logicalY) const
{
	// SDL_RenderSetLogicalSize installs an event filter that has already
	// converted mouse and touch coordinates to the logical render size.
	logicalX = windowX;
	logicalY = windowY;
}

bool Application::contains(const SDL_Rect& rect, int x, int y) const
{
	return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}
