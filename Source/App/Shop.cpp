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
	const SDL_Rect SHOP_PREV_BUTTON = { 52, 99, 108, 34 };
	const SDL_Rect SHOP_GIVE_BUTTON = { 842, 99, 276, 34 };
	const SDL_Rect SHOP_NEXT_BUTTON = { 1128, 99, 108, 34 };
	const int SHOP_CARDS_PER_PAGE = 10;

	int shopPageCount(size_t cardCount)
	{
		return std::max(1, ((int)cardCount + SHOP_CARDS_PER_PAGE - 1) /
			SHOP_CARDS_PER_PAGE);
	}
}

void Application::enterShop()
{
	ensurePlayerDataLoaded();
	mDialogueNpc = -1;
	mPauseMenuOpen = false;
	mScreen = Screen::Shop;
	mShopHoveredCard = -1;
	mShopPage = 0;
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
	std::set<int> added;
	for (size_t i = 0; i < mMercerStock.initialStock.size(); ++i)
	{
		int cardId = getCardIdFromName(mMercerStock.initialStock[i]);
		if (cardId >= 0 && added.insert(cardId).second) cards.push_back(cardId);
	}
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		if (!mMercerShards.count(shard.id)) continue;
		for (size_t card = 0; card < shard.stock.size(); ++card)
		{
			int cardId = getCardIdFromName(shard.stock[card]);
			if (cardId >= 0 && added.insert(cardId).second) cards.push_back(cardId);
		}
	}
	return cards;
}

int Application::shopPrice(int cardId) const
{
	if (cardId < 0 || cardId >= (int)gCardDatabase.size()) return 0;
	int tier = std::max(1, std::min(5, gCardDatabase[cardId].PriceTier));
	return mMercerStock.prices[tier - 1];
}

void Application::handleShopEvent(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		logicalMouse(event.motion.x, event.motion.y, mMouseX, mMouseY);
		return;
	}
	if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0)
	{
		int pages = shopPageCount(shopInventory().size());
		int direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ?
			-event.wheel.y : event.wheel.y;
		if (direction > 0) mShopPage = std::max(0, mShopPage - 1);
		else mShopPage = std::min(pages - 1, mShopPage + 1);
		return;
	}
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			leaveShop();
			return;
		}
		if (key == SDLK_w || key == SDLK_UP || key == SDLK_s || key == SDLK_DOWN)
		{
			int pages = shopPageCount(shopInventory().size());
			if (key == SDLK_w || key == SDLK_UP)
				mShopPage = std::max(0, mShopPage - 1);
			else mShopPage = std::min(pages - 1, mShopPage + 1);
			return;
		}
	}
	if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;

	int x, y;
	logicalMouse(event.button.x, event.button.y, x, y);
	if (contains(SHOP_BACK_BUTTON, x, y))
	{
		leaveShop();
		return;
	}
	std::vector<int> inventory = shopInventory();
	int pages = shopPageCount(inventory.size());
	if (contains(SHOP_PREV_BUTTON, x, y))
	{
		mShopPage = std::max(0, mShopPage - 1);
		return;
	}
	if (contains(SHOP_NEXT_BUTTON, x, y))
	{
		mShopPage = std::min(pages - 1, mShopPage + 1);
		return;
	}
	if (contains(SHOP_GIVE_BUTTON, x, y))
	{
		for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
		{
			const MercerShard& shard = mMercerStock.shards[i];
			if (!mCollectedShards.count(shard.id) || mMercerShards.count(shard.id)) continue;
			int previousStockSize = (int)inventory.size();
			mMercerShards.insert(shard.id);
			int cardsAdded = (int)shopInventory().size() - previousStockSize;
			if (cardsAdded > 0) mShopPage = previousStockSize / SHOP_CARDS_PER_PAGE;
			savePlayerProgress();
			mShopNotice = "Gave Mercer " + shard.name + ". Added " +
				std::to_string(cardsAdded) + " cards to his stock.";
			mShopNoticeUntil = SDL_GetTicks() + 5000;
			return;
		}
		mShopNotice = "You are not carrying a shard Mercer can use.";
		mShopNoticeUntil = SDL_GetTicks() + 3500;
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
	drawText("Buy up to four copies of each card. Give Mercer shards to expand his stock.",
		205, 68, color(172, 190, 216), 15);

	std::vector<int> cards = shopInventory();
	int pages = shopPageCount(cards.size());
	mShopPage = std::max(0, std::min(pages - 1, mShopPage));
	fillRect(SHOP_PREV_BUTTON, 34, 50, 75, 250);
	outlineRect(SHOP_PREV_BUTTON, 112, 149, 205, 255, 2);
	drawText("Previous", SHOP_PREV_BUTTON.x + 14, SHOP_PREV_BUTTON.y + 8,
		color(238, 241, 247), 14);
	fillRect(SHOP_NEXT_BUTTON, 34, 50, 75, 250);
	outlineRect(SHOP_NEXT_BUTTON, 112, 149, 205, 255, 2);
	drawText("Next", SHOP_NEXT_BUTTON.x + 31, SHOP_NEXT_BUTTON.y + 8,
		color(238, 241, 247), 14);
	drawText("PAGE " + std::to_string(mShopPage + 1) + "/" + std::to_string(pages),
		188, 107, color(172, 190, 216), 14);
	drawText("W/S, arrows, or mouse wheel", 300, 107, color(143, 163, 190), 14);
	int heldShards = (int)mCollectedShards.size() - (int)mMercerShards.size();
	bool canGiveShard = heldShards > 0;
	fillRect(SHOP_GIVE_BUTTON, canGiveShard ? 66 : 47, canGiveShard ? 52 : 48,
		canGiveShard ? 94 : 54, 248);
	outlineRect(SHOP_GIVE_BUTTON, canGiveShard ? 190 : 103, canGiveShard ? 135 : 105,
		canGiveShard ? 225 : 112, 255, 2);
	drawText(canGiveShard ? "Give Mercer a shard (" + std::to_string(heldShards) + ")" :
		"No collected shards", SHOP_GIVE_BUTTON.x + 14, SHOP_GIVE_BUTTON.y + 8,
		canGiveShard ? color(245, 215, 255) : color(168, 172, 182), 14);

	int firstCard = mShopPage * SHOP_CARDS_PER_PAGE;
	int lastCard = std::min((int)cards.size(), firstCard + SHOP_CARDS_PER_PAGE);
	for (int i = firstCard; i < lastCard; ++i)
	{
		int pageIndex = i - firstCard;
		int column = pageIndex % 5;
		int row = pageIndex / 5;
		SDL_Rect cardRect = { 76 + column * 238, 142 + row * 282, 150, 208 };
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
			drawText("Tier " + std::to_string(gCardDatabase[cardId].PriceTier) + "  •  Owned " +
				std::to_string(owned) + "/4", pricePanel.x + 9, pricePanel.y + 27,
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
