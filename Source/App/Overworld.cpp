#include "Application.h"

#include "AppSupport.h"

#include <algorithm>
#include <cmath>

using namespace AppSupport;

namespace
{
	constexpr Uint32 DOOR_OPEN_DURATION = 400;
	constexpr Uint32 DIALOGUE_CHARACTER_DELAY = 18;
}

void Application::handleOverworldEvent(const SDL_Event& event)
{
	if (mDialogueNpc >= 0)
	{
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
			advanceDialogue();
		else if (event.type == SDL_KEYDOWN && !event.key.repeat)
		{
			SDL_Keycode key = event.key.keysym.sym;
			if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN)
				advanceDialogue();
			else if (key == SDLK_ESCAPE && mDialogueAction == DialogueAction::NpcInteraction)
				clearDialogue();
		}
		return;
	}
	if (handleStoryEvent(event)) return;
	if (mPauseMenuOpen)
	{
		handlePauseMenuEvent(event);
		return;
	}
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
	if (event.type != SDL_KEYDOWN || event.key.repeat)
		return;
	SDL_Keycode key = event.key.keysym.sym;
	if (key == SDLK_ESCAPE)
	{
		mPauseMenuOpen = true;
		mPauseMenuSelection = 0;
		mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
		mMoveIntentX = mMoveIntentY = 0;
		return;
	}
	if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN) interact();
}

void Application::updateOverworld(Uint32 deltaTime)
{
	updateDialogue(deltaTime);
	if (mPauseMenuOpen || (mStoryScene != StoryScene::None && mDialogueNpc < 0)) return;
	float playerDx = mPlayerX - mVisualX;
	float playerDy = mPlayerY - mVisualY;
	float playerDistance = std::sqrt(playerDx * playerDx + playerDy * playerDy);
	if (playerDistance <= 0.001f && mOpeningPortal < 0 && mDialogueNpc < 0 &&
		(mMoveIntentX != 0 || mMoveIntentY != 0))
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
	if (mOpeningPortal >= 0)
	{
		if (playerDistance <= 0.001f && now - mPortalAnimationStarted >= DOOR_OPEN_DURATION)
			activatePortalAt(mPlayerX, mPlayerY);
		return;
	}
	const int directionX[] = { 0, 1, 0, -1 };
	const int directionY[] = { -1, 0, 1, 0 };
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (!npcVisible((int)i) || mNpcs[i].mapId != currentMapId()) continue;
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

bool Application::isWalkable(int x, int y) const
{
	const std::vector<std::string>& map = currentMap();
	if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size()) return false;
	char tile = map[y][x];
	return tile == '.' || tile == '=' || tile == 'F' || tile == 'D' || tile == 'S' ||
		tile == 'X' || tile == 'G' || tile == 'U';
}

int Application::npcAt(int x, int y, int ignoredNpc) const
{
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if ((int)i != ignoredNpc && npcVisible((int)i) && mNpcs[i].mapId == currentMapId() &&
			mNpcs[i].x == x && mNpcs[i].y == y) return (int)i;
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
		if (npcVisible((int)i) && mNpcs[i].mapId == currentMapId() &&
			x == (int)std::round(mNpcs[i].visualX) &&
			y == (int)std::round(mNpcs[i].visualY))
			occupiedByMovingNpc = true;
	if (isWalkable(x, y) && npcAt(x, y) < 0 && !occupiedByMovingNpc)
	{
		mPlayerX = x;
		mPlayerY = y;
		if (!beginPortalAt(x, y)) collectShardAt(x, y);
	}
}

void Application::collectShardAt(int x, int y)
{
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		if (shard.mapId != currentMapId() || shard.x != x || shard.y != y) continue;
		ensurePlayerDataLoaded();
		if (mCollectedShards.count(shard.id)) return;
		mCollectedShards.insert(shard.id);
		savePlayerProgress();
		mNotice = "Found " + shard.name + "! Take it to Mercer to expand his stock.";
		mNoticeUntil = SDL_GetTicks() + 6000;
		return;
	}
}

void Application::interact()
{
	if (mOpeningPortal >= 0) return;
	if (std::fabs(mPlayerX - mVisualX) > 0.001f || std::fabs(mPlayerY - mVisualY) > 0.001f) return;
	int npcIndex = npcAt(mPlayerX + mFacingX, mPlayerY + mFacingY);
	if (npcIndex < 0) return;
	discoverStoryClue(npcIndex);
	beginDialogue(npcIndex, storyDialogueForNpc(npcIndex), DialogueAction::NpcInteraction);
}

void Application::beginDialogue(int npcIndex, const std::string& text, DialogueAction action)
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return;
	mDialogueNpc = npcIndex;
	mDialogueText = text;
	mDialogueVisibleBytes = 0;
	mDialogueCharacterAccumulator = 0;
	mDialogueAction = action;
	mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
	mMoveIntentX = mMoveIntentY = 0;
}

void Application::clearDialogue()
{
	mDialogueNpc = -1;
	mDialogueText.clear();
	mDialogueVisibleBytes = 0;
	mDialogueCharacterAccumulator = 0;
	mDialogueAction = DialogueAction::None;
}

void Application::advanceDialogue()
{
	if (mDialogueNpc < 0) return;
	if (mDialogueVisibleBytes < mDialogueText.size())
	{
		mDialogueVisibleBytes = mDialogueText.size();
		mDialogueCharacterAccumulator = 0;
		return;
	}

	int npcIndex = mDialogueNpc;
	DialogueAction action = mDialogueAction;
	clearDialogue();
	if (action == DialogueAction::ShowReward)
	{
		mRewardCardId = mPendingRewardCardId;
		mRewardGold = mPendingRewardGold;
		mPendingRewardCardId = -1;
		mPendingRewardGold = 0;
	}
	else if (action == DialogueAction::NpcInteraction &&
		npcIndex >= 0 && npcIndex < (int)mNpcs.size())
	{
		Npc& npc = mNpcs[npcIndex];
		if (npc.isShopkeeper()) enterShop();
		else if (npc.canBattle()) startDuel(npcIndex);
	}
}

void Application::updateDialogue(Uint32 deltaTime)
{
	if (mDialogueNpc < 0 || mDialogueVisibleBytes >= mDialogueText.size()) return;
	mDialogueCharacterAccumulator += deltaTime;
	while (mDialogueCharacterAccumulator >= DIALOGUE_CHARACTER_DELAY &&
		mDialogueVisibleBytes < mDialogueText.size())
	{
		mDialogueCharacterAccumulator -= DIALOGUE_CHARACTER_DELAY;
		++mDialogueVisibleBytes;
		while (mDialogueVisibleBytes < mDialogueText.size() &&
			(static_cast<unsigned char>(mDialogueText[mDialogueVisibleBytes]) & 0xc0) == 0x80)
			++mDialogueVisibleBytes;
	}
}

