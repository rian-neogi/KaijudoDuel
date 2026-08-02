#include "Application.h"

#include "AppSupport.h"

#include <algorithm>
#include <cmath>

using namespace AppSupport;

void Application::handleOverworldEvent(const SDL_Event& event)
{
	if (handleStoryEvent(event)) return;
	if ((event.type == SDL_KEYDOWN && !event.key.repeat) || event.type == SDL_KEYUP)
	{
		SDL_Keycode movementKey = event.key.keysym.sym;
		bool pressed = event.type == SDL_KEYDOWN;
		bool isMovementKey = true;
		if (movementKey == SDLK_w || movementKey == SDLK_UP) mMoveUp = pressed;
		else if (movementKey == SDLK_s || movementKey == SDLK_DOWN) mMoveDown = pressed;
		else if (movementKey == SDLK_a || movementKey == SDLK_LEFT) mMoveLeft = pressed;
		else if (movementKey == SDLK_d || movementKey == SDLK_RIGHT) mMoveRight = pressed;
		else isMovementKey = false;

		if (isMovementKey)
		{
			if (pressed)
			{
				mMoveIntentX = movementKey == SDLK_a || movementKey == SDLK_LEFT ? -1 :
					(movementKey == SDLK_d || movementKey == SDLK_RIGHT ? 1 : 0);
				mMoveIntentY = movementKey == SDLK_w || movementKey == SDLK_UP ? -1 :
					(movementKey == SDLK_s || movementKey == SDLK_DOWN ? 1 : 0);
				mFacingX = mMoveIntentX;
				mFacingY = mMoveIntentY;
			}
			else if ((mMoveIntentY < 0 && !mMoveUp) || (mMoveIntentY > 0 && !mMoveDown) ||
				(mMoveIntentX < 0 && !mMoveLeft) || (mMoveIntentX > 0 && !mMoveRight))
			{
				mMoveIntentX = mMoveLeft ? -1 : (mMoveRight ? 1 : 0);
				mMoveIntentY = mMoveUp ? -1 : (mMoveDown ? 1 : 0);
				if (mMoveIntentX != 0) mMoveIntentY = 0;
			}
			return;
		}
	}
	if (mPauseMenuOpen)
	{
		handlePauseMenuEvent(event);
		return;
	}
	if (event.type != SDL_KEYDOWN || event.key.repeat)
		return;
	SDL_Keycode key = event.key.keysym.sym;
	if (key == SDLK_ESCAPE)
	{
		if (mDialogueNpc >= 0) mDialogueNpc = -1;
		else
		{
			mPauseMenuOpen = true;
			mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
			mMoveIntentX = mMoveIntentY = 0;
		}
		return;
	}
	if (mDialogueNpc >= 0)
	{
		if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN) interact();
		return;
	}
	if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN) interact();
}

