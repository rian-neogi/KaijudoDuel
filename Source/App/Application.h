#pragma once

#include "Game/Duel.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

class Application
{
public:
	Application();
	~Application();

	int run(bool smokeTest = false);

private:
	enum class Screen
	{
		Overworld,
		Duel
	};

	struct Npc
	{
		int x;
		int y;
		std::string name;
		std::string deck;
		std::string challenge;
		bool defeated;
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

	bool initialize();
	void shutdown();
	void handleEvent(const SDL_Event& event);
	void update(Uint32 deltaTime);
	void render();

	void handleOverworldEvent(const SDL_Event& event);
	void updateOverworld(Uint32 deltaTime);
	void renderOverworld();
	void tryMove(int dx, int dy);
	void interact();
	bool isWalkable(int x, int y) const;
	int npcAt(int x, int y) const;

	void startDuel(int npcIndex);
	void stopDuel();
	void handleDuelEvent(const SDL_Event& event);
	bool handleGraveyardEvent(const SDL_Event& event);
	void updateDuel(Uint32 deltaTime);
	void renderDuel();
	void renderGraveyardPile(int player);
	void renderGraveyardOverlay();
	SDL_Rect graveyardPileRect(int player) const;
	bool exerciseEvolutionSmoke();
	bool exerciseBinaryChoiceSmoke();
	bool beginBlackFeatherAiSmoke(int& blackFeather, int& sacrifice);
	bool verifyBlackFeatherAiSmoke(int blackFeather, int sacrifice);
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
	void destroyCardTextures();
	SDL_Rect cardBounds(const AnimatedCard& animation) const;
	void drawCard(Card* card, const SDL_Rect& rect, bool faceUp, bool selected, bool clickable, float angle = 0.f);
	void drawCardBack(const SDL_Rect& rect);
	void drawZone(const std::vector<Card*>& cards, int x, int y, int width, int cardWidth, int cardHeight, bool faceUp, bool clickable);
	void drawHand(const std::vector<Card*>& cards, bool opponent);
	void drawCharacter(int gridX, int gridY, bool rival, bool defeated);
	SDL_Color civilizationColor(int civilization) const;
	void logicalMouse(int windowX, int windowY, int& logicalX, int& logicalY) const;
	bool contains(const SDL_Rect& rect, int x, int y) const;

	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	SDL_Texture* mBoardTexture;
	SDL_Texture* mCardBackTexture;
	std::map<int, SDL_Texture*> mCardTextures;
	std::map<int, TTF_Font*> mFonts;
	bool mRunning;
	Screen mScreen;

	std::vector<std::string> mMap;
	std::vector<Npc> mNpcs;
	int mPlayerX;
	int mPlayerY;
	int mFacingX;
	int mFacingY;
	float mVisualX;
	float mVisualY;
	int mDialogueNpc;
	std::string mNotice;
	Uint32 mNoticeUntil;

	Duel* mDuel;
	std::thread mDuelThread;
	int mActiveNpc;
	int mSelectedCard;
	int mActionScroll;
	int mOpenGraveyardPlayer;
	int mGraveyardOffset;
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
};
