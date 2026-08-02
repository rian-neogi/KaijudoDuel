#include "Application.h"

#include "AppSupport.h"
#include "Game/Card.h"

#include <algorithm>
#include <iostream>

using namespace AppSupport;

Application::Application()
	: mWindow(NULL), mRenderer(NULL), mBoardTexture(NULL), mCardBackTexture(NULL), mRunning(false),
	  mScreen(Screen::Overworld), mPlayerX(2), mPlayerY(10), mFacingX(1), mFacingY(0),
	  mVisualX(2.f), mVisualY(10.f), mDialogueNpc(-1), mNoticeUntil(0),
	  mDuel(NULL), mActiveNpc(-1), mSelectedCard(-1), mActionScroll(0),
	  mOpenGraveyardPlayer(-1), mGraveyardOffset(0),
	  mNextAiMove(0), mDuelResult(-1), mDuelResultAt(0), mDraggingCard(-1),
	  mDragFromZone(-1), mDragOrigin({ 0, 0, 0, 0 }), mDragMouseX(0), mDragMouseY(0),
	  mMouseX(-100), mMouseY(-100), mHoveredCard(-1), mHoverCandidateCard(-1),
	  mHoverCandidateSince(0)
{
	mMap.push_back("####################");
	mMap.push_back("#......~~~.........#");
	mMap.push_back("#..HH..~~~..TTTT...#");
	mMap.push_back("#..HH.......T..T...#");
	mMap.push_back("#......====........#");
	mMap.push_back("#..####=..=..~~~...#");
	mMap.push_back("#......=..=..~~~...#");
	mMap.push_back("#......====........#");
	mMap.push_back("#...TT......HH.....#");
	mMap.push_back("#...TT......HH.....#");
	mMap.push_back("#..................#");
	mMap.push_back("####################");

	mNpcs.push_back({ 10, 7, "Mira", "Decks/Zagaan.txt", "Darkness answers my call. Ready to duel?", false });
	mNpcs.push_back({ 16, 4, "Marin", "Decks/AquaSniper.txt", "Let us see whether you can read the currents.", false });
	mNpcs.push_back({ 7, 4, "Rook", "Decks/RoaringGreathorn.txt", "Strength grows one turn at a time.", false });
}

Application::~Application()
{
	shutdown();
}