void Application::updateOverworld(Uint32 deltaTime)
{
	if (mPauseMenuOpen || mStoryScene != StoryScene::None) return;
	float playerDx = mPlayerX - mVisualX;
	float playerDy = mPlayerY - mVisualY;
	float playerDistance = std::sqrt(playerDx * playerDx + playerDy * playerDy);
	if (playerDistance <= 0.001f && mDialogueNpc < 0 && (mMoveIntentX != 0 || mMoveIntentY != 0))
	{
		mVisualX = (float)mPlayerX;
		mVisualY = (float)mPlayerY;
		tryMove(mMoveIntentX, mMoveIntentY);
		playerDx = mPlayerX - mVisualX;
		playerDy = mPlayerY - mVisualY;
		playerDistance = std::sqrt(playerDx * playerDx + playerDy * playerDy);
	}
	const float playerStep = 5.0f * deltaTime / 1000.f;
	if (playerDistance > 0.001f)
	{
		if (playerStep >= playerDistance)
		{
			mVisualX = (float)mPlayerX;
			mVisualY = (float)mPlayerY;
		}
		else
		{
			mVisualX += playerDx / playerDistance * playerStep;
			mVisualY += playerDy / playerDistance * playerStep;
		}
	}

	Uint32 now = SDL_GetTicks();
	const int directionX[] = { 0, 1, 0, -1 };
	const int directionY[] = { -1, 0, 1, 0 };
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (!npcVisible((int)i)) continue;
		Npc& npc = mNpcs[i];
		npc.updateMovement(deltaTime);
		if (!npc.canWander()) continue;
		if (npc.nextMoveAt == 0)
		{
			npc.scheduleWander(now);
			continue;
		}
		if (npc.isMoving() || now < npc.nextMoveAt || mDialogueNpc == (int)i) continue;

		for (int attempt = 0; attempt < 4; ++attempt)
		{
			int direction = npc.nextWanderDirection();
			int nextX = npc.x + directionX[direction];
			int nextY = npc.y + directionY[direction];
			if (std::abs(nextX - npc.homeX) > 1 || std::abs(nextY - npc.homeY) > 1) continue;
			if (!isWalkable(nextX, nextY) || npcAt(nextX, nextY, (int)i) >= 0) continue;
			if ((nextX == mPlayerX && nextY == mPlayerY) ||
				(nextX == (int)std::round(mVisualX) && nextY == (int)std::round(mVisualY))) continue;
			bool visuallyOccupied = false;
			for (size_t other = 0; other < mNpcs.size(); ++other)
				if (other != i && nextX == (int)std::round(mNpcs[other].visualX) &&
					nextY == (int)std::round(mNpcs[other].visualY)) visuallyOccupied = true;
			if (visuallyOccupied) continue;
			npc.x = nextX;
			npc.y = nextY;
			break;
		}
		npc.scheduleWander(now);
	}
}

bool Application::exerciseOverworldMovementSmoke()
{
	if (mNpcs.empty()) return false;
	int savedPlayerX = mPlayerX;
	int savedPlayerY = mPlayerY;
	float savedVisualX = mVisualX;
	float savedVisualY = mVisualY;
	int savedIntentX = mMoveIntentX;
	int savedIntentY = mMoveIntentY;
	mMoveIntentX = 1;
	mMoveIntentY = 0;
	updateOverworld(50);
	bool playerInterpolated = mPlayerX == savedPlayerX + 1 &&
		mVisualX > savedVisualX && mVisualX < (float)mPlayerX;
	mPlayerX = savedPlayerX;
	mPlayerY = savedPlayerY;
	mVisualX = savedVisualX;
	mVisualY = savedVisualY;
	mMoveIntentX = savedIntentX;
	mMoveIntentY = savedIntentY;

	Npc& npc = mNpcs[0];
	int savedNpcX = npc.x;
	int savedNpcY = npc.y;
	float savedNpcVisualX = npc.visualX;
	float savedNpcVisualY = npc.visualY;
	Uint32 savedNextMoveAt = npc.nextMoveAt;
	npc.nextMoveAt = SDL_GetTicks();
	updateOverworld(16);
	int targetDistance = std::abs(npc.x - npc.homeX) + std::abs(npc.y - npc.homeY);
	updateOverworld(50);
	bool npcInterpolated = targetDistance == 1 && npc.isMoving() &&
		(std::fabs(npc.visualX - savedNpcVisualX) > 0.001f ||
		 std::fabs(npc.visualY - savedNpcVisualY) > 0.001f);
	npc.x = savedNpcX;
	npc.y = savedNpcY;
	npc.visualX = savedNpcVisualX;
	npc.visualY = savedNpcVisualY;
	npc.nextMoveAt = savedNextMoveAt;
	return playerInterpolated && npcInterpolated;
}

bool Application::isWalkable(int x, int y) const
{
	if (y < 0 || y >= (int)mMap.size() || x < 0 || x >= (int)mMap[y].size()) return false;
	char tile = mMap[y][x];
	return tile == '.' || tile == '=';
}

int Application::npcAt(int x, int y, int ignoredNpc) const
{
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if ((int)i != ignoredNpc && npcVisible((int)i) && mNpcs[i].x == x && mNpcs[i].y == y) return (int)i;
	return -1;
}