float Application::overworldCameraX() const
{
	const std::vector<std::string>& map = currentMap();
	float maximum = (float)std::max(0, (int)map[0].size() - MAP_VIEW_COLUMNS);
	return std::max(0.f, std::min(maximum,
		mVisualX - (MAP_VIEW_COLUMNS - 1) * 0.5f));
}

float Application::overworldCameraY() const
{
	const std::vector<std::string>& map = currentMap();
	float maximum = (float)std::max(0, (int)map.size() - MAP_VIEW_ROWS);
	return std::max(0.f, std::min(maximum,
		mVisualY - (MAP_VIEW_ROWS - 1) * 0.5f));
}

void Application::renderOverworld()
{
	ensurePlayerDataLoaded();
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 13, 21, 34);
	const std::vector<std::string>& map = currentMap();
	const bool cinderrail = currentMapId() == "cinderrail";
	const bool oldRoad = currentMapId() == "old_road";
	int mapX = mapOriginX((int)map[0].size()) -
		(int)std::round(overworldCameraX() * TILE);
	int mapY = mapOriginY((int)map.size()) -
		(int)std::round(overworldCameraY() * TILE);
	SDL_Rect mapViewport = { MAP_X, MAP_Y, MAP_VIEW_WIDTH, MAP_VIEW_HEIGHT };
	SDL_RenderSetClipRect(mRenderer, &mapViewport);
	for (size_t y = 0; y < map.size(); ++y)
	{
		for (size_t x = 0; x < map[y].size(); ++x)
		{
			SDL_Rect tileRect = { mapX + (int)x * TILE, mapY + (int)y * TILE, TILE, TILE };
			char tile = map[y][x];
			if (tile == '=') fillRect(tileRect, oldRoad ? 124 : (cinderrail ? 116 : 162),
				oldRoad ? 112 : (cinderrail ? 91 : 132), oldRoad ? 88 : (cinderrail ? 58 : 76));
			else if (tile == '~') fillRect(tileRect, 25, 111, 157);
			else if (tile == 'H') fillRect(tileRect, cinderrail ? 111 : 126,
				cinderrail ? 55 : 65, cinderrail ? 39 : 43);
			else if (tile == '#' || tile == 'T') fillRect(tileRect,
				cinderrail && tile == '#' ? 55 : 26,
				cinderrail && tile == '#' ? 45 : 75,
				cinderrail && tile == '#' ? 43 : 33);
			else if (tile == 'S') fillRect(tileRect, cinderrail ? 118 : 188,
				cinderrail ? 72 : 151, cinderrail ? 48 : 87);
			else if (tile == 'M') fillRect(tileRect, oldRoad ? 79 : 198,
				oldRoad ? 72 : 204, oldRoad ? 65 : 207);
			else if (tile == 'Q') fillRect(tileRect, 201, 196, 177);
			else if (tile == 'B') fillRect(tileRect, 83, 64, 42);
			else if (tile == 'A') fillRect(tileRect, 61, 139, 61);
			else if (tile == 'E') fillRect(tileRect, 137, 91, 49);
			else if (tile == 'R' || tile == 'X') fillRect(tileRect,
				tile == 'X' ? 104 : 47, tile == 'X' ? 83 : 45, tile == 'X' ? 57 : 43);
			else if (tile == 'G') fillRect(tileRect, 73, 80, 86);
			else if (tile == 'I') fillRect(tileRect, 112, 49, 38);
			else if (tile == 'P') fillRect(tileRect, 63, 66, 67);
			else if (tile == 'V') fillRect(tileRect, 64, 47, 43);
			else if (tile == 'K') fillRect(tileRect, 91, 48, 37);
			else if (tile == 'J') fillRect(tileRect, 67, 68, 70);
			else if (tile == 'U') fillRect(tileRect, 31, 99, 132);
			else if (tile == 'O') fillRect(tileRect, 73, 65, 59);
			else if (tile == 'W' || tile == 'D' || tile == 'F' || tile == 'C')
				fillRect(tileRect, tile == 'F' ? 137 : 91, tile == 'F' ? 91 : 53, tile == 'F' ? 49 : 31);
			else fillRect(tileRect, cinderrail ? 82 : 61, cinderrail ? 76 : 139,
				cinderrail ? 59 : 61);

			if (tile == '~')
			{
				int wave = (int)((SDL_GetTicks() / 180 + x * 5 + y * 3) % 24);
				fillRect({ tileRect.x + 7, tileRect.y + 10 + wave / 3, 28, 3 }, 92, 189, 210, 190);
			}
			else if (tile == '#' && cinderrail)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 5, 42, 38 }, 72, 58, 54);
				fillRect({ tileRect.x + 7, tileRect.y + 11, 17, 4 }, 91, 70, 61);
				fillRect({ tileRect.x + 27, tileRect.y + 25, 14, 4 }, 39, 35, 37);
				fillRect({ tileRect.x + 18, tileRect.y + 15, 4, 13 }, 45, 39, 40);
			}
			else if (tile == '#' || tile == 'T')
			{
				fillRect({ tileRect.x + 19, tileRect.y + 27, 10, 18 }, 85, 48, 26);
				fillRect({ tileRect.x + 6, tileRect.y + 5, 36, 30 }, 41, 116, 49);
			}
			else if (tile == 'H' && cinderrail)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 8, 42, 35 }, 139, 60, 43);
				fillRect({ tileRect.x, tileRect.y + 5, 48, 8 }, 63, 56, 55);
				fillRect({ tileRect.x + 7, tileRect.y + 17, 12, 10 }, 231, 177, 72);
				fillRect({ tileRect.x + 27, tileRect.y + 18, 12, 25 }, 66, 55, 51);
				fillRect({ tileRect.x + 30, tileRect.y + 22, 5, 8 }, 222, 143, 56);
			}
			else if (tile == 'H')
			{
				fillRect({ tileRect.x + 3, tileRect.y + 4, 42, 16 }, 186, 76, 46);
				fillRect({ tileRect.x + 18, tileRect.y + 23, 13, 25 }, 54, 31, 24);
			}
			else if (tile == 'U')
			{
				fillRect({ tileRect.x, tileRect.y + 6, 48, 36 }, 31, 99, 132);
				int current = (int)((SDL_GetTicks() / 190 + x * 2 + y) % 13);
				fillRect({ tileRect.x + 4 + current, tileRect.y + 18, 20, 3 }, 92, 176, 190);
				for (int plank = 0; plank < 48; plank += 9)
				{
					fillRect({ tileRect.x + plank, tileRect.y + 8, 8, 32 }, 129, 82, 45);
					fillRect({ tileRect.x + plank + 1, tileRect.y + 10, 6, 3 }, 172, 117, 62);
				}
				fillRect({ tileRect.x, tileRect.y + 5, 48, 5 }, 66, 45, 32);
				fillRect({ tileRect.x, tileRect.y + 39, 48, 5 }, 66, 45, 32);
				fillRect({ tileRect.x, tileRect.y + 7, 48, 2 }, 218, 169, 83);
			}
			else if (tile == 'O')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 91, 80, 69);
				fillRect({ tileRect.x + 5, tileRect.y + 8, 19, 8 }, 116, 99, 78);
				fillRect({ tileRect.x + 27, tileRect.y + 18, 15, 9 }, 58, 55, 54);
				fillRect({ tileRect.x + 10, tileRect.y + 27, 24, 4 }, 66, 60, 56);
				fillRect({ tileRect.x + 20, tileRect.y + 14, 3, 14 }, 54, 51, 50);
				fillRect({ tileRect.x + 33, tileRect.y + 7, 7, 3 }, 143, 118, 84);
			}
			else if (tile == 'K')
			{
				fillRect({ tileRect.x, tileRect.y + 3, 48, 42 }, 111, 56, 40);
				fillRect({ tileRect.x, tileRect.y + 2, 48, 7 }, 63, 39, 34);
				for (int shingle = 0; shingle < 4; ++shingle)
				{
					int shingleY = tileRect.y + 11 + shingle * 9;
					fillRect({ tileRect.x, shingleY, 48, 3 }, 67, 38, 33);
					int offset = shingle % 2 == 0 ? 5 : 13;
					for (int seam = offset; seam < 48; seam += 17)
						fillRect({ tileRect.x + seam, shingleY - 6, 2, 6 }, 79, 42, 34);
				}
				fillRect({ tileRect.x, tileRect.y + 42, 48, 5 }, 50, 34, 31);
				if (x == 0 || map[y][x - 1] != 'K')
					fillRect({ tileRect.x, tileRect.y + 4, 4, 41 }, 48, 33, 30);
				if (x + 1 >= map[y].size() || map[y][x + 1] != 'K')
					fillRect({ tileRect.x + 44, tileRect.y + 4, 4, 41 }, 48, 33, 30);
			}
			else if (tile == 'Q')
			{
				fillRect({ tileRect.x + 1, tileRect.y + 3, 46, 42 }, 216, 213, 199);
				fillRect({ tileRect.x + 1, tileRect.y + 3, 46, 6 }, 244, 241, 222);
				for (int course = 0; course < 4; ++course)
				{
					int courseY = tileRect.y + 11 + course * 9;
					fillRect({ tileRect.x + 2, courseY, 44, 2 }, 157, 164, 164);
					int offset = course % 2 == 0 ? 8 : 18;
					for (int seam = offset; seam < 48; seam += 20)
						fillRect({ tileRect.x + seam, courseY - 7, 2, 7 }, 181, 185, 179);
				}
				fillRect({ tileRect.x, tileRect.y + 42, 48, 5 }, 174, 139, 67);
				fillRect({ tileRect.x, tileRect.y + 42, 48, 2 }, 238, 207, 117);
				if (x == 0 || map[y][x - 1] != 'Q')
					fillRect({ tileRect.x, tileRect.y + 4, 4, 41 }, 139, 142, 139);
				if (x + 1 >= map[y].size() || map[y][x + 1] != 'Q')
					fillRect({ tileRect.x + 44, tileRect.y + 4, 4, 41 }, 139, 142, 139);
			}
			else if (tile == 'W')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 124, 72, 38);
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tileRect.x + 3, tileRect.y + 8 + plank * 10, 42, 2 }, 76, 42, 27);
				fillRect({ tileRect.x + 5, tileRect.y + 2, 5, 46 }, 67, 38, 26);
				fillRect({ tileRect.x + 38, tileRect.y + 2, 5, 46 }, 67, 38, 26);
			}
			else if (tile == 'D')
			{
				float open = 0.f;
				bool mercerDoor = false;
				for (size_t portalIndex = 0; portalIndex < mWorldPortals.size(); ++portalIndex)
					if (mWorldPortals[portalIndex].fromMap == currentMapId() &&
						mWorldPortals[portalIndex].fromX == (int)x &&
						mWorldPortals[portalIndex].fromY == (int)y &&
						mWorldPortals[portalIndex].toMap == "mercers_house") mercerDoor = true;
				if (mOpeningPortal >= 0 && mOpeningPortal < (int)mWorldPortals.size())
				{
					const WorldPortal& portal = mWorldPortals[mOpeningPortal];
					if (portal.fromMap == currentMapId() && portal.fromX == (int)x &&
						portal.fromY == (int)y)
						open = std::min(1.f, (SDL_GetTicks() - mPortalAnimationStarted) /
							(float)DOOR_OPEN_DURATION);
				}
				fillRect({ tileRect.x + 7, tileRect.y + 2, 34, 46 },
					cinderrail ? 75 : 104, cinderrail ? 71 : 57, cinderrail ? 68 : 31);
				fillRect({ tileRect.x + 11, tileRect.y + 6, 26, 38 }, 24, 20, 19);
				int doorWidth = std::max(2, (int)std::round(26.f * (1.f - open)));
				fillRect({ tileRect.x + 11, tileRect.y + 6, doorWidth, 38 },
					cinderrail ? 107 : 139, cinderrail ? 111 : 78, cinderrail ? 112 : 39);
				if (cinderrail && doorWidth > 6)
				{
					fillRect({ tileRect.x + 13, tileRect.y + 10, doorWidth - 4, 4 }, 164, 65, 42);
					fillRect({ tileRect.x + 13, tileRect.y + 34, doorWidth - 4, 3 }, 224, 169, 61);
				}
				if (doorWidth >= 8)
					fillRect({ tileRect.x + 11 + doorWidth - 7, tileRect.y + 24, 4, 4 },
						231, 184, 73);
				fillRect({ tileRect.x + 3, tileRect.y + 2, 42, 5 }, 65, 37, 25);
				if (mercerDoor && doorWidth >= 18)
				{
					int signX = tileRect.x + 11 + doorWidth - 14;
					fillRect({ signX, tileRect.y + 8, 13, 13 }, 206, 160, 72);
					drawText("M", signX + 2, tileRect.y + 9, color(72, 43, 28), 9);
				}
			}
			else if (tile == 'F')
			{
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tileRect.x, tileRect.y + plank * 12, 48, 2 }, 96, 58, 36);
				fillRect({ tileRect.x + 23, tileRect.y + 2, 2, 10 }, 109, 67, 39);
				fillRect({ tileRect.x + 10, tileRect.y + 26, 2, 10 }, 109, 67, 39);
			}
			else if (tile == 'C')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 10, 44, 32 }, 112, 62, 34);
				fillRect({ tileRect.x, tileRect.y + 7, 48, 7 }, 166, 105, 53);
				fillRect({ tileRect.x + 8, tileRect.y + 18, 4, 22 }, 71, 40, 27);
				fillRect({ tileRect.x + 36, tileRect.y + 18, 4, 22 }, 71, 40, 27);
			}
			else if (tile == 'B')
			{
				fillRect({ tileRect.x + 5, tileRect.y + 32, 38, 8 }, 74, 70, 65);
				fillRect({ tileRect.x + 10, tileRect.y + 29, 28, 10 }, 111, 100, 83);
				int flicker = (int)((SDL_GetTicks() / 110 + x + y) % 3);
				fillRect({ tileRect.x + 17, tileRect.y + 13 + flicker, 15, 22 - flicker },
					221, 69, 32);
				fillRect({ tileRect.x + 20, tileRect.y + 8 - flicker, 10, 23 }, 249, 137, 36);
				fillRect({ tileRect.x + 23, tileRect.y + 16, 6, 15 }, 255, 222, 89);
			}
			else if (tile == 'A')
			{
				fillRect({ tileRect.x + 6, tileRect.y + 6, 36, 5 }, 93, 52, 29);
				fillRect({ tileRect.x + 6, tileRect.y + 37, 36, 5 }, 93, 52, 29);
				fillRect({ tileRect.x + 4, tileRect.y + 15, 40, 18 }, 139, 82, 39);
				fillRect({ tileRect.x + 8, tileRect.y + 18, 32, 3 }, 181, 116, 54);
				fillRect({ tileRect.x + 12, tileRect.y + 23, 7, 6 }, 232, 208, 151);
				fillRect({ tileRect.x + 29, tileRect.y + 22, 8, 7 }, 175, 49, 37);
			}
			else if (tile == 'S')
			{
				if (cinderrail)
				{
					fillRect({ tileRect.x + 3, tileRect.y + 3, 42, 42 }, 132, 76, 48);
					fillRect({ tileRect.x + 3, tileRect.y + 5, 42, 3 }, 225, 178, 71);
					fillRect({ tileRect.x + 22, tileRect.y + 8, 3, 35 }, 91, 49, 40);
				}
				else
				{
					fillRect({ tileRect.x + 7, tileRect.y + 10, 5, 3 }, 157, 121, 69);
					fillRect({ tileRect.x + 34, tileRect.y + 31, 6, 3 }, 211, 177, 109);
				}
			}
			else if (tile == 'M')
			{
				if (oldRoad)
				{
					fillRect({ tileRect.x + 11, tileRect.y + 5, 27, 39 }, 102, 91, 76);
					fillRect({ tileRect.x + 14, tileRect.y + 8, 21, 31 }, 127, 110, 84);
					fillRect({ tileRect.x + 17, tileRect.y + 13, 15, 4 }, 58, 83, 69);
					fillRect({ tileRect.x + 21, tileRect.y + 20, 7, 12 }, 62, 91, 75);
					fillRect({ tileRect.x + 7, tileRect.y + 42, 35, 4 }, 53, 48, 45);
				}
				else
				{
					fillRect({ tileRect.x + 2, tileRect.y + 2, 44, 44 }, 220, 224, 224);
					fillRect({ tileRect.x + 3, tileRect.y + 22, 42, 2 }, 162, 173, 179);
					fillRect({ tileRect.x + 17, tileRect.y + 3, 2, 19 }, 177, 186, 190);
					fillRect({ tileRect.x + 31, tileRect.y + 24, 2, 21 }, 177, 186, 190);
				}
			}
			else if (tile == 'E')
			{
				fillRect({ tileRect.x + 3, tileRect.y + 24, 42, 17 }, 104, 58, 32);
				fillRect({ tileRect.x + 5, tileRect.y + 20, 38, 6 }, 171, 108, 51);
				fillRect({ tileRect.x + 9, tileRect.y + 10, 7, 11 }, 54, 151, 193);
				fillRect({ tileRect.x + 20, tileRect.y + 7, 6, 14 }, 151, 71, 183);
				fillRect({ tileRect.x + 31, tileRect.y + 12, 8, 9 }, 217, 158, 48);
			}
			else if (tile == 'R' || tile == 'X')
			{
				for (int tie = 2; tie < 48; tie += 10)
					fillRect({ tileRect.x + tie, tileRect.y + 5, 5, 38 },
						tile == 'X' ? 155 : 100, tile == 'X' ? 112 : 73,
						tile == 'X' ? 55 : 48);
				fillRect({ tileRect.x, tileRect.y + 9, 48, 5 }, 151, 158, 158);
				fillRect({ tileRect.x, tileRect.y + 34, 48, 5 }, 151, 158, 158);
				fillRect({ tileRect.x, tileRect.y + 11, 48, 2 }, 221, 225, 220);
				fillRect({ tileRect.x, tileRect.y + 36, 48, 2 }, 221, 225, 220);
				if (tile == 'X')
					for (int plate = 3; plate < 48; plate += 9)
						fillRect({ tileRect.x + plate, tileRect.y + 16, 6, 15 }, 186, 151, 72);
			}
			else if (tile == 'G')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 2, 44, 44 }, 88, 96, 101);
				for (int grate = 7; grate < 44; grate += 9)
					fillRect({ tileRect.x + grate, tileRect.y + 4, 3, 40 }, 45, 50, 54);
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 3 }, 222, 168, 57);
				fillRect({ tileRect.x + 5, tileRect.y + 8, 4, 4 }, 196, 204, 202);
				fillRect({ tileRect.x + 39, tileRect.y + 36, 4, 4 }, 196, 204, 202);
			}
			else if (tile == 'J')
			{
				fillRect({ tileRect.x + 1, tileRect.y + 3, 46, 42 }, 75, 76, 77);
				fillRect({ tileRect.x, tileRect.y + 40, 48, 7 }, 43, 45, 47);
				for (int panel = 0; panel < 48; panel += 16)
				{
					fillRect({ tileRect.x + panel, tileRect.y + 3, 3, 38 }, 43, 47, 49);
					fillRect({ tileRect.x + panel + 3, tileRect.y + 7, 12, 4 }, 142, 77, 51);
					fillRect({ tileRect.x + panel + 6, tileRect.y + 11, 9, 4 }, 125, 72, 51);
					fillRect({ tileRect.x + panel + 9, tileRect.y + 15, 6, 4 }, 108, 67, 52);
				}
				fillRect({ tileRect.x + 4, tileRect.y + 24, 40, 7 }, 155, 170, 171);
				fillRect({ tileRect.x + 7, tileRect.y + 26, 34, 3 }, 104, 189, 202);
				if (x == 0 || map[y][x - 1] != 'J')
					fillRect({ tileRect.x, tileRect.y + 2, 4, 43 }, 38, 42, 44);
				if (x + 1 >= map[y].size() || map[y][x + 1] != 'J')
					fillRect({ tileRect.x + 44, tileRect.y + 2, 4, 43 }, 38, 42, 44);
			}
			else if (tile == 'I')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 135, 55, 40);
				for (int brick = 10; brick < 43; brick += 11)
					fillRect({ tileRect.x + 3, tileRect.y + brick, 42, 2 }, 73, 39, 37);
				fillRect({ tileRect.x + 23, tileRect.y + 4, 2, 8 }, 75, 40, 37);
				fillRect({ tileRect.x + 12, tileRect.y + 14, 2, 7 }, 75, 40, 37);
				fillRect({ tileRect.x + 34, tileRect.y + 25, 2, 7 }, 75, 40, 37);
				if ((x + y) % 3 == 0)
					fillRect({ tileRect.x + 8, tileRect.y + 18, 13, 11 }, 225, 173, 69);
			}
			else if (tile == 'P')
			{
				fillRect({ tileRect.x + 5, tileRect.y + 6, 38, 37 }, 82, 88, 89);
				fillRect({ tileRect.x + 8, tileRect.y + 10, 32, 6 }, 176, 75, 43);
				fillRect({ tileRect.x + 21, tileRect.y + 16, 8, 23 }, 169, 178, 175);
				fillRect({ tileRect.x + 14, tileRect.y + 23, 22, 8 }, 169, 178, 175);
				int pulse = (SDL_GetTicks() / 240 + x + y) % 2 == 0 ? 0 : 3;
				fillRect({ tileRect.x + 12 + pulse, tileRect.y + 34, 7, 5 }, 235, 174, 54);
				fillRect({ tileRect.x + 32 - pulse, tileRect.y + 20, 5, 5 }, 97, 213, 182);
			}
			else if (tile == 'V')
			{
				fillRect({ tileRect.x + 5, tileRect.y + 3, 38, 43 }, 69, 55, 52);
				fillRect({ tileRect.x + 9, tileRect.y, 30, 8 }, 105, 89, 81);
				fillRect({ tileRect.x + 11, tileRect.y + 20, 26, 22 }, 34, 29, 29);
				int flicker = (int)((SDL_GetTicks() / 105 + x * 3 + y) % 4);
				fillRect({ tileRect.x + 14, tileRect.y + 29 - flicker, 20, 11 + flicker },
					235, 71, 29);
				fillRect({ tileRect.x + 19, tileRect.y + 24 + flicker, 10, 15 - flicker },
					255, 177, 48);
				fillRect({ tileRect.x + 8, tileRect.y + 12, 32, 4 }, 203, 175, 106);
			}
			else if (tile == '.')
			{
				if (cinderrail)
				{
					fillRect({ tileRect.x + 7, tileRect.y + 35, 5, 3 }, 117, 94, 61);
					fillRect({ tileRect.x + 32, tileRect.y + 13, 3, 3 }, 58, 57, 52);
				}
				else fillRect({ tileRect.x + 7, tileRect.y + 34, 3, 8 }, 111, 180, 64);
			}
			if (oldRoad && tile == '=')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 5, 18, 15 }, 143, 130, 101);
				fillRect({ tileRect.x + 24, tileRect.y + 7, 21, 13 }, 105, 98, 82);
				fillRect({ tileRect.x + 6, tileRect.y + 25, 25, 14 }, 104, 98, 82);
				fillRect({ tileRect.x + 34, tileRect.y + 26, 12, 12 }, 148, 131, 95);
				fillRect({ tileRect.x + 19, tileRect.y + 8, 3, 8 }, 62, 82, 60);
				fillRect({ tileRect.x + 31, tileRect.y + 34, 5, 3 }, 67, 91, 62);
			}
			else if (cinderrail && tile == '=')
			{
				SDL_Color route = color(190, 145, 55);
				if ((y == 25 || y == 26) && x <= 25) route = color(88, 170, 93);
				else if (y >= 32 && x >= 15 && x <= 33) route = color(141, 91, 177);
				else if (x >= 45 && y >= 15 && y <= 21) route = color(222, 225, 216);
				bool verticalRoute = (x == 25 || x == 26) && y != 10 && y != 11 &&
					y != 16 && y != 17 && y != 25 && y != 26 && y != 34;
				if (verticalRoute)
					fillRect({ tileRect.x + 4, tileRect.y, 3, 48 }, route.r, route.g, route.b);
				else fillRect({ tileRect.x, tileRect.y + 4, 48, 3 }, route.r, route.g, route.b);
				fillRect({ tileRect.x + 8, tileRect.y + 26, 5, 5 }, 74, 66, 57);
				fillRect({ tileRect.x + 36, tileRect.y + 14, 4, 4 }, 205, 174, 104);
			}
		}
	}
	if (cinderrail)
	{
		int stationX = mapX + 8 * TILE;
		int stationY = mapY + TILE;
		fillRect({ stationX + 8, stationY, 80, 102 }, 116, 50, 39);
		fillRect({ stationX + 3, stationY, 90, 9 }, 55, 54, 56);
		fillRect({ stationX + 28, stationY + 17, 39, 39 }, 44, 42, 43);
		fillRect({ stationX + 33, stationY + 22, 29, 29 }, 223, 207, 159);
		fillRect({ stationX + 46, stationY + 25, 3, 13 }, 61, 56, 52);
		fillRect({ stationX + 47, stationY + 36, 10, 3 }, 61, 56, 52);
		drawText("CENTRAL STATION", mapX + 4 * TILE, mapY + 4 * TILE + 14,
			color(250, 211, 104), 12, 9 * TILE);
		drawText("FORGE SQUARE", mapX + 21 * TILE, mapY + 13 * TILE + 12,
			color(244, 207, 92), 12, 9 * TILE);
		drawText("FOUNDRY HALL", mapX + 42 * TILE, mapY + 13 * TILE + 12,
			color(248, 205, 102), 12, 8 * TILE);
		drawText("FORGE ARENA", mapX + 42 * TILE, mapY + 23 * TILE + 12,
			color(255, 219, 137), 12, 8 * TILE);
	}

	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (!npcVisible((int)i) || mNpcs[i].mapId != currentMapId()) continue;
		drawCharacter(mNpcs[i].visualX, mNpcs[i].visualY, mNpcs[i].appearance,
			mNpcs[i].isComplete(), mNpcs[i].isMoving());
		if (npcHasStoryMarker((int)i))
		{
			int markerX = mapX + (int)std::round(mNpcs[i].visualX * TILE) + 17;
			int markerY = mapY + (int)std::round(mNpcs[i].visualY * TILE) - 20;
			fillRect({ markerX - 4, markerY - 2, 20, 22 }, 31, 24, 14, 235);
			outlineRect({ markerX - 4, markerY - 2, 20, 22 }, 246, 203, 78, 255, 2);
			drawText("!", markerX + 2, markerY, color(255, 225, 111), 16);
		}
	}
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		if (shard.mapId != currentMapId() || mCollectedShards.count(shard.id)) continue;
		int x = mapX + shard.x * TILE;
		int y = mapY + shard.y * TILE;
		int shimmer = (int)((SDL_GetTicks() / 180 + i * 3) % 5);
		fillRect({ x + 19, y + 10 - shimmer / 2, 12, 28 }, 42, 24, 65, 220);
		fillRect({ x + 14, y + 15 - shimmer / 2, 22, 18 }, 139, 82, 204, 245);
		fillRect({ x + 19, y + 19 - shimmer / 2, 12, 10 }, 225, 184, 255, 255);
	}
	bool playerWalking = std::fabs(mPlayerX - mVisualX) > 0.001f ||
		std::fabs(mPlayerY - mVisualY) > 0.001f;
	drawCharacter(mVisualX, mVisualY, CharacterAppearance::Player, false, playerWalking);
	SDL_RenderSetClipRect(mRenderer, NULL);

	fillRect({ 1012, 28, 238, 670 }, 21, 28, 45, 245);
	outlineRect({ 1012, 28, 238, 670 }, 190, 146, 61, 255, 2);
	drawText(mWorldAreas[mCurrentWorldArea].name, 1034, 48, color(242, 205, 99), 24, 205);
	drawText("GOLD " + std::to_string(mMoney), 1034, 91, color(245, 205, 88), 16);
	int heldShards = (int)mCollectedShards.size() - (int)mMercerShards.size();
	drawText("SHARDS " + std::to_string(std::max(0, heldShards)) +
		"  •  MERCER " + std::to_string(mMercerShards.size()),
		1034, 113, color(190, 150, 225), 12);
	drawText("DUELISTS", 1034, 134, color(135, 162, 199), 16);
	int duelistRow = 0;
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (mNpcs[i].kind != NpcKind::Duelist) continue;
		const int rowY = 159 + duelistRow++ * 44;
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
		std::string dialogue = mDialogueText.substr(0, mDialogueVisibleBytes);
		bool fullyRevealed = mDialogueVisibleBytes >= mDialogueText.size();
		std::string prompt = "E / Click: continue";
		if (mDialogueAction == DialogueAction::NpcInteraction && npc.isShopkeeper())
			prompt = "E / Click: browse";
		else if (mDialogueAction == DialogueAction::NpcInteraction && npc.isComplete())
		{
			prompt = "E / Click: close";
		}
		else if (mDialogueAction == DialogueAction::NpcInteraction && npc.isDuelist())
			prompt = npc.rankName() + "  •  E / Click: battle";
		drawText(dialogue, 68, 704, color(232, 237, 246), 19, 1080);
		if (fullyRevealed)
		{
			drawText(prompt, 850, 668, color(126, 176, 242), 15, 340);
			int arrowOffset = (SDL_GetTicks() / 220) % 2 == 0 ? 0 : 3;
			drawText("▼", 1191, 739 + arrowOffset, color(244, 206, 103), 17);
		}
	}
	if (mDialogueNpc < 0) renderStoryScene();
	if (mPauseMenuOpen) renderPauseMenu();
}

