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
	return tile == '.' || tile == '=' || tile == 'F' || tile == 'D' || tile == 'S';
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
			if (tile == '=') fillRect(tileRect, 162, 132, 76);
			else if (tile == '~') fillRect(tileRect, 25, 111, 157);
			else if (tile == 'H') fillRect(tileRect, 126, 65, 43);
			else if (tile == '#' || tile == 'T') fillRect(tileRect, 26, 75, 33);
			else if (tile == 'S') fillRect(tileRect, 188, 151, 87);
			else if (tile == 'M') fillRect(tileRect, 198, 204, 207);
			else if (tile == 'B') fillRect(tileRect, 83, 64, 42);
			else if (tile == 'A') fillRect(tileRect, 61, 139, 61);
			else if (tile == 'E') fillRect(tileRect, 137, 91, 49);
			else if (tile == 'W' || tile == 'D' || tile == 'F' || tile == 'C')
				fillRect(tileRect, tile == 'F' ? 137 : 91, tile == 'F' ? 91 : 53, tile == 'F' ? 49 : 31);
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
			else if (tile == 'W')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 124, 72, 38);
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tileRect.x + 3, tileRect.y + 8 + plank * 10, 42, 2 }, 76, 42, 27);
				fillRect({ tileRect.x + 5, tileRect.y + 2, 5, 46 }, 67, 38, 26);
				fillRect({ tileRect.x + 38, tileRect.y + 2, 5, 46 }, 67, 38, 26);
				if (!mWorldAreas[mCurrentWorldArea].indoor &&
					(y == 0 || (map[y - 1][x] != 'W' && map[y - 1][x] != 'D')))
				{
					fillRect({ tileRect.x, tileRect.y, 48, 16 }, 73, 42, 31);
					fillRect({ tileRect.x + 3, tileRect.y + 3, 42, 5 }, 151, 83, 43);
					fillRect({ tileRect.x + 8, tileRect.y + 10, 35, 3 }, 109, 56, 35);
				}
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
				fillRect({ tileRect.x + 7, tileRect.y + 2, 34, 46 }, 104, 57, 31);
				fillRect({ tileRect.x + 11, tileRect.y + 6, 26, 38 }, 24, 20, 19);
				int doorWidth = std::max(2, (int)std::round(26.f * (1.f - open)));
				fillRect({ tileRect.x + 11, tileRect.y + 6, doorWidth, 38 }, 139, 78, 39);
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
				fillRect({ tileRect.x + 7, tileRect.y + 10, 5, 3 }, 157, 121, 69);
				fillRect({ tileRect.x + 34, tileRect.y + 31, 6, 3 }, 211, 177, 109);
			}
			else if (tile == 'M')
			{
				fillRect({ tileRect.x + 2, tileRect.y + 2, 44, 44 }, 220, 224, 224);
				fillRect({ tileRect.x + 3, tileRect.y + 22, 42, 2 }, 162, 173, 179);
				fillRect({ tileRect.x + 17, tileRect.y + 3, 2, 19 }, 177, 186, 190);
				fillRect({ tileRect.x + 31, tileRect.y + 24, 2, 21 }, 177, 186, 190);
			}
			else if (tile == 'E')
			{
				fillRect({ tileRect.x + 3, tileRect.y + 24, 42, 17 }, 104, 58, 32);
				fillRect({ tileRect.x + 5, tileRect.y + 20, 38, 6 }, 171, 108, 51);
				fillRect({ tileRect.x + 9, tileRect.y + 10, 7, 11 }, 54, 151, 193);
				fillRect({ tileRect.x + 20, tileRect.y + 7, 6, 14 }, 151, 71, 183);
				fillRect({ tileRect.x + 31, tileRect.y + 12, 8, 9 }, 217, 158, 48);
			}
			else if (tile == '.')
				fillRect({ tileRect.x + 7, tileRect.y + 34, 3, 8 }, 111, 180, 64);
		}
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

	SDL_Color trousers = color(31, 38, 53);
	SDL_Color shoes = color(17, 22, 31);
	if (appearance == CharacterAppearance::Rook || appearance == CharacterAppearance::Briar)
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