void Application::tryMove(int dx, int dy)
{
	mFacingX = dx;
	mFacingY = dy;
	int x = mPlayerX + dx;
	int y = mPlayerY + dy;
	bool occupiedByMovingNpc = false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (npcVisible((int)i) && x == (int)std::round(mNpcs[i].visualX) &&
			y == (int)std::round(mNpcs[i].visualY))
			occupiedByMovingNpc = true;
	if (isWalkable(x, y) && npcAt(x, y) < 0 && !occupiedByMovingNpc)
	{
		mPlayerX = x;
		mPlayerY = y;
	}
}

void Application::interact()
{
	if (std::fabs(mPlayerX - mVisualX) > 0.001f || std::fabs(mPlayerY - mVisualY) > 0.001f) return;
	if (mDialogueNpc >= 0)
	{
		Npc& npc = mNpcs[mDialogueNpc];
		if (npc.isShopkeeper())
			enterShop();
		else if (!npc.canBattle())
			mDialogueNpc = -1;
		else
			startDuel(mDialogueNpc);
		return;
	}
	mDialogueNpc = npcAt(mPlayerX + mFacingX, mPlayerY + mFacingY);
	if (mDialogueNpc >= 0) discoverStoryClue(mDialogueNpc);
}

void Application::renderOverworld()
{
	ensurePlayerDataLoaded();
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 13, 21, 34);
	for (size_t y = 0; y < mMap.size(); ++y)
	{
		for (size_t x = 0; x < mMap[y].size(); ++x)
		{
			SDL_Rect tileRect = { MAP_X + (int)x * TILE, MAP_Y + (int)y * TILE, TILE, TILE };
			char tile = mMap[y][x];
			if (tile == '=') fillRect(tileRect, 162, 132, 76);
			else if (tile == '~') fillRect(tileRect, 25, 111, 157);
			else if (tile == 'H') fillRect(tileRect, 126, 65, 43);
			else if (tile == '#' || tile == 'T') fillRect(tileRect, 26, 75, 33);
			else fillRect(tileRect, 61, 139, 61);

			if (tile == '~')
			{
				int wave = (int)((SDL_GetTicks() / 180 + x * 5 + y * 3) % 24);
				fillRect({ tileRect.x + 7, tileRect.y + 10 + wave / 3, 28, 3 }, 92, 189, 210, 190);
			}
			else if (tile == '#' || tile == 'T')
			{
				fillRect({ tileRect.x + 19, tileRect.y + 27, 10, 18 }, 85, 48, 26);
				fillRect({ tileRect.x + 6, tileRect.y + 5, 36, 30 }, 41, 116, 49);
			}
			else if (tile == 'H')
			{
				fillRect({ tileRect.x + 3, tileRect.y + 4, 42, 16 }, 186, 76, 46);
				fillRect({ tileRect.x + 18, tileRect.y + 23, 13, 25 }, 54, 31, 24);
			}
			else if (tile == '.')
				fillRect({ tileRect.x + 7, tileRect.y + 34, 3, 8 }, 111, 180, 64);
		}
	}

	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (!npcVisible((int)i)) continue;
		drawCharacter(mNpcs[i].visualX, mNpcs[i].visualY, mNpcs[i].isDuelist(),
			mNpcs[i].isComplete(), mNpcs[i].isShopkeeper(), mNpcs[i].isMoving());
		if (npcHasStoryMarker((int)i))
		{
			int markerX = MAP_X + (int)std::round(mNpcs[i].visualX * TILE) + 17;
			int markerY = MAP_Y + (int)std::round(mNpcs[i].visualY * TILE) - 20;
			fillRect({ markerX - 4, markerY - 2, 20, 22 }, 31, 24, 14, 235);
			outlineRect({ markerX - 4, markerY - 2, 20, 22 }, 246, 203, 78, 255, 2);
			drawText("!", markerX + 2, markerY, color(255, 225, 111), 16);
		}
	}
	bool playerWalking = std::fabs(mPlayerX - mVisualX) > 0.001f ||
		std::fabs(mPlayerY - mVisualY) > 0.001f;
	drawCharacter(mVisualX, mVisualY, false, false, false, playerWalking);

	fillRect({ 1012, 28, 238, 670 }, 21, 28, 45, 245);
	outlineRect({ 1012, 28, 238, 670 }, 190, 146, 61, 255, 2);
	drawText("EMBERGLEN", 1034, 48, color(242, 205, 99), 28);
	drawText("GOLD " + std::to_string(mMoney), 1034, 91, color(245, 205, 88), 16);
	drawText("DUELISTS", 1034, 120, color(135, 162, 199), 16);
	int duelistRow = 0;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (mNpcs[i].kind != NpcKind::Duelist) continue;
		const int rowY = 145 + duelistRow++ * 48;
		drawText(mNpcs[i].name, 1034, rowY, color(235, 238, 245), 17);
		drawText(mNpcs[i].statusText(), 1034, rowY + 21,
			mNpcs[i].isComplete() ? color(92, 208, 121) : color(235, 151, 65), 12);
	}
	drawText("CARD SHOP", 1034, 535, color(135, 162, 199), 14);
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].isShopkeeper()) drawText(mNpcs[i].name, 1034, 557, color(235, 238, 245), 17);
	drawText("WASD / Arrows: move", 1034, 613, color(187, 200, 221), 14);
	drawText("E / Space: talk", 1034, 636, color(187, 200, 221), 14);
	drawText("Esc: menu", 1034, 659, color(187, 200, 221), 14);
	renderStoryTracker();

	if (!mNotice.empty() && SDL_GetTicks() < mNoticeUntil)
	{
		fillRect({ 36, 10, 930, 42 }, 17, 28, 43, 230);
		drawText(mNotice, 48, 19, color(113, 232, 143), 16, 900);
	}

	if (mDialogueNpc >= 0)
	{
		const Npc& npc = mNpcs[mDialogueNpc];
		fillRect({ 40, 646, 1200, 128 }, 16, 22, 36, 248);
		outlineRect({ 40, 646, 1200, 128 }, 194, 148, 62, 255, 3);
		drawText(npc.name, 68, 664, color(244, 206, 103), 25);
		std::string dialogue = storyDialogueForNpc(mDialogueNpc);
		std::string prompt = npc.isShopkeeper() ? "E / Space to browse" : "E / Space to battle";
		if (npc.isComplete())
		{
			prompt = "E / Space to close";
		}
		else if (npc.isDuelist()) prompt = npc.rankName() + "  •  E / Space to battle";
		drawText(dialogue, 68, 704, color(232, 237, 246), 19, 1080);
		drawText(prompt, 965, 742, color(126, 176, 242), 15);
	}
	renderStoryScene();
	if (mPauseMenuOpen) renderPauseMenu();
}