bool Application::initialize()
{
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

int Application::run(bool smokeTest)
{
	if (!initialize())
		return 1;
	if (smokeTest)
		startDuel(0);

	Uint32 previous = SDL_GetTicks();
	int smokeFrames = 0;
	int smokeNpc = 0;
	int smokeBlackFeather = -1;
	int smokeBlackFeatherSacrifice = -1;
	bool blackFeatherSmokeStarted = false;
	bool blackFeatherWasSelectable = false;
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
		if (smokeTest)
		{
			if (smokeNpc == 0 && smokeFrames == 5 && !exerciseHoverTimingSmoke())
			{
				std::cerr << "Hover timing smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 10 && !exerciseBinaryChoiceSmoke())
			{
				std::cerr << "Binary choice menu smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames >= 100 && smokeFrames < 180 && mDraggingCard < 0)
			{
				for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
				{
					std::lock_guard<std::mutex> lock(gMutex);
					if (item->hoverAnchor && mDuel != NULL && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size() &&
						mDuel->mCardList[item->cardId]->mOwner == 0 && mDuel->mCardList[item->cardId]->mZone == ZONE_HAND)
					{
						mMouseX = item->rect.x + item->rect.w / 2;
						mMouseY = item->rect.y + item->rect.h / 2;
						break;
					}
				}
			}
			if (smokeNpc == 0 && smokeFrames == 20 && !exerciseEvolutionSmoke())
			{
				std::cerr << "Evolution smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 60 && mDuel != NULL)
			{
				int selectionCard = -1;
				{
					std::lock_guard<std::mutex> lock(gMutex);
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
					{
						if (moves[i].getType() == "choiceselect" && messageInt(moves[i], "selection") >= 0)
						{
							selectionCard = messageInt(moves[i], "selection");
							break;
						}
					}
				}
				CardHitbox handCard = { { 0, 0, 0, 0 }, -1, true, true, true };
				{
					std::lock_guard<std::mutex> lock(gMutex);
					for (std::vector<CardHitbox>::reverse_iterator item = mCardHitboxes.rbegin(); item != mCardHitboxes.rend(); ++item)
					{
						if (item->hoverAnchor && item->cardId >= 0 && item->cardId < (int)mDuel->mCardList.size() &&
							mDuel->mCardList[item->cardId]->mOwner == 0 && mDuel->mCardList[item->cardId]->mZone == ZONE_HAND)
						{
							handCard = *item;
							break;
						}
					}
				}
				if (selectionCard >= 0 && handCard.cardId >= 0)
				{
					beginDrag(handCard.cardId, handCard.rect, handCard.rect.x, handCard.rect.y);
					if (mDraggingCard >= 0)
					{
						std::cerr << "Pending choice allowed an unrelated card drag." << std::endl;
						return 2;
					}
				}
				SDL_Rect selectionButton = { 0, 0, 0, 0 };
				for (size_t i = 0; i < mActionButtons.size(); ++i)
				{
					if (mActionButtons[i].message.getType() == "choiceselect" &&
						messageInt(mActionButtons[i].message, "selection") == selectionCard)
					{
						selectionButton = mActionButtons[i].rect;
						break;
					}
				}
				if (selectionCard < 0 || selectionButton.w == 0)
				{
					std::cerr << "Clickable action smoke test could not find its button." << std::endl;
					return 2;
				}
				SDL_Event click = {};
				click.type = SDL_MOUSEBUTTONDOWN;
				click.button.button = SDL_BUTTON_LEFT;
				// SDL_RenderSetLogicalSize filters real mouse events into logical
				// coordinates before they reach the application.
				click.button.x = selectionButton.x + selectionButton.w / 2;
				click.button.y = selectionButton.y + selectionButton.h / 2;
				handleDuelEvent(click);
				{
					std::lock_guard<std::mutex> lock(gMutex);
					if (mDuel->mIsChoiceActive)
					{
						std::cerr << "Clickable action smoke test did not dispatch its click." << std::endl;
						return 2;
					}
				}
			}
			if (smokeNpc == 0 && smokeFrames >= 65 && smokeFrames < 75 && !blackFeatherSmokeStarted)
			{
				blackFeatherSmokeStarted = beginBlackFeatherAiSmoke(
					smokeBlackFeather, smokeBlackFeatherSacrifice);
				if (smokeFrames == 74 && !blackFeatherSmokeStarted && mDuel != NULL)
				{
					std::lock_guard<std::mutex> lock(gMutex);
					std::cerr << "Black Feather setup state: choice=" << mDuel->mIsChoiceActive
						<< " pointer=" << (mDuel->mChoice != NULL)
						<< " suspended=" << mDuel->mLuaCallbackSuspended
						<< " queued=" << mDuel->mMsgMngr.hasMoreMessages() << std::endl;
				}
			}
			if (smokeNpc == 0 && blackFeatherSmokeStarted && !blackFeatherWasSelectable && mDuel != NULL)
			{
				std::lock_guard<std::mutex> lock(gMutex);
				if (mDuel->mIsChoiceActive && mDuel->mChoicePlayer == 1)
				{
					blackFeatherWasSelectable = mDuel->choiceCanBeSelected(smokeBlackFeather) == 1;
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
						if (moves[i].getType() == "choiceselect" && messageInt(moves[i], "selection") < 0)
							blackFeatherWasSelectable = false;
					mNextAiMove = 0;
				}
			}
			if (smokeFrames == 75 && mDuel != NULL)
			{
				Message endTurn;
				bool foundEndTurn = false;
				{
					std::lock_guard<std::mutex> lock(gMutex);
					std::vector<Message> moves = mDuel->getPossibleMoves();
					for (size_t i = 0; i < moves.size(); ++i)
					{
						if (moves[i].getType() == "endturn")
						{
							endTurn = moves[i];
							foundEndTurn = true;
							break;
						}
					}
				}
				if (foundEndTurn) playAction(endTurn);
			}
			if (smokeNpc == 0 && smokeFrames == 90 &&
				(!blackFeatherSmokeStarted || !blackFeatherWasSelectable ||
				 !verifyBlackFeatherAiSmoke(smokeBlackFeather, smokeBlackFeatherSacrifice)))
			{
				std::cerr << "Black Feather AI sacrifice smoke test failed." << std::endl;
				return 2;
			}
			if (smokeNpc == 0 && smokeFrames == 95 && !exerciseGraveyardBrowserSmoke())
			{
				std::cerr << "Graveyard browser smoke test failed." << std::endl;
				return 2;
			}
			SDL_Delay(5);
			if (++smokeFrames >= 300)
			{
				smokeFrames = 0;
				if (++smokeNpc < (int)mNpcs.size()) startDuel(smokeNpc);
				else mRunning = false;
			}
		}
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
	if (mScreen == Screen::Overworld)
		handleOverworldEvent(event);
	else
		handleDuelEvent(event);
}

void Application::update(Uint32 deltaTime)
{
	if (mScreen == Screen::Overworld)
		updateOverworld(deltaTime);
	else
		updateDuel(deltaTime);
}

void Application::render()
{
	setColor(14, 18, 28);
	SDL_RenderClear(mRenderer);
	if (mScreen == Screen::Overworld)
		renderOverworld();
	else
		renderDuel();
	SDL_RenderPresent(mRenderer);
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
