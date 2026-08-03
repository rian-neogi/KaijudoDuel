#pragma once

#include "App/MercerStock.h"
#include "App/Npc.h"
#include "Game/Duel.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

class SoundManager;

class Application
{
public:
	Application(bool worldBuilder = false);
	~Application();

	int run(bool smokeTest = false, const std::string& directPlayerDeck = "",
		const std::string& directAiDeck = "", bool worldBuilder = false);

private:
	enum class Screen
	{
		Overworld,
		Duel,
		DeckBuilder,
		Settings,
		Shop,
		WorldBuilder
	};
	enum class WorldBuilderTab
	{
		Tiles,
		Npcs,
		Shards
	};
	enum class StoryScene
	{
		None,
		Intro,
		BossReveal,
		ActComplete
	};

	struct CardHitbox
	{
		SDL_Rect rect;
		int cardId;
		bool faceUp;
		bool hoverAnchor;
		bool immediateHover;
	};

	struct ActionButton
	{
		SDL_Rect rect;
		Message message;
		std::string label;
	};

	struct AnimatedCard
	{
		float x = 0.f;
		float y = 0.f;
		float width = 0.f;
		float height = 0.f;
		float angle = 0.f;
		float targetX = 0.f;
		float targetY = 0.f;
		float targetWidth = 0.f;
		float targetHeight = 0.f;
		float targetAngle = 0.f;
		bool initialized = false;
	};

	struct PlayerDeck
	{
		std::string name;
		std::string path;
		std::map<int, int> cards;
		bool managed = false;
		bool dirty = false;
	};

	struct DeckCardHitbox
	{
		SDL_Rect rect;
		int cardId;
	};
	struct WorldArea
	{
		std::string id;
		std::string name;
		bool indoor;
		std::vector<std::string> tiles;
	};
	struct WorldPortal
	{
		std::string fromMap;
		int fromX;
		int fromY;
		std::string toMap;
		int toX;
		int toY;
	};

	bool initialize();
	void shutdown();
	void handleEvent(const SDL_Event& event);
	void update(Uint32 deltaTime);
	void render();

	void handleOverworldEvent(const SDL_Event& event);
	void updateOverworld(Uint32 deltaTime);
	void renderOverworld();
	void handlePauseMenuEvent(const SDL_Event& event);
	void renderPauseMenu();
	void handleSettingsEvent(const SDL_Event& event);
	void renderSettings();
	bool exerciseMenuScreensSmoke();
	bool exerciseOverworldMovementSmoke();
	bool exerciseStorySmoke();
	void initializeStory();
	bool handleStoryEvent(const SDL_Event& event);
	void renderStoryScene();
	void renderStoryTracker();
	void discoverStoryClue(int npcIndex);
	void updateStoryProgress();
	bool npcVisible(int npcIndex) const;
	bool npcHasStoryMarker(int npcIndex) const;
	std::string storyDialogueForNpc(int npcIndex) const;
	std::string storyObjective() const;
	float overworldCameraX() const;
	float overworldCameraY() const;
	void tryMove(int dx, int dy);
	void collectShardAt(int x, int y);
	void interact();
	bool isWalkable(int x, int y) const;
	int npcAt(int x, int y, int ignoredNpc = -1) const;
	std::vector<std::string>& currentMap();
	const std::vector<std::string>& currentMap() const;
	const std::string& currentMapId() const;
	int worldAreaIndex(const std::string& id) const;
	bool beginPortalAt(int x, int y);
	bool activatePortalAt(int x, int y);
	bool isPortalAt(const std::string& mapId, int x, int y) const;
	bool loadWorldMap(const std::string& path, std::string& error, bool allowMissingPositions = false);
	void handleWorldBuilderEvent(const SDL_Event& event);
	void renderWorldBuilder();
	void paintWorldBuilderTile(int x, int y);
	void placeWorldBuilderSelection(int x, int y);
	bool worldBuilderCanPlace(int x, int y, int ignoredNpc, int ignoredShard) const;
	bool saveWorldBuilder(std::string& error);
	void showWorldBuilderNotice(const std::string& notice, bool error = false);

