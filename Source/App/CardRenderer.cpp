#include "Application.h"

#include "AppSupport.h"
#include "Game/Card.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace AppSupport;

namespace
{
	constexpr Uint32 NON_HAND_HOVER_DELAY_MS = 1000;
}

void Application::drawZone(const std::vector<Card*>& cards, int x, int y, int width, int cardWidth, int cardHeight, bool faceUp, bool clickable)
{
	if (cards.empty()) return;
	int step = cards.size() == 1 ? cardWidth : std::min(cardWidth + 8, std::max(22, (width - cardWidth) / ((int)cards.size() - 1)));
	int total = cardWidth + step * ((int)cards.size() - 1);
	int start = x + std::max(0, (width - total) / 2);
	for (size_t i = 0; i < cards.size(); ++i)
	{
		SDL_Rect rect = { start + (int)i * step, y, cardWidth, cardHeight };
		drawCard(cards[i], rect, faceUp, cards[i]->mUniqueId == mSelectedCard, clickable);
	}
}

void Application::drawHand(const std::vector<Card*>& cards, bool opponent)
{
	if (cards.empty()) return;
	const int cardWidth = opponent ? 76 : 88;
	const int cardHeight = opponent ? 106 : 122;
	const int availableWidth = opponent ? 440 : 660;
	const float midpoint = ((float)cards.size() - 1.f) * 0.5f;
	const int step = cards.size() == 1 ? cardWidth :
		std::min(cardWidth - 8, std::max(30, (availableWidth - cardWidth) / ((int)cards.size() - 1)));
	const int totalWidth = cardWidth + step * ((int)cards.size() - 1);
	const int startX = 490 - totalWidth / 2;
	for (size_t i = 0; i < cards.size(); ++i)
	{
		const float offset = (float)i - midpoint;
		const float angle = std::max(-22.f, std::min(22.f, offset * (opponent ? -5.f : 5.f)));
		const int arc = (int)std::round(std::fabs(offset) * 4.f);
		SDL_Rect rect = { startX + (int)i * step, opponent ? -48 + arc : 650 + arc, cardWidth, cardHeight };
		drawCard(cards[i], rect, !opponent, cards[i]->mUniqueId == mSelectedCard, !opponent, angle);
	}
}