void Application::drawCharacter(float gridX, float gridY, CharacterAppearance appearance,
	bool completed, bool walking)
{
	const std::vector<std::string>& map = currentMap();
	float cameraX = mScreen == Screen::WorldBuilder ? (float)mWorldBuilderCameraX :
		overworldCameraX();
	float cameraY = mScreen == Screen::WorldBuilder ? (float)mWorldBuilderCameraY :
		overworldCameraY();
	int x = mapOriginX((int)map[0].size()) +
		(int)std::round((gridX - cameraX) * TILE);
	int y = mapOriginY((int)map.size()) +
		(int)std::round((gridY - cameraY) * TILE);
	int stride = walking && (SDL_GetTicks() / 110) % 2 == 0 ? 2 : (walking ? -2 : 0);
	int bob = walking && (SDL_GetTicks() / 110) % 2 == 0 ? -1 : 0;
	fillRect({ x + 9, y + 39, 31, 6 }, 8, 14, 18, 100);
	y += bob;

	const int appearanceValue = static_cast<int>(appearance);
	const int genericFirst = static_cast<int>(CharacterAppearance::Generic1);
	const int genericVariant = appearanceValue >= genericFirst &&
		appearanceValue <= static_cast<int>(CharacterAppearance::Generic10) ?
		appearanceValue - genericFirst : -1;
	SDL_Color trousers = color(31, 38, 53);
	SDL_Color shoes = color(17, 22, 31);
	if (genericVariant >= 0)
	{
		const SDL_Color genericTrousers[10] = {
			color(35, 48, 77), color(48, 67, 43), color(88, 58, 34), color(48, 37, 70),
			color(30, 70, 82), color(91, 74, 42), color(65, 37, 35), color(38, 42, 49),
			color(75, 43, 66), color(38, 55, 84)
		};
		trousers = genericTrousers[genericVariant];
	}
	else if (appearance == CharacterAppearance::Rook || appearance == CharacterAppearance::Briar)
		trousers = color(60, 56, 37);
	else if (appearance == CharacterAppearance::Aurelia)
		trousers = color(184, 167, 112);
	else if (appearance == CharacterAppearance::Mercer)
		trousers = color(75, 48, 30);
	else if (appearance == CharacterAppearance::VeiledOne || appearance == CharacterAppearance::Nyx)
		trousers = color(25, 20, 36);
	fillRect({ x + 16 + stride, y + 34, 7, 9 }, trousers.r, trousers.g, trousers.b);
	fillRect({ x + 27 - stride, y + 34, 7, 9 }, trousers.r, trousers.g, trousers.b);
	fillRect({ x + 15 + stride, y + 40, 8, 4 }, shoes.r, shoes.g, shoes.b);
	fillRect({ x + 27 - stride, y + 40, 8, 4 }, shoes.r, shoes.g, shoes.b);

	auto block = [this, x, y](int offsetX, int offsetY, int width, int height, SDL_Color shade)
	{
		fillRect({ x + offsetX, y + offsetY, width, height },
			shade.r, shade.g, shade.b, shade.a);
	};

	const SDL_Color outline = color(24, 25, 35);
	const SDL_Color skin = color(224, 172, 126);
	const SDL_Color skinShadow = color(183, 124, 91);

	switch (appearance)
	{
	case CharacterAppearance::Player:
		block(12, 18, 25, 21, outline);
		block(14, 20, 21, 17, color(31, 88, 185));
		block(14, 30, 21, 4, color(22, 54, 111));
		block(18, 7, 15, 15, skin);
		block(15, 4, 21, 8, color(91, 48, 22));
		block(15, 10, 4, 9, color(91, 48, 22));
		block(29, 13, 2, 2, color(38, 31, 27));
		break;

	case CharacterAppearance::Mira:
		block(10, 18, 30, 22, outline);
		block(12, 20, 26, 20, color(91, 38, 119));
		block(9, 19, 32, 6, color(53, 27, 75));
		block(16, 7, 18, 16, color(202, 150, 137));
		block(13, 4, 24, 8, color(57, 23, 68));
		block(13, 9, 5, 16, color(57, 23, 68));
		block(33, 9, 5, 16, color(57, 23, 68));
		block(24, 27, 4, 5, color(190, 85, 219));
		block(29, 13, 2, 2, color(57, 28, 52));
		break;

	case CharacterAppearance::Marin:
		block(12, 18, 25, 21, outline);
		block(14, 20, 21, 18, color(31, 112, 174));
		block(14, 20, 21, 5, color(113, 211, 218));
		block(18, 7, 15, 15, skin);
		block(15, 4, 21, 7, color(28, 94, 135));
		block(15, 10, 4, 10, color(28, 94, 135));
		block(34, 8, 5, 13, color(36, 151, 181));
		block(38, 16, 4, 8, color(36, 151, 181));
		block(17, 9, 18, 3, color(210, 235, 224));
		block(29, 13, 2, 2, color(24, 50, 62));
		break;

	case CharacterAppearance::Rook:
		block(8, 18, 33, 22, outline);
		block(11, 20, 27, 19, color(72, 122, 53));
		block(8, 20, 8, 8, color(113, 82, 44));
		block(35, 20, 7, 8, color(113, 82, 44));
		block(18, 7, 15, 15, skin);
		block(15, 4, 21, 7, color(66, 45, 28));
		block(12, 2, 4, 7, color(151, 116, 62));
		block(35, 1, 4, 8, color(151, 116, 62));
		block(15, 30, 22, 4, color(47, 75, 38));
		block(29, 13, 2, 2, color(45, 34, 26));
		break;

	case CharacterAppearance::Aurelia:
		block(12, 18, 26, 22, outline);
		block(14, 20, 22, 20, color(225, 218, 185));
		block(11, 20, 28, 6, color(210, 166, 55));
		block(19, 7, 14, 15, skin);
		block(15, 3, 21, 8, color(227, 196, 93));
		block(15, 9, 5, 17, color(227, 196, 93));
		block(33, 9, 5, 17, color(227, 196, 93));
		block(19, 4, 15, 2, color(250, 238, 168));
		block(24, 27, 4, 8, color(237, 194, 68));
		block(29, 13, 2, 2, color(71, 58, 37));
		break;

	case CharacterAppearance::Flint:
		block(11, 18, 28, 21, outline);
		block(13, 20, 24, 18, color(177, 55, 37));
		block(10, 20, 8, 7, color(226, 102, 40));
		block(34, 20, 7, 7, color(226, 102, 40));
		block(18, 8, 15, 14, skin);
		block(14, 4, 22, 7, color(190, 55, 28));
		block(16, 1, 5, 6, color(232, 91, 30));
		block(23, 2, 5, 5, color(232, 91, 30));
		block(31, 0, 5, 8, color(232, 91, 30));
		block(16, 10, 20, 3, color(71, 43, 36));
		block(17, 10, 7, 3, color(238, 170, 53));
		block(28, 10, 7, 3, color(238, 170, 53));
		block(14, 30, 23, 4, color(89, 33, 30));
		break;

	case CharacterAppearance::Nyx:
		block(9, 16, 32, 25, outline);
		block(11, 18, 28, 22, color(45, 28, 64));
		block(12, 5, 26, 19, color(24, 20, 39));
		block(16, 8, 18, 13, color(73, 50, 81));
		block(17, 5, 16, 5, color(55, 31, 76));
		block(20, 13, 4, 2, color(190, 77, 221));
		block(29, 13, 4, 2, color(190, 77, 221));
		block(9, 35, 7, 7, color(45, 28, 64));
		block(33, 35, 8, 7, color(45, 28, 64));
		block(24, 26, 4, 5, color(128, 55, 157));
		break;

	case CharacterAppearance::Tidal:
		block(10, 18, 30, 22, outline);
		block(12, 20, 26, 20, color(23, 71, 116));
		block(9, 20, 8, 7, color(40, 142, 154));
		block(35, 20, 7, 7, color(40, 142, 154));
		block(18, 7, 15, 15, skinShadow);
		block(14, 4, 23, 7, color(28, 113, 128));
		block(14, 9, 5, 11, color(28, 113, 128));
		block(17, 7, 19, 3, color(224, 220, 177));
		block(34, 2, 5, 8, color(50, 169, 176));
		block(29, 13, 2, 2, color(24, 40, 44));
		block(13, 30, 25, 4, color(35, 154, 161));
		break;

	case CharacterAppearance::Briar:
		block(10, 18, 29, 22, outline);
		block(12, 20, 25, 20, color(70, 118, 51));
		block(11, 18, 28, 6, color(115, 78, 41));
		block(18, 8, 15, 14, skin);
		block(14, 5, 23, 8, color(43, 87, 43));
		block(14, 11, 5, 12, color(43, 87, 43));
		block(33, 10, 5, 14, color(43, 87, 43));
		block(11, 2, 7, 5, color(70, 137, 51));
		block(35, 3, 7, 5, color(70, 137, 51));
		block(29, 13, 2, 2, color(44, 40, 25));
		block(23, 28, 5, 6, color(157, 110, 48));
		break;

	case CharacterAppearance::Generic1:
	case CharacterAppearance::Generic2:
	case CharacterAppearance::Generic3:
	case CharacterAppearance::Generic4:
	case CharacterAppearance::Generic5:
	case CharacterAppearance::Generic6:
	case CharacterAppearance::Generic7:
	case CharacterAppearance::Generic8:
	case CharacterAppearance::Generic9:
	case CharacterAppearance::Generic10:
	{
		const SDL_Color outfits[10] = {
			color(42, 86, 166), color(59, 119, 70), color(190, 104, 43), color(109, 63, 145),
			color(34, 128, 145), color(226, 205, 112), color(174, 55, 45), color(65, 72, 83),
			color(187, 69, 132), color(63, 103, 176)
		};
		const SDL_Color accents[10] = {
			color(225, 68, 54), color(151, 199, 92), color(246, 178, 64), color(208, 139, 231),
			color(93, 207, 204), color(247, 238, 185), color(247, 113, 61), color(151, 165, 180),
			color(244, 148, 190), color(157, 201, 244)
		};
		const SDL_Color hairColors[10] = {
			color(73, 44, 28), color(47, 67, 38), color(189, 83, 31), color(67, 40, 78),
			color(32, 78, 101), color(226, 186, 81), color(40, 30, 28), color(47, 48, 54),
			color(129, 58, 93), color(213, 220, 229)
		};
		const SDL_Color skinTones[10] = {
			skin, skin, color(220, 159, 103), color(238, 188, 150), skinShadow,
			color(235, 188, 142), color(139, 88, 65), color(201, 148, 111),
			color(240, 190, 153), color(217, 166, 130)
		};
		const SDL_Color outfit = outfits[genericVariant];
		const SDL_Color accent = accents[genericVariant];
		const SDL_Color hair = hairColors[genericVariant];
		const SDL_Color genericSkin = skinTones[genericVariant];
		const bool broad = genericVariant == 2 || genericVariant == 6 || genericVariant == 7;
		block(broad ? 9 : 11, 18, broad ? 32 : 28, 22, outline);
		block(broad ? 11 : 13, 20, broad ? 28 : 24, 19, outfit);
		block(broad ? 8 : 10, 21, 7, 8, broad && genericVariant == 6 ? genericSkin : outfit);
		block(broad ? 36 : 35, 21, 7, 8, broad && genericVariant == 6 ? genericSkin : outfit);
		block(14, 29, 23, 4, accent);
		block(18, 7, 15, 15, genericSkin);
		block(29, 13, 2, 2, color(43, 35, 31));

		switch (genericVariant)
		{
		case 0: // cap and short jacket
			block(15, 4, 21, 7, hair);
			block(13, 2, 24, 6, accent);
			block(31, 7, 10, 3, accent);
			block(20, 22, 4, 15, color(224, 224, 215));
			break;
		case 1: // hood and long side locks
			block(13, 3, 24, 8, hair);
			block(13, 9, 5, 17, hair);
			block(33, 9, 5, 17, hair);
			block(15, 2, 20, 4, accent);
			block(17, 20, 17, 5, color(38, 77, 49));
			break;
		case 2: // spiked hair and utility vest
			block(14, 5, 23, 7, hair);
			block(14, 1, 5, 7, hair);
			block(22, 0, 5, 8, hair);
			block(31, 2, 6, 7, hair);
			block(12, 20, 5, 17, accent);
			block(34, 20, 5, 17, accent);
			break;
		case 3: // bob cut and square glasses
			block(14, 3, 23, 9, hair);
			block(14, 9, 5, 15, hair);
			block(33, 9, 5, 15, hair);
			block(17, 11, 8, 5, color(32, 34, 45));
			block(28, 11, 8, 5, color(32, 34, 45));
			block(24, 12, 5, 2, color(32, 34, 45));
			break;
		case 4: // bandanna with trailing knot
			block(14, 5, 23, 7, hair);
			block(13, 4, 25, 4, accent);
			block(36, 6, 7, 5, accent);
			block(39, 10, 5, 8, accent);
			block(20, 20, 12, 4, color(221, 231, 217));
			break;
		case 5: // high ponytail and bright collar
			block(14, 4, 23, 8, hair);
			block(34, 1, 7, 8, hair);
			block(39, 5, 5, 13, hair);
			block(12, 20, 27, 5, accent);
			block(23, 25, 6, 9, color(198, 156, 55));
			break;
		case 6: // athletic headband and wrist guards
			block(14, 4, 23, 8, hair);
			block(13, 7, 25, 4, accent);
			block(8, 26, 8, 4, color(83, 31, 30));
			block(36, 26, 8, 4, color(83, 31, 30));
			block(17, 20, 17, 5, color(112, 35, 33));
			break;
		case 7: // beanie, scarf, and long coat
			block(14, 5, 23, 7, hair);
			block(13, 1, 25, 8, accent);
			block(16, 0, 19, 3, accent);
			block(11, 20, 29, 7, color(48, 54, 65));
			block(14, 25, 24, 5, accent);
			break;
		case 8: // twin tails
			block(14, 4, 23, 8, hair);
			block(9, 6, 8, 14, hair);
			block(35, 6, 8, 14, hair);
			block(8, 5, 7, 5, accent);
			block(38, 5, 7, 5, accent);
			block(21, 21, 10, 5, color(238, 202, 219));
			break;
		case 9: // swept pale hair and shoulder satchel
			block(13, 4, 24, 8, hair);
			block(12, 2, 9, 6, hair);
			block(31, 1, 8, 7, hair);
			block(34, 9, 5, 11, hair);
			block(15, 20, 5, 18, accent);
			block(35, 27, 8, 11, color(120, 76, 42));
			break;
		}
		break;
	}

	case CharacterAppearance::Mercer:
		block(10, 18, 30, 22, outline);
		block(12, 20, 26, 19, color(211, 174, 105));
		block(17, 22, 18, 18, color(145, 87, 40));
		block(18, 8, 15, 14, skin);
		block(14, 4, 23, 7, color(117, 70, 35));
		block(11, 9, 30, 4, color(151, 96, 43));
		block(18, 7, 15, 3, color(151, 96, 43));
		block(21, 16, 10, 3, color(104, 57, 31));
		block(35, 25, 8, 11, color(193, 134, 45));
		block(36, 27, 6, 7, color(108, 67, 34));
		break;

	case CharacterAppearance::VeiledOne:
		block(7, 14, 36, 28, outline);
		block(9, 16, 32, 26, color(42, 29, 56));
		block(10, 3, 30, 23, color(20, 18, 30));
		block(14, 7, 22, 15, color(65, 49, 77));
		block(7, 7, 8, 11, color(20, 18, 30));
		block(38, 6, 7, 12, color(20, 18, 30));
		block(13, 1, 6, 7, color(72, 45, 91));
		block(32, 0, 6, 8, color(72, 45, 91));
		block(17, 14, 19, 7, color(27, 21, 36));
		block(19, 12, 5, 2, color(222, 79, 226));
		block(30, 12, 5, 2, color(222, 79, 226));
		block(9, 35, 8, 8, color(68, 39, 83));
		block(33, 35, 8, 8, color(68, 39, 83));
		block(24, 27, 5, 7, color(169, 70, 188));
		break;
	}

	if (completed)
	{
		block(34, 17, 8, 10, color(33, 39, 48));
		block(36, 19, 4, 4, color(220, 193, 92));
		block(37, 23, 2, 3, color(151, 124, 55));
	}
}