	void startDuel(int npcIndex, bool ignoreProgressLimit = false);
	bool startDuelWithDecks(const std::string& playerDeck, const std::string& aiDeck,
		int npcIndex);
	void stopDuel();
	void handleDuelEvent(const SDL_Event& event);
	bool handleGraveyardEvent(const SDL_Event& event);
	void updateDuel(Uint32 deltaTime);
	void renderDuel();
	void renderGraveyardPile(int player);
	void renderGraveyardOverlay();
	SDL_Rect graveyardPileRect(int player) const;
	void renderDeckPile(int player);
	SDL_Rect deckPileRect(int player) const;
	bool handleActionLogEvent(const SDL_Event& event);
	void renderActionLogOverlay();
	void renderAttackIndicator();
	std::string actionLogLabel(const Message& message, int player) const;
	bool exerciseEvolutionSmoke();
	bool exerciseHeuristicAttackSafetySmoke();
	bool exerciseHeuristicBlockChoiceSmoke();
	bool exerciseHeuristicManaConservationSmoke();
	bool exerciseBinaryChoiceSmoke();
	bool exerciseActionLabelSmoke();
	bool beginMandatorySacrificeAiSmoke(const std::string& cardName, int& summonedCard, int& sacrifice);
	bool verifyMandatorySacrificeAiSmoke(const std::string& cardName, int summonedCard, int sacrifice);
	bool exerciseGraveyardBrowserSmoke();
	void playAction(const Message& message);
	void playCard(const Message& message);
	void beginDrag(int cardId, const SDL_Rect& origin, int mouseX, int mouseY);
	void finishDrag(int mouseX, int mouseY);
	void cancelDrag();
	bool findDragAction(const std::string& type, int cardId, int targetId, Message& result);
	bool findClickAction(int cardId, Message& result);
	void renderDragOverlay();
	void renderHoverPreview();
	void updateHoverState(int candidateCard, bool immediate, Uint32 now);
	int duelHoverCandidateAt(int x, int y, bool& immediate) const;
	bool exerciseHoverTimingSmoke();
	std::vector<Message> visibleActions();
	bool messageReferencesCard(const Message& message, int cardId) const;
	std::string actionLabel(const Message& message) const;