void Application::drawCard(Card* card, const SDL_Rect& rect, bool faceUp, bool selected, bool clickable, float angle)
{
	AnimatedCard& animation = mCardAnimations[card->mUniqueId];
	AnimatedCard originalLayout;
	originalLayout.x = (float)rect.x;
	originalLayout.y = (float)rect.y;
	originalLayout.width = (float)rect.w;
	originalLayout.height = (float)rect.h;
	originalLayout.angle = angle + (faceUp && card->mIsTapped ? 90.f : 0.f);
	SDL_Rect originalBounds = cardBounds(originalLayout);
	const bool poppedOut = faceUp && card->mUniqueId == mHoveredCard && mDraggingCard < 0;
	if (poppedOut)
	{
		const int popupWidth = 280;
		const int popupHeight = 400;
		animation.targetX = (float)std::max(12, std::min(968 - popupWidth, rect.x + rect.w / 2 - popupWidth / 2));
		int popupY = rect.y + (rect.y > 400 ? rect.h - popupHeight : 0);
		animation.targetY = (float)std::max(10, std::min(790 - popupHeight, popupY));
		animation.targetWidth = (float)popupWidth;
		animation.targetHeight = (float)popupHeight;
		animation.targetAngle = 0.f;
	}
	else
	{
		animation.targetX = (float)rect.x;
		animation.targetY = (float)rect.y;
		animation.targetWidth = (float)rect.w;
		animation.targetHeight = (float)rect.h;
		animation.targetAngle = angle + (faceUp && card->mIsTapped ? 90.f : 0.f);
	}
	if (!animation.initialized)
	{
		animation.x = animation.targetX;
		animation.y = animation.targetY;
		animation.width = animation.targetWidth;
		animation.height = animation.targetHeight;
		animation.angle = animation.targetAngle;
		animation.initialized = true;
	}

	SDL_Rect drawRect = {
		(int)std::round(animation.x), (int)std::round(animation.y),
		(int)std::round(animation.width), (int)std::round(animation.height)
	};
	SDL_Rect bounds = cardBounds(animation);
	bool floatingHandCard = mDraggingCard == card->mUniqueId && mDragFromZone == ZONE_HAND;
	if (!floatingHandCard && !poppedOut)
	{
		SDL_Texture* texture = faceUp ? cardTexture(card) : mCardBackTexture;
		if (texture != NULL)
			SDL_RenderCopyEx(mRenderer, texture, NULL, &drawRect, animation.angle, NULL, SDL_FLIP_NONE);
		else if (!faceUp)
			drawCardBack(bounds);
		else
		{
			SDL_Color civ = civilizationColor(card->mCivilization);
			fillRect(bounds, 224, 210, 177);
			outlineRect(bounds, civ.r, civ.g, civ.b, 255, 3);
			drawText(card->mName, bounds.x + 5, bounds.y + bounds.h / 3, color(37, 28, 23), 11, bounds.w - 10);
		}

		SDL_Color border = faceUp ? civilizationColor(card->mCivilization) : color(202, 154, 66);
		if (selected) border = color(255, 224, 104);
		outlineRect(bounds, border.r, border.g, border.b, selected ? 255 : 210, selected ? 4 : 2);
		if (faceUp && card->mType == TYPE_CREATURE && card->mZone != ZONE_HAND)
		{
			SDL_Rect powerBadge = { bounds.x + bounds.w - 27, bounds.y + bounds.h - 23, 25, 21 };
			fillRect(powerBadge, 18, 25, 35, 225);
			int power = mDuel->mLuaCallbackSuspended ? card->mPower : mDuel->getCreaturePower(card->mUniqueId);
			drawText(std::to_string(power), powerBadge.x + 3, powerBadge.y + 2,
				color(255, 242, 197), 11);
		}
	}
	if (clickable && !floatingHandCard)
	{
		const bool immediateHover = card->mZone == ZONE_HAND;
		if (poppedOut)
		{
			// The original card remains the only hover anchor. The enlarged card is
			// clickable, but moving onto it after leaving this box ends the hover.
			mCardHitboxes.push_back({ originalBounds, card->mUniqueId, faceUp, true, immediateHover });
		}
		mCardHitboxes.push_back({ bounds, card->mUniqueId, faceUp, !poppedOut, immediateHover });
	}
}

void Application::updateHoverState(int candidateCard, bool immediate, Uint32 now)
{
	if (candidateCard < 0 || mDraggingCard >= 0)
	{
		mHoveredCard = -1;
		mHoverCandidateCard = -1;
		mHoverCandidateSince = 0;
		return;
	}

	if (candidateCard != mHoverCandidateCard)
	{
		mHoverCandidateCard = candidateCard;
		mHoverCandidateSince = now;
		mHoveredCard = -1;
	}

	if (immediate || now - mHoverCandidateSince >= NON_HAND_HOVER_DELAY_MS)
		mHoveredCard = candidateCard;
	else
		mHoveredCard = -1;
}

int Application::duelHoverCandidateAt(int x, int y, bool& immediate) const
{
	immediate = false;
	// Overlapping fanned cards must not steal hover from the card that already
	// owns it. Keep that owner until the pointer leaves its original anchor.
	if (mHoverCandidateCard >= 0)
	{
		for (std::vector<CardHitbox>::const_reverse_iterator item = mCardHitboxes.rbegin();
			item != mCardHitboxes.rend(); ++item)
		{
			if (item->cardId == mHoverCandidateCard && item->faceUp && item->hoverAnchor &&
				contains(item->rect, x, y))
			{
				immediate = item->immediateHover;
				return item->cardId;
			}
		}
	}
	for (std::vector<CardHitbox>::const_reverse_iterator item = mCardHitboxes.rbegin();
		item != mCardHitboxes.rend(); ++item)
	{
		if (item->faceUp && item->hoverAnchor && contains(item->rect, x, y))
		{
			immediate = item->immediateHover;
			return item->cardId;
		}
	}
	return -1;
}