void Application::drawCharacter(
	float gridX, float gridY, bool rival, bool completed, bool shopkeeper, bool walking)
{
	int x = MAP_X + (int)std::round(gridX * TILE);
	int y = MAP_Y + (int)std::round(gridY * TILE);
	int stride = walking && (SDL_GetTicks() / 110) % 2 == 0 ? 2 : (walking ? -2 : 0);
	int bob = walking && (SDL_GetTicks() / 110) % 2 == 0 ? -1 : 0;
	fillRect({ x + 11, y + 39, 28, 6 }, 8, 14, 18, 100);
	fillRect({ x + 17 + stride, y + 35, 6, 9 }, 31, 38, 53);
	fillRect({ x + 27 - stride, y + 35, 6, 9 }, 31, 38, 53);
	y += bob;
	if (shopkeeper)
		fillRect({ x + 13, y + 20, 22, 22 }, 173, 119, 38);
	else if (rival)
		fillRect({ x + 13, y + 20, 22, 22 }, completed ? 91 : 111, completed ? 94 : 46, completed ? 103 : 143);
	else
		fillRect({ x + 13, y + 20, 22, 22 }, 31, 88, 185);
	fillRect({ x + 17, y + 8, 15, 15 }, 224, 172, 126);
	fillRect({ x + 14, y + 5, 21, 8 }, rival ? 41 : 91, rival ? 24 : 48, rival ? 58 : 22);
}