	void setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
	void fillRect(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
	void outlineRect(const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, int thickness = 1);
	void drawText(const std::string& text, int x, int y, SDL_Color color, int size, int maxWidth = 0);
	TTF_Font* font(int size);
	void updateCardAnimations(Uint32 deltaTime);
	SDL_Texture* cardTexture(Card* card);
	SDL_Texture* cardTextureById(int cardId);
	void destroyCardTextures();
	SDL_Rect cardBounds(const AnimatedCard& animation) const;
	void drawCard(Card* card, const SDL_Rect& rect, bool faceUp, bool selected, bool clickable, float angle = 0.f);
	void drawCardBack(const SDL_Rect& rect);
	void drawZone(const std::vector<Card*>& cards, int x, int y, int width, int cardWidth, int cardHeight, bool faceUp, bool clickable);
	void drawHand(const std::vector<Card*>& cards, bool opponent);
	void drawCharacter(float gridX, float gridY, CharacterAppearance appearance,
		bool completed, bool walking = false);
	SDL_Color civilizationColor(int civilization) const;
	void logicalMouse(int windowX, int windowY, int& logicalX, int& logicalY) const;
	bool contains(const SDL_Rect& rect, int x, int y) const;

	void ensurePlayerDataLoaded();
	void loadSettings();
	void handleDeckBuilderEvent(const SDL_Event& event);
	void renderDeckBuilder();
	void renderDeckBuilderHoverPreview();
	void enterDeckBuilder();
	void leaveDeckBuilder();
	void createDeck();
	bool saveDeck(int deckIndex);
	void setActiveDeck(int deckIndex);
	int deckCardCount(const PlayerDeck& deck) const;
	std::vector<int> filteredCollection() const;
	std::string availableDeckPath(const std::string& name, const std::string& currentPath) const;
	void showDeckNotice(const std::string& notice);
	void savePlayerProgress();
	void saveSettings();
	void awardNpcVictory(int npcIndex);

	void enterShop();
	void leaveShop();
	void handleShopEvent(const SDL_Event& event);
	void renderShop();
	void renderShopHoverPreview();
	std::vector<int> shopInventory() const;
	int shopPrice(int cardId) const;

	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	SDL_Texture* mBoardTexture;
	SDL_Texture* mCardBackTexture;
	SoundManager* mSoundManager;
	std::map<int, SDL_Texture*> mCardTextures;
	std::map<int, TTF_Font*> mFonts;
	bool mRunning;
	Screen mScreen;
	bool mPauseMenuOpen;
	int mPauseMenuSelection;

	std::vector<WorldArea> mWorldAreas;
	std::vector<WorldPortal> mWorldPortals;
	int mCurrentWorldArea;
	int mOpeningPortal;
	Uint32 mPortalAnimationStarted;
	std::string mWorldStartMap;
	int mWorldStartX;
	int mWorldStartY;
	std::vector<Npc> mNpcs;
	MercerStockData mMercerStock;
	std::string mNpcMetadataError;
	int mPlayerX;
	int mPlayerY;
	int mFacingX;
	int mFacingY;
	bool mMoveUp;
	bool mMoveDown;
	bool mMoveLeft;
	bool mMoveRight;
	int mMoveIntentX;
	int mMoveIntentY;
	float mVisualX;
	float mVisualY;
	int mDialogueNpc;
	std::string mNotice;
	Uint32 mNoticeUntil;
	int mStoryStage;
	int mStoryClues;
	StoryScene mStoryScene;
	int mStoryScenePage;
	WorldBuilderTab mWorldBuilderTab;
	char mWorldBuilderTile;
	int mWorldBuilderSelectedNpc;
	int mWorldBuilderSelectedShard;
	int mWorldBuilderListScroll;
	int mWorldBuilderCameraX;
	int mWorldBuilderCameraY;
	bool mWorldBuilderPainting;
	bool mWorldBuilderDragging;
	bool mWorldBuilderDirty;
	bool mWorldBuilderNoticeError;
	std::string mWorldBuilderNotice;
	Uint32 mWorldBuilderNoticeUntil;

	Duel* mDuel;
	std::thread mDuelThread;
	int mActiveNpc;
	bool mDirectDuelMode;
	int mSelectedCard;
	int mActionScroll;
	int mOpenGraveyardPlayer;
	int mGraveyardOffset;
	bool mActionLogOpen;
	int mActionLogScroll;
	Uint32 mNextAiMove;
	int mDuelResult;
	Uint32 mDuelResultAt;
	std::vector<CardHitbox> mCardHitboxes;
	std::vector<ActionButton> mActionButtons;
	std::map<int, AnimatedCard> mCardAnimations;
	int mDraggingCard;
	int mDragFromZone;
	SDL_Rect mDragOrigin;
	int mDragMouseX;
	int mDragMouseY;
	int mMouseX;
	int mMouseY;
	int mHoveredCard;
	int mHoverCandidateCard;
	Uint32 mHoverCandidateSince;
	std::string mOnlyActionCandidate;
	Uint32 mOnlyActionCandidateSince;
	bool mOnlyActionDispatched;

	bool mPlayerDataLoaded;
	bool mSettingsLoaded;
	int mMusicVolume;
	int mSoundVolume;
	bool mAutoChooseOnlyAction;
	int mSettingsDraggingSlider;
	int mMoney;
	std::set<std::string> mCollectedShards;
	std::set<std::string> mMercerShards;
	std::vector<int> mCollectionCounts;
	std::vector<PlayerDeck> mPlayerDecks;
	std::string mActiveDeckPath;
	int mActiveDeckIndex;
	int mEditingDeckIndex;
	int mDeckCollectionPage;
	int mDeckListScroll;
	int mDeckContentsScroll;
	std::string mDeckSearch;
	std::string mDeckNameInput;
	bool mDeckSearchFocused;
	bool mDeckRenameFocused;
	std::string mDeckNotice;
	Uint32 mDeckNoticeUntil;
	std::vector<DeckCardHitbox> mDeckCardHitboxes;
	int mDeckHoveredCard;
	std::vector<DeckCardHitbox> mShopCardHitboxes;
	int mShopHoveredCard;
	int mShopPage;
	std::string mShopNotice;
	Uint32 mShopNoticeUntil;
};