bool Application::exerciseHoverTimingSmoke()
{
	bool valid = true;
	updateHoverState(42, false, 1000);
	valid = valid && mHoveredCard == -1;
	updateHoverState(42, false, 1999);
	valid = valid && mHoveredCard == -1;
	updateHoverState(42, false, 2000);
	valid = valid && mHoveredCard == 42;
	updateHoverState(43, false, 2001);
	valid = valid && mHoveredCard == -1;
	updateHoverState(-1, false, 2002);
	valid = valid && mHoveredCard == -1 && mHoverCandidateCard == -1;
	updateHoverState(7, true, 3000);
	valid = valid && mHoveredCard == 7;
	updateHoverState(-1, false, 3001);
	valid = valid && mHoveredCard == -1;

	std::vector<CardHitbox> savedHitboxes = mCardHitboxes;
	int savedCandidate = mHoverCandidateCard;
	mCardHitboxes.clear();
	SDL_Rect overlap = { 100, 100, 80, 120 };
	mCardHitboxes.push_back({ overlap, 11, true, true, true });
	mCardHitboxes.push_back({ overlap, 12, true, true, true });
	mHoverCandidateCard = 11;
	bool immediate = false;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == 11 && immediate;
	mHoverCandidateCard = -1;
	valid = valid && duelHoverCandidateAt(140, 150, immediate) == 12;
	mCardHitboxes = savedHitboxes;
	mHoverCandidateCard = savedCandidate;
	return valid;
}

void Application::drawCardBack(const SDL_Rect& rect)
{
	if (mCardBackTexture != NULL)
	{
		SDL_RenderCopy(mRenderer, mCardBackTexture, NULL, &rect);
		outlineRect(rect, 202, 154, 66, 255, 2);
		return;
	}
	fillRect(rect, 38, 65, 103);
	outlineRect(rect, 202, 154, 66, 255, 3);
	SDL_Rect inset = { rect.x + 7, rect.y + 7, rect.w - 14, rect.h - 14 };
	outlineRect(inset, 100, 148, 190, 255, 2);
	fillRect({ rect.x + rect.w / 2 - 7, rect.y + rect.h / 2 - 7, 14, 14 }, 196, 72, 50);
}

void Application::updateCardAnimations(Uint32 deltaTime)
{
	float blend = 1.f - std::exp(-0.012f * (float)deltaTime);
	for (std::map<int, AnimatedCard>::iterator item = mCardAnimations.begin(); item != mCardAnimations.end(); ++item)
	{
		AnimatedCard& card = item->second;
		if (!card.initialized) continue;
		card.x += (card.targetX - card.x) * blend;
		card.y += (card.targetY - card.y) * blend;
		card.width += (card.targetWidth - card.width) * blend;
		card.height += (card.targetHeight - card.height) * blend;
		card.angle += (card.targetAngle - card.angle) * blend;
	}
}

SDL_Rect Application::cardBounds(const AnimatedCard& animation) const
{
	const float radians = animation.angle * 3.14159265358979323846f / 180.f;
	const float cosine = std::fabs(std::cos(radians));
	const float sine = std::fabs(std::sin(radians));
	const int width = (int)std::ceil(animation.width * cosine + animation.height * sine);
	const int height = (int)std::ceil(animation.width * sine + animation.height * cosine);
	const int centerX = (int)std::round(animation.x + animation.width * 0.5f);
	const int centerY = (int)std::round(animation.y + animation.height * 0.5f);
	return { centerX - width / 2, centerY - height / 2, width, height };
}

SDL_Texture* Application::cardTexture(Card* card)
{
	return cardTextureById(card->mCardId);
}

SDL_Texture* Application::cardTextureById(int cardId)
{
	std::map<int, SDL_Texture*>::iterator existing = mCardTextures.find(cardId);
	if (existing != mCardTextures.end()) return existing->second;
	if (cardId < 0 || cardId >= (int)gCardDatabase.size()) return NULL;
	const CardData& card = gCardDatabase[cardId];

	static const char* pngSets[] = {
		"Base Set", "Evo-Crushinators of Doom", "Rampage of the Super Warriors",
		"Shadowclash of Blinding Night", "Survivors of the Megapocalypse",
		"Stomp-A-Trons of Invincible Wrath", "Promo"
	};
	SDL_Texture* loaded = NULL;
	for (size_t i = 0; i < sizeof(pngSets) / sizeof(pngSets[0]) && loaded == NULL; ++i)
	{
		std::string path = "Resources/Cards/" + std::string(pngSets[i]) + "/" + card.Name + ".png";
		loaded = IMG_LoadTexture(mRenderer, path.c_str());
	}
	if (loaded == NULL)
	{
		std::string path = "Resources/Cards/Textures/Sets/" + card.Set + "/Cards/" + card.Name + ".jpg";
		loaded = IMG_LoadTexture(mRenderer, path.c_str());
	}
	if (loaded != NULL) SDL_SetTextureBlendMode(loaded, SDL_BLENDMODE_BLEND);
	else std::cerr << "No card image found for " << card.Name << std::endl;
	mCardTextures[cardId] = loaded;
	return loaded;
}

