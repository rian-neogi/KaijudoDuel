#include "Application.h"

#include "AppSupport.h"
#include "Game/Card.h"
#include "Game/CardData.h"

#include <algorithm>

using namespace AppSupport;

namespace
{
	const SDL_Rect SHOP_BACK_BUTTON = { 24, 24, 140, 46 };
	const SDL_Rect SHOP_PANEL = { 32, 92, 1216, 660 };

	const char* SHOP_CARD_NAMES[] = {
		"Aqua Hulcus",
		"Aqua Surfer",
		"Bronze-Arm Tribe",
		"Natural Snare",
		"Ghost Touch",
		"Terror Pit",
		"Mini Titan Gett",
		"Burst Shot",
		"Senatine Jade Tree",
		"Mighty Shouter"
	};
}

void Application::enterShop()
{
	ensurePlayerDataLoaded();
	mDialogueNpc = -1;
	mPauseMenuOpen = false;
	mScreen = Screen::Shop;
	mShopHoveredCard = -1;
	mShopCardHitboxes.clear();
}

void Application::leaveShop()
{
	mShopHoveredCard = -1;
	mShopCardHitboxes.clear();
	mScreen = Screen::Overworld;
}

std::vector<int> Application::shopInventory() const
{
	std::vector<int> cards;
	for (size_t i = 0; i < sizeof(SHOP_CARD_NAMES) / sizeof(SHOP_CARD_NAMES[0]); ++i)
	{
		int cardId = getCardIdFromName(SHOP_CARD_NAMES[i]);
		if (cardId >= 0) cards.push_back(cardId);
	}
	return cards;
}

int Application::shopPrice(int cardId) const
{
	if (cardId < 0 || cardId >= (int)gCardDatabase.size()) return 0;
	return 60 + std::max(1, gCardDatabase[cardId].ManaCost) * 20;
}

void Application::handleShopEvent(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
		return;
	}
	if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym == SDLK_ESCAPE)
	{
		leaveShop();
		return;
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;

	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(SHOP_BACK_BUTTON, x, y))
	{
		leaveShop();
		return;
	}

	for (size_t i = 0; i < mShopCardHitboxes.size(); ++i)
	{
		if (!contains(mShopCardHitboxes[i].rect, x, y)) continue;
		int cardId = mShopCardHitboxes[i].cardId;
		if (cardId < 0 || cardId >= (int)mCollectionCounts.size()) return;
		int price = shopPrice(cardId);
		if (mCollectionCounts[cardId] >= 4)
			mShopNotice = "You already own four copies of " + gCardDatabase[cardId].Name + ".";
		else if (mMoney < price)
			mShopNotice = "You need " + std::to_string(price - mMoney) + " more gold.";
		else
		{
			mMoney -= price;
			++mCollectionCounts[cardId];
			savePlayerProgress();
			mShopNotice = "Purchased " + gCardDatabase[cardId].Name + ".";
		}
		mShopNoticeUntil = SDL_GetTicks() + 3500;
		return;
	}
}

void Application::renderShop()
{
	ensurePlayerDataLoaded();
	mShopHoveredCard = -1;
	for (std::vector<DeckCardHitbox>::reverse_iterator hitbox = mShopCardHitboxes.rbegin();
		hitbox != mShopCardHitboxes.rend(); ++hitbox)
	{
		if (contains(hitbox->rect, mMouseX, mMouseY))
		{
			mShopHoveredCard = hitbox->cardId;
			break;
		}
	}
	mShopCardHitboxes.clear();

	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 12, 18, 29);
	fillRect(SHOP_PANEL, 22, 30, 46, 250);
	outlineRect(SHOP_PANEL, 194, 149, 61, 255, 3);
	fillRect(SHOP_BACK_BUTTON, 34, 50, 75, 250);
	outlineRect(SHOP_BACK_BUTTON, 112, 149, 205, 255, 2);
	drawText("Back", 70, 37, color(238, 241, 247), 18);
	drawText("MERCER'S CARD SHOP", 205, 30, color(244, 207, 112), 31);
	drawText("Gold: " + std::to_string(mMoney), 1035, 37, color(245, 205, 88), 22);
	drawText("Click a card to buy one copy. You may own up to four copies.",
		205, 68, color(172, 190, 216), 15);

	std::vector<int> cards = shopInventory();
	for (size_t i = 0; i < cards.size(); ++i)
	{
		int column = (int)i % 5;
		int row = (int)i / 5;
		SDL_Rect cardRect = { 76 + column * 238, 132 + row * 292, 150, 208 };
		int cardId = cards[i];
		SDL_Texture* texture = cardTextureById(cardId);
		if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &cardRect);
		else
		{
			fillRect(cardRect, 220, 207, 176);
			drawText(gCardDatabase[cardId].Name, cardRect.x + 8, cardRect.y + 70,
				color(39, 30, 23), 14, cardRect.w - 16);
		}
		SDL_Color civ = civilizationColor(gCardDatabase[cardId].Civilization);
		outlineRect(cardRect, civ.r, civ.g, civ.b, 255, 3);
		mShopCardHitboxes.push_back({ cardRect, cardId });

		int owned = cardId < (int)mCollectionCounts.size() ? mCollectionCounts[cardId] : 0;
		SDL_Rect pricePanel = { cardRect.x, cardRect.y + cardRect.h + 8, cardRect.w, 48 };
		bool soldOut = owned >= 4;
		fillRect(pricePanel, soldOut ? 47 : 31, soldOut ? 48 : 42, soldOut ? 54 : 62, 248);
		outlineRect(pricePanel, soldOut ? 103 : 176, soldOut ? 105 : 137, soldOut ? 112 : 55, 255, 2);
		drawText(soldOut ? "OWNED 4/4" : std::to_string(shopPrice(cardId)) + " GOLD",
			pricePanel.x + 9, pricePanel.y + 6, soldOut ? color(168, 172, 182) : color(247, 210, 102), 14);
		if (!soldOut)
			drawText("Owned " + std::to_string(owned) + "/4", pricePanel.x + 9, pricePanel.y + 27,
				color(174, 192, 216), 12);
	}

	if (!mShopNotice.empty() && SDL_GetTicks() < mShopNoticeUntil)
	{
		fillRect({ 350, 706, 580, 38 }, 19, 37, 49, 248);
		drawText(mShopNotice, 367, 716, color(118, 228, 151), 14, 548);
	}
	renderShopHoverPreview();
}

void Application::renderShopHoverPreview()
{
	if (mShopHoveredCard < 0 || mShopHoveredCard >= (int)gCardDatabase.size()) return;
	const CardData& card = gCardDatabase[mShopHoveredCard];
	const int width = 300;
	const int height = 420;
	int x = mMouseX >= LOGICAL_WIDTH / 2 ? 330 : 650;
	int y = 180;
	fillRect({ x + 10, y + 12, width, height }, 2, 5, 9, 175);
	SDL_Rect preview = { x, y, width, height };
	SDL_Texture* texture = cardTextureById(mShopHoveredCard);
	if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &preview);
	else
	{
		fillRect(preview, 220, 207, 176);
		drawText(card.Name, preview.x + 18, preview.y + 150, color(39, 30, 23), 22, preview.w - 36);
	}
	SDL_Color civ = civilizationColor(card.Civilization);
	outlineRect(preview, civ.r, civ.g, civ.b, 255, 5);
}