void Application::destroyCardTextures()
{
	for (std::map<int, SDL_Texture*>::iterator item = mCardTextures.begin(); item != mCardTextures.end(); ++item)
		if (item->second != NULL) SDL_DestroyTexture(item->second);
	mCardTextures.clear();
}

void Application::renderDragOverlay()
{
	if (mDraggingCard < 0 || mDuel == NULL || mDraggingCard >= (int)mDuel->mCardList.size()) return;
	Card* card = mDuel->mCardList[mDraggingCard];
	if (mDragFromZone == ZONE_HAND)
	{
		fillRect({ 55, 330, 890, 190 }, 69, 151, 97, 34);
		outlineRect({ 55, 330, 890, 190 }, 94, 224, 132, 220, 3);
		fillRect({ 500, 535, 455, 125 }, 67, 115, 188, 30);
		outlineRect({ 500, 535, 455, 125 }, 103, 170, 242, 220, 3);
		drawText("SUMMON", 72, 340, color(126, 239, 157), 16);
		drawText("MANA", 515, 540, color(131, 191, 251), 14);

		SDL_Rect floating = { mDragMouseX - 49, mDragMouseY - 68, 98, 136 };
		SDL_Texture* texture = cardTexture(card);
		if (texture != NULL) SDL_RenderCopy(mRenderer, texture, NULL, &floating);
		else
		{
			fillRect(floating, 224, 210, 177);
			drawText(card->mName, floating.x + 7, floating.y + 45, color(37, 28, 23), 12, floating.w - 14);
		}
		outlineRect(floating, 255, 224, 104, 255, 4);
	}
	else if (mDragFromZone == ZONE_BATTLE)
	{
		int startX = mDragOrigin.x + mDragOrigin.w / 2;
		int startY = mDragOrigin.y + mDragOrigin.h / 2;
		setColor(242, 82, 68, 235);
		for (int offset = -2; offset <= 2; ++offset)
			SDL_RenderDrawLine(mRenderer, startX + offset, startY, mDragMouseX + offset, mDragMouseY);
		fillRect({ mDragMouseX - 8, mDragMouseY - 8, 16, 16 }, 242, 82, 68, 235);
		outlineRect({ 150, 0, 760, 200 }, 242, 82, 68, 210, 3);
	}
}

void Application::renderHoverPreview()
{
	if (mHoveredCard < 0 || mDuel == NULL || mHoveredCard >= (int)mDuel->mCardList.size()) return;

	Card* card = mDuel->mCardList[mHoveredCard];
	std::map<int, AnimatedCard>::iterator found = mCardAnimations.find(mHoveredCard);
	if (found == mCardAnimations.end() || !found->second.initialized) return;
	AnimatedCard& animation = found->second;
	SDL_Rect preview = {
		(int)std::round(animation.x), (int)std::round(animation.y),
		(int)std::round(animation.width), (int)std::round(animation.height)
	};
	SDL_Rect bounds = cardBounds(animation);
	fillRect({ bounds.x + 9, bounds.y + 11, bounds.w, bounds.h }, 5, 8, 13, 150);
	SDL_Texture* texture = cardTexture(card);
	if (texture != NULL)
		SDL_RenderCopyEx(mRenderer, texture, NULL, &preview, animation.angle, NULL, SDL_FLIP_NONE);
	else
	{
		fillRect(bounds, 224, 210, 177);
		drawText(card->mName, bounds.x + 14, bounds.y + bounds.h / 3, color(37, 28, 23), 18, bounds.w - 28);
	}
	SDL_Color border = civilizationColor(card->mCivilization);
	outlineRect(bounds, border.r, border.g, border.b, 255, 5);
	if (card->mType == TYPE_CREATURE && card->mZone != ZONE_HAND)
	{
		int power = mDuel->mLuaCallbackSuspended ? card->mPower : mDuel->getCreaturePower(card->mUniqueId);
		SDL_Rect badge = { bounds.x + bounds.w - 58, bounds.y + bounds.h - 43, 50, 34 };
		fillRect(badge, 17, 23, 34, 235);
		drawText(std::to_string(power), badge.x + 7, badge.y + 5, color(255, 242, 197), 17);
	}
}
