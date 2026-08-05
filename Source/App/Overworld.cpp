#include "Application.h"

#include "AppSupport.h"
#include "Landmarks.h"
#include "SpriteSheetRenderer.h"
#include "WorldTileRenderer.h"

#include <algorithm>
#include <cmath>

using namespace AppSupport;

namespace
{
	constexpr Uint32 DOOR_OPEN_DURATION = 400;
	constexpr Uint32 DIALOGUE_CHARACTER_DELAY = 18;
	constexpr Uint32 REGION_BANNER_DURATION = 3400;
	constexpr Uint32 REGION_BANNER_SLIDE = 260;

	CharacterSpriteDefinition characterSprite(CharacterAppearance appearance)
	{
		const std::string root = "Resources/Graphics/Characters/";
		const int value = static_cast<int>(appearance);
		const int maleFirst = static_cast<int>(CharacterAppearance::GenericMale1);
		const int femaleFirst = static_cast<int>(CharacterAppearance::GenericFemale1);
		if (value >= maleFirst && value <= static_cast<int>(CharacterAppearance::GenericMale10))
		{
			int variant = value - maleFirst;
			return { root + (variant < 8 ? "People3.png" : "People5.png"), variant % 8 };
		}
		if (value >= femaleFirst && value <= static_cast<int>(CharacterAppearance::GenericFemale10))
		{
			int variant = value - femaleFirst;
			return { root + (variant < 8 ? "People4.png" : "People6.png"), variant % 8 };
		}

		switch (appearance)
		{
		case CharacterAppearance::Player: return { root + "Actor1.png", 0 };
		case CharacterAppearance::Mira: return { root + "Actor1.png", 5 };
		case CharacterAppearance::Marin: return { root + "Actor1.png", 3 };
		case CharacterAppearance::Rook: return { root + "Actor1.png", 6 };
		case CharacterAppearance::Aurelia: return { root + "Actor1.png", 1 };
		case CharacterAppearance::Flint: return { root + "Actor3.png", 0 };
		case CharacterAppearance::Nyx: return { root + "Actor2.png", 7 };
		case CharacterAppearance::Tidal: return { root + "Actor2.png", 2 };
		case CharacterAppearance::Briar: return { root + "Actor5.png", 0 };
		case CharacterAppearance::Mercer: return { root + "People1.png", 4 };
		case CharacterAppearance::VeiledOne: return { root + "Evil.png", 3 };
		case CharacterAppearance::Neris: return { root + "Actor2.png", 4 };
		case CharacterAppearance::Oren: return { root + "Actor5.png", 1 };
		default: return { "", 0 };
		}
	}
}

void Application::handleOverworldEvent(const SDL_Event& event)
{
	if (mDialogueNpc >= 0 || mDialogueObject >= 0)
	{
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
			advanceDialogue();
		else if (event.type == SDL_KEYDOWN && !event.key.repeat)
		{
			SDL_Keycode key = event.key.keysym.sym;
			if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN)
				advanceDialogue();
			else if (key == SDLK_ESCAPE)
			{
				if (mDialogueObject >= 0)
				{
					clearDialogue();
					return;
				}
				int npcIndex = mDialogueNpc;
				DialogueAction action = mDialogueAction;
				if (action == DialogueAction::NpcInteraction) clearDialogue();
				else if (action == DialogueAction::OpenNpcMenu ||
					action == DialogueAction::ReturnToNpcMenu)
				{
					clearDialogue();
					mNpcMenuNpc = npcIndex;
					mNpcMenuSelection = 0;
				}
			}
		}
		return;
	}
	if (handleStoryEvent(event)) return;
	if (mPauseMenuOpen)
	{
		handlePauseMenuEvent(event);
		return;
	}
	if (mNpcMenuNpc >= 0)
	{
		handleNpcMenuEvent(event);
		return;
	}
	if (mRouteChallengeNpc >= 0) return;
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
	updateRegionBanner();
	if (mPauseMenuOpen || (mStoryScene != StoryScene::None && mDialogueNpc < 0 &&
		mDialogueObject < 0)) return;
	float playerDx = mPlayerX - mVisualX;
	float playerDy = mPlayerY - mVisualY;
	float playerDistance = std::sqrt(playerDx * playerDx + playerDy * playerDy);
	if (playerDistance <= 0.001f && mOpeningPortal < 0 && mDialogueNpc < 0 &&
		mDialogueObject < 0 && mNpcMenuNpc < 0 && mRouteChallengeNpc < 0 &&
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
	updateRegionBanner();

	Uint32 now = SDL_GetTicks();
	if (mOpeningPortal >= 0)
	{
		if (playerDistance <= 0.001f && now - mPortalAnimationStarted >= DOOR_OPEN_DURATION)
			activatePortalAt(mPlayerX, mPlayerY);
		updateRegionBanner();
		return;
	}
	const int directionX[] = { 0, 1, 0, -1 };
	const int directionY[] = { -1, 0, 1, 0 };
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (!npcVisible((int)i) || mNpcs[i].mapId != currentMapId()) continue;
		Npc& npc = mNpcs[i];
		npc.updateMovement(deltaTime, mRouteChallengeNpc == (int)i ? 7.2f : 2.8f);
		if (!npc.canWander()) continue;
		if (npc.isRouteDuelist() && (mRouteChallengeNpc == (int)i ||
			routeDuelistCanCatchPlayer((int)i))) continue;
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
			if (!isWalkable(nextX, nextY) || npcAt(nextX, nextY, (int)i) >= 0 ||
				worldObjectAt(nextX, nextY) >= 0) continue;
			if ((nextX == mPlayerX && nextY == mPlayerY) ||
				(nextX == (int)std::round(mVisualX) && nextY == (int)std::round(mVisualY))) continue;
			bool visuallyOccupied = false;
			for (size_t other = 0; other < mNpcs.size(); ++other)
				if (other != i && nextX == (int)std::round(mNpcs[other].visualX) &&
					nextY == (int)std::round(mNpcs[other].visualY)) visuallyOccupied = true;
			if (visuallyOccupied) continue;
			npc.moveTo(nextX, nextY);
			break;
		}
		npc.scheduleWander(now);
	}
	updateRouteDuelistChallenge();
}

void Application::updateRegionBanner()
{
	const WorldRegion* region = currentWorldRegion();
	std::string regionId = region == NULL ? "" : region->id;
	if (regionId == mLastWorldRegionId) return;
	mLastWorldRegionId = regionId;
	if (region == NULL) return;
	mRegionBannerName = region->name;
	mRegionBannerConnector = region->connector;
	mRegionBannerStarted = SDL_GetTicks();
}

void Application::renderRegionBanner()
{
	if (mRegionBannerName.empty()) return;
	Uint32 elapsed = SDL_GetTicks() - mRegionBannerStarted;
	if (elapsed >= REGION_BANNER_DURATION) return;
	const int width = 252;
	int slide = 0;
	if (elapsed < REGION_BANNER_SLIDE)
		slide = (int)((REGION_BANNER_SLIDE - elapsed) * width / REGION_BANNER_SLIDE);
	else if (elapsed > REGION_BANNER_DURATION - REGION_BANNER_SLIDE)
		slide = (int)((elapsed - (REGION_BANNER_DURATION - REGION_BANNER_SLIDE)) *
			width / REGION_BANNER_SLIDE);
	int x = LOGICAL_WIDTH - width - 18 + slide;
	const SDL_Rect panel = { x, 70, width, 58 };
	fillRect(panel, 13, 23, 38, 224);
	fillRect({ x, panel.y, 5, panel.h }, mRegionBannerConnector ? 89 : 207,
		mRegionBannerConnector ? 151 : 163, mRegionBannerConnector ? 194 : 65, 255);
	drawText(mRegionBannerConnector ? "NEW REGION" : "NOW ENTERING", x + 16,
		panel.y + 7, mRegionBannerConnector ? color(139, 199, 231) : color(234, 193, 104), 10);
	drawText(mRegionBannerName, x + 16, panel.y + 25, color(235, 240, 247), 15,
		width - 28);
}

bool Application::routeDuelistCanCatchPlayer(int npcIndex) const
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return false;
	const Npc& npc = mNpcs[npcIndex];
	if (!npc.isRouteDuelist() || !npc.canBattle() || npc.wins > 0 ||
		npc.mapId != currentMapId() || npc.isMoving()) return false;
	int distance = std::abs(mPlayerX - npc.x) + std::abs(mPlayerY - npc.y);
	if (distance <= 0 || distance > npc.sightRange) return false;
	int nextX = npc.x;
	int nextY = npc.y;
	return routeDuelistNextStep(npcIndex, nextX, nextY);
}

bool Application::routeDuelistNextStep(int npcIndex, int& nextX, int& nextY) const
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return false;
	const Npc& npc = mNpcs[npcIndex];
	if (npc.mapId != currentMapId()) return false;
	typedef std::pair<int, int> Cell;
	Cell start = std::make_pair(npc.x, npc.y);
	if (std::abs(npc.x - mPlayerX) + std::abs(npc.y - mPlayerY) <= 1)
	{
		nextX = npc.x;
		nextY = npc.y;
		return true;
	}
	std::vector<Cell> frontier;
	std::map<Cell, Cell> parents;
	std::map<Cell, int> depths;
	frontier.push_back(start);
	parents[start] = start;
	depths[start] = 0;
	const int directionX[] = { 0, 1, 0, -1 };
	const int directionY[] = { -1, 0, 1, 0 };
	const int maximumPathLength = std::max(8, npc.sightRange * 3);
	for (size_t currentIndex = 0; currentIndex < frontier.size(); ++currentIndex)
	{
		Cell current = frontier[currentIndex];
		if (std::abs(current.first - mPlayerX) + std::abs(current.second - mPlayerY) == 1)
		{
			Cell step = current;
			while (parents[step] != start && step != start) step = parents[step];
			if (step == start) return false;
			nextX = step.first;
			nextY = step.second;
			return true;
		}
		if (depths[current] >= maximumPathLength) continue;
		for (int direction = 0; direction < 4; ++direction)
		{
			Cell candidate = std::make_pair(current.first + directionX[direction],
				current.second + directionY[direction]);
			if (candidate.first == mPlayerX && candidate.second == mPlayerY) continue;
			if (parents.count(candidate) || !isWalkable(candidate.first, candidate.second) ||
				worldObjectAt(candidate.first, candidate.second) >= 0 ||
				npcAt(candidate.first, candidate.second, npcIndex) >= 0) continue;
			parents[candidate] = current;
			depths[candidate] = depths[current] + 1;
			frontier.push_back(candidate);
		}
	}
	return false;
}

void Application::updateRouteDuelistChallenge()
{
	if (mDialogueNpc >= 0 || mDialogueObject >= 0 || mNpcMenuNpc >= 0 || mPauseMenuOpen ||
		mOpeningPortal >= 0 || mStoryScene != StoryScene::None) return;
	if (std::fabs(mPlayerX - mVisualX) > 0.001f ||
		std::fabs(mPlayerY - mVisualY) > 0.001f) return;

	if (mRouteChallengeNpc >= 0)
	{
		if (mRouteChallengeNpc >= (int)mNpcs.size() ||
			!npcVisible(mRouteChallengeNpc) ||
			mNpcs[mRouteChallengeNpc].mapId != currentMapId())
		{
			mRouteChallengeNpc = -1;
			return;
		}
		Npc& npc = mNpcs[mRouteChallengeNpc];
		if (npc.isMoving()) return;
		int dx = mPlayerX - npc.x;
		int dy = mPlayerY - npc.y;
		if (std::abs(dx) + std::abs(dy) <= 1)
		{
			int challenger = mRouteChallengeNpc;
			mRouteChallengeNpc = -1;
			beginDialogue(challenger, npc.dialogueText("greeting",
				"I saw you on the road. Prepare to duel!"), DialogueAction::ForcedBattle);
			return;
		}
		int nextX = npc.x;
		int nextY = npc.y;
		if (!routeDuelistNextStep(mRouteChallengeNpc, nextX, nextY))
		{
			mRouteChallengeNpc = -1;
			return;
		}
		npc.moveTo(nextX, nextY);
		return;
	}

	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		Npc& npc = mNpcs[i];
		if (!npc.isRouteDuelist() || npc.mapId != currentMapId()) continue;
		bool catchesPlayer = routeDuelistCanCatchPlayer((int)i);
		if (mSuppressedRouteChallenges.count(npc.id))
		{
			if (!catchesPlayer) mSuppressedRouteChallenges.erase(npc.id);
			continue;
		}
		if (!catchesPlayer) continue;
		mRouteChallengeNpc = (int)i;
		mSuppressedRouteChallenges.insert(npc.id);
		mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
		mMoveIntentX = mMoveIntentY = 0;
		mNotice = npc.name + " spotted you!";
		mNoticeUntil = SDL_GetTicks() + 2500;
		return;
	}
}

bool Application::isWalkable(int x, int y) const
{
	const std::vector<std::string>& map = currentMap();
	if (y < 0 || y >= (int)map.size() || x < 0 || x >= (int)map[y].size()) return false;
	WorldTileId tile = WorldTiles::fromGlyph(map[y][x]);
	if (tile == WorldTiles::BlackstoneGate && !hasCrest("confluence")) return false;
	return WorldTiles::isWalkable(tile);
}

int Application::npcAt(int x, int y, int ignoredNpc) const
{
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if ((int)i != ignoredNpc && npcVisible((int)i) && mNpcs[i].mapId == currentMapId() &&
			mNpcs[i].x == x && mNpcs[i].y == y) return (int)i;
	return -1;
}

int Application::worldObjectAt(int x, int y) const
{
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
		if (mWorldObjects[i].mapId == currentMapId() &&
			mWorldObjects[i].x == x && mWorldObjects[i].y == y) return (int)i;
	return -1;
}

void Application::tryMove(int dx, int dy)
{
	mFacingX = dx;
	mFacingY = dy;
	int x = mPlayerX + dx;
	int y = mPlayerY + dy;
	const std::vector<std::string>& map = currentMap();
	if (y >= 0 && y < (int)map.size() && x >= 0 && x < (int)map[y].size() &&
		WorldTiles::fromGlyph(map[y][x]) == WorldTiles::BlackstoneGate &&
		!hasCrest("confluence"))
	{
		mNotice = "The Blackstone gate is sealed. Dragon Keep's Confluence relay must be restored.";
		mNoticeUntil = SDL_GetTicks() + 4500;
		return;
	}
	const WorldRegion* fromRegion = worldRegionAt(currentMapId(), mPlayerX, mPlayerY);
	const WorldRegion* toRegion = worldRegionAt(currentMapId(), x, y);
	if (mStoryStage < 4 && fromRegion != NULL && toRegion != NULL &&
		fromRegion->id == "emberglen" &&
		(toRegion->id == "old_road" || toRegion->id == "watershed_crossroads"))
	{
		mNotice = "The roads beyond Emberglen are unsafe. Finish the investigation first.";
		mNoticeUntil = SDL_GetTicks() + 4500;
		return;
	}
	bool occupiedByMovingNpc = false;
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (npcVisible((int)i) && mNpcs[i].mapId == currentMapId() &&
			x == (int)std::round(mNpcs[i].visualX) &&
			y == (int)std::round(mNpcs[i].visualY))
			occupiedByMovingNpc = true;
	if (isWalkable(x, y) && npcAt(x, y) < 0 && worldObjectAt(x, y) < 0 &&
		!occupiedByMovingNpc)
	{
		mPlayerX = x;
		mPlayerY = y;
		if (!beginPortalAt(x, y))
		{
			discoverLandmarkAt(x, y);
			collectShardAt(x, y);
		}
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

void Application::discoverLandmarkAt(int x, int y)
{
	for (size_t landmark = 0; landmark < Landmarks::COUNT; ++landmark)
	{
		const Landmarks::Definition& definition = Landmarks::DEFINITIONS[landmark];
		if (mPlayerDataLoaded && mDiscoveredLandmarks.count(definition.id)) continue;
		const WorldRegion* region = NULL;
		for (size_t candidate = 0; candidate < mWorldRegions.size(); ++candidate)
			if (mWorldRegions[candidate].mapId == currentMapId() &&
				mWorldRegions[candidate].id == definition.regionId)
			{
				region = &mWorldRegions[candidate];
				break;
			}
		if (region == NULL) continue;
		int landmarkX = region->x + definition.localX;
		int landmarkY = region->y + definition.localY;
		if (std::abs(x - landmarkX) + std::abs(y - landmarkY) >
			definition.discoveryRadius) continue;

		ensurePlayerDataLoaded();
		if (mDiscoveredLandmarks.count(definition.id)) return;
		mDiscoveredLandmarks.insert(definition.id);
		mMoney += definition.goldReward;
		savePlayerProgress();
		mNotice = "Landmark discovered: " + std::string(definition.name) + "  (+" +
			std::to_string(definition.goldReward) + " gold)";
		mNoticeUntil = SDL_GetTicks() + 6000;
		return;
	}
}

void Application::interact()
{
	if (mOpeningPortal >= 0) return;
	if (std::fabs(mPlayerX - mVisualX) > 0.001f || std::fabs(mPlayerY - mVisualY) > 0.001f) return;
	int targetX = mPlayerX + mFacingX;
	int targetY = mPlayerY + mFacingY;
	int objectIndex = worldObjectAt(targetX, targetY);
	if (objectIndex >= 0)
	{
		beginObjectDialogue(objectIndex);
		return;
	}
	int npcIndex = npcAt(targetX, targetY);
	if (npcIndex < 0) return;
	Npc& npc = mNpcs[npcIndex];
	if (npc.isTownNpc())
		beginDialogue(npcIndex, npc.dialogueText("greeting", "Hello there."),
			DialogueAction::OpenNpcMenu);
	else
	{
		discoverStoryClue(npcIndex);
		beginDialogue(npcIndex, storyDialogueForNpc(npcIndex), DialogueAction::NpcInteraction);
	}
}

void Application::beginDialogue(int npcIndex, const std::string& text, DialogueAction action)
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return;
	mDialogueNpc = npcIndex;
	mDialogueObject = -1;
	mDialogueText = text;
	mDialogueVisibleBytes = 0;
	mDialogueCharacterAccumulator = 0;
	mDialogueAction = action;
	mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
	mMoveIntentX = mMoveIntentY = 0;
}

void Application::beginObjectDialogue(int objectIndex)
{
	if (objectIndex < 0 || objectIndex >= (int)mWorldObjects.size()) return;
	WorldObject& object = mWorldObjects[objectIndex];
	std::string dialogue = object.text;
	if (object.kind == WorldObjectKind::DeckChest)
	{
		ensurePlayerDataLoaded();
		if (mOpenedWorldObjects.count(object.id)) dialogue = object.openedText;
		else
		{
			mOpenedWorldObjects.insert(object.id);
			std::string error;
			if (awardDeckReward(object.rewardDeck, object.rewardDeckName, error))
			{
				savePlayerProgress();
				mNotice = "Found the " + object.rewardDeckName + " deck!";
				mNoticeUntil = SDL_GetTicks() + 6500;
			}
			else
			{
				mOpenedWorldObjects.erase(object.id);
				dialogue = error;
			}
		}
	}
	mDialogueNpc = -1;
	mDialogueObject = objectIndex;
	mDialogueText = dialogue;
	mDialogueVisibleBytes = 0;
	mDialogueCharacterAccumulator = 0;
	mDialogueAction = DialogueAction::Close;
	mMoveUp = mMoveDown = mMoveLeft = mMoveRight = false;
	mMoveIntentX = mMoveIntentY = 0;
}

void Application::clearDialogue()
{
	mDialogueNpc = -1;
	mDialogueObject = -1;
	mDialogueText.clear();
	mDialogueVisibleBytes = 0;
	mDialogueCharacterAccumulator = 0;
	mDialogueAction = DialogueAction::None;
}

void Application::advanceDialogue()
{
	if (mDialogueNpc < 0 && mDialogueObject < 0) return;
	if (mDialogueVisibleBytes < mDialogueText.size())
	{
		mDialogueVisibleBytes = mDialogueText.size();
		mDialogueCharacterAccumulator = 0;
		return;
	}

	int npcIndex = mDialogueNpc;
	int objectIndex = mDialogueObject;
	DialogueAction action = mDialogueAction;
	clearDialogue();
	if (objectIndex >= 0) return;
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
		if (npc.canBattle()) startDuel(npcIndex);
	}
	else if ((action == DialogueAction::OpenNpcMenu ||
		action == DialogueAction::ReturnToNpcMenu) &&
		npcIndex >= 0 && npcIndex < (int)mNpcs.size())
	{
		mNpcMenuNpc = npcIndex;
		mNpcMenuSelection = 0;
	}
	else if (action == DialogueAction::ForcedBattle &&
		npcIndex >= 0 && npcIndex < (int)mNpcs.size() && mNpcs[npcIndex].canBattle())
		startDuel(npcIndex);
}

std::vector<Application::NpcMenuAction> Application::npcMenuActions(int npcIndex) const
{
	std::vector<NpcMenuAction> actions;
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return actions;
	actions.push_back(NpcMenuAction::Talk);
	if (mNpcs[npcIndex].isDuelist()) actions.push_back(NpcMenuAction::Duel);
	if (mNpcs[npcIndex].canTrade()) actions.push_back(NpcMenuAction::Trade);
	actions.push_back(NpcMenuAction::Leave);
	return actions;
}

void Application::activateNpcMenuAction(NpcMenuAction action)
{
	if (mNpcMenuNpc < 0 || mNpcMenuNpc >= (int)mNpcs.size()) return;
	int npcIndex = mNpcMenuNpc;
	Npc& npc = mNpcs[npcIndex];
	mNpcMenuNpc = -1;
	if (action == NpcMenuAction::Talk)
	{
		discoverStoryClue(npcIndex);
		beginDialogue(npcIndex, storyDialogueForNpc(npcIndex),
			DialogueAction::ReturnToNpcMenu);
	}
	else if (action == NpcMenuAction::Duel)
	{
		if (npc.canBattle()) startDuel(npcIndex);
		else
			beginDialogue(npcIndex, npc.dialogueText("complete",
				"We have already settled every duel between us."),
				DialogueAction::ReturnToNpcMenu);
	}
	else if (action == NpcMenuAction::Trade && npc.canTrade())
		enterShop();
}

void Application::handleNpcMenuEvent(const SDL_Event& event)
{
	std::vector<NpcMenuAction> actions = npcMenuActions(mNpcMenuNpc);
	if (actions.empty())
	{
		mNpcMenuNpc = -1;
		return;
	}
	mNpcMenuSelection = std::max(0, std::min((int)actions.size() - 1, mNpcMenuSelection));
	if (event.type == SDL_KEYDOWN && !event.key.repeat)
	{
		SDL_Keycode key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE)
		{
			mNpcMenuNpc = -1;
			return;
		}
		if (key == SDLK_w || key == SDLK_UP)
			mNpcMenuSelection = (mNpcMenuSelection + (int)actions.size() - 1) % actions.size();
		else if (key == SDLK_s || key == SDLK_DOWN)
			mNpcMenuSelection = (mNpcMenuSelection + 1) % actions.size();
		else if (key == SDLK_RETURN || key == SDLK_SPACE || key == SDLK_e)
			activateNpcMenuAction(actions[mNpcMenuSelection]);
		return;
	}
	if (event.type != SDL_MOUSEMOTION &&
		(event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT)) return;
	int x, y;
	if (event.type == SDL_MOUSEMOTION)
		logicalMouse(event.motion.x, event.motion.y, x, y);
	else
		logicalMouse(event.button.x, event.button.y, x, y);
	for (size_t i = 0; i < actions.size(); ++i)
	{
		SDL_Rect button = { 856, 468 + (int)i * 58, 340, 48 };
		if (!contains(button, x, y)) continue;
		mNpcMenuSelection = (int)i;
		if (event.type == SDL_MOUSEBUTTONDOWN) activateNpcMenuAction(actions[i]);
		return;
	}
}

void Application::renderNpcMenu()
{
	if (mNpcMenuNpc < 0 || mNpcMenuNpc >= (int)mNpcs.size()) return;
	const Npc& npc = mNpcs[mNpcMenuNpc];
	std::vector<NpcMenuAction> actions = npcMenuActions(mNpcMenuNpc);
	int panelHeight = 82 + (int)actions.size() * 58;
	SDL_Rect panel = { 830, 410, 390, panelHeight };
	fillRect(panel, 14, 22, 35, 248);
	outlineRect(panel, 194, 148, 62, 255, 3);
	drawText(npc.name, 856, 428, color(244, 206, 103), 23, 330);
	for (size_t i = 0; i < actions.size(); ++i)
	{
		SDL_Rect button = { 856, 468 + (int)i * 58, 340, 48 };
		bool selected = (int)i == mNpcMenuSelection;
		fillRect(button, selected ? 48 : 29, selected ? 66 : 43,
			selected ? 96 : 66, 250);
		outlineRect(button, selected ? 226 : 102, selected ? 178 : 126,
			selected ? 86 : 172, 255, selected ? 3 : 2);
		std::string label;
		if (actions[i] == NpcMenuAction::Talk) label = "Talk";
		else if (actions[i] == NpcMenuAction::Duel)
			label = npc.canBattle() ? "Duel  •  " + npc.rankName() : "Duel  •  Complete";
		else if (actions[i] == NpcMenuAction::Trade) label = "Trade";
		else label = "Leave";
		drawText(label, button.x + 18, button.y + 13,
			selected ? color(247, 224, 155) : color(224, 231, 241), 17, 304);
	}
}

void Application::updateDialogue(Uint32 deltaTime)
{
	if ((mDialogueNpc < 0 && mDialogueObject < 0) ||
		mDialogueVisibleBytes >= mDialogueText.size()) return;
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
	float maximum = (float)std::max(0, (int)map[0].size() - OVERWORLD_VIEW_COLUMNS);
	return std::max(0.f, std::min(maximum,
		mVisualX - (OVERWORLD_VIEW_COLUMNS - 1) * 0.5f));
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
	const WorldRegion* cinderrailRegion = NULL;
	for (size_t region = 0; region < mWorldRegions.size(); ++region)
	{
		if (mWorldRegions[region].mapId != currentMapId()) continue;
		if (mWorldRegions[region].id == "cinderrail") cinderrailRegion = &mWorldRegions[region];
	}
	float cameraX = overworldCameraX();
	float cameraY = overworldCameraY();
	int mapX = mapOriginX((int)map[0].size(), OVERWORLD_VIEW_COLUMNS) -
		(int)std::round(cameraX * TILE);
	int mapY = mapOriginY((int)map.size()) - (int)std::round(cameraY * TILE);
	TileBounds visibleTiles = visibleTileBounds(cameraX, cameraY,
		(int)map[0].size(), (int)map.size(), OVERWORLD_VIEW_COLUMNS, MAP_VIEW_ROWS);
	SDL_Rect mapViewport = { MAP_X, MAP_Y, OVERWORLD_VIEW_WIDTH, MAP_VIEW_HEIGHT };
	SDL_RenderSetClipRect(mRenderer, &mapViewport);
	for (int y = visibleTiles.top; y < visibleTiles.bottom; ++y)
	{
		for (int x = visibleTiles.left; x < visibleTiles.right; ++x)
		{
			SDL_Rect tileRect = { mapX + (int)x * TILE, mapY + (int)y * TILE, TILE, TILE };
			WorldTileId tile = WorldTiles::fromGlyph(map[y][x]);
			if (tile == WorldTiles::Path) fillRect(tileRect, 162, 132, 76);
			else if (tile == WorldTiles::OldRoadPath) fillRect(tileRect, 124, 112, 88);
			else if (tile == WorldTiles::CinderrailPath) fillRect(tileRect, 116, 91, 58);
			else if (tile == WorldTiles::WatershedPath) fillRect(tileRect, 148, 139, 105);
			else if (tile == WorldTiles::GlasswaterPaving) fillRect(tileRect, 165, 199, 202);
			else if (tile == WorldTiles::RootmazePath) fillRect(tileRect, 119, 134, 75);
			else if (tile == WorldTiles::BlackstonePath) fillRect(tileRect, 111, 106, 96);
			else if (tile == WorldTiles::Water) fillRect(tileRect, 25, 111, 157);
			else if (tile == WorldTiles::House) fillRect(tileRect, 126, 65, 43);
			else if (tile == WorldTiles::Forest || tile == WorldTiles::Tree)
				fillRect(tileRect, 26, 75, 33);
			else if (tile == WorldTiles::CinderrailRubble) fillRect(tileRect, 55, 45, 43);
			else if (tile == WorldTiles::DuelSand) fillRect(tileRect, 188, 151, 87);
			else if (tile == WorldTiles::CinderrailDuelSand) fillRect(tileRect, 118, 72, 48);
			else if (tile == WorldTiles::Marble) fillRect(tileRect, 198, 204, 207);
			else if (tile == WorldTiles::OldRoadWaystone) fillRect(tileRect, 79, 72, 65);
			else if (tile == WorldTiles::MarbleRoof) fillRect(tileRect, 201, 196, 177);
			else if (tile == WorldTiles::Bonfire) fillRect(tileRect, 83, 64, 42);
			else if (tile == WorldTiles::FeastTable) fillRect(tileRect, 61, 139, 61);
			else if (tile == WorldTiles::WorkshopTools) fillRect(tileRect, 137, 91, 49);
			else if (tile == WorldTiles::Rail || tile == WorldTiles::RailCrossing)
				fillRect(tileRect, tile == WorldTiles::RailCrossing ? 104 : 47,
					tile == WorldTiles::RailCrossing ? 83 : 45,
					tile == WorldTiles::RailCrossing ? 57 : 43);
			else if (tile == WorldTiles::MetalGrate) fillRect(tileRect, 73, 80, 86);
			else if (tile == WorldTiles::IndustrialBrick) fillRect(tileRect, 112, 49, 38);
			else if (tile == WorldTiles::Machinery) fillRect(tileRect, 63, 66, 67);
			else if (tile == WorldTiles::Furnace) fillRect(tileRect, 64, 47, 43);
			else if (tile == WorldTiles::TimberRoof) fillRect(tileRect, 91, 48, 37);
			else if (tile == WorldTiles::IndustrialRoof) fillRect(tileRect, 67, 68, 70);
			else if (tile == WorldTiles::TimberBridge) fillRect(tileRect, 31, 99, 132);
			else if (tile == WorldTiles::RockyCliff) fillRect(tileRect, 73, 65, 59);
			else if (tile == WorldTiles::WoodWall || tile == WorldTiles::Door ||
				tile == WorldTiles::CinderrailDoor || tile == WorldTiles::WoodFloor ||
				tile == WorldTiles::Counter)
				fillRect(tileRect, tile == WorldTiles::WoodFloor ? 137 : 91,
					tile == WorldTiles::WoodFloor ? 91 : 53,
					tile == WorldTiles::WoodFloor ? 49 : 31);
			else if (tile == WorldTiles::CinderrailGround) fillRect(tileRect, 82, 76, 59);
			else if (tile == WorldTiles::WatershedGround ||
				tile == WorldTiles::WatershedMarker) fillRect(tileRect, 56, 124, 74);
			else if (tile == WorldTiles::GlasswaterGround ||
				tile == WorldTiles::GlasswaterMarker) fillRect(tileRect, 91, 151, 153);
			else if (tile == WorldTiles::GlasswaterRoof) fillRect(tileRect, 42, 91, 134);
			else if (tile == WorldTiles::GlasswaterDock) fillRect(tileRect, 106, 76, 48);
			else if (tile == WorldTiles::GlasswaterWall) fillRect(tileRect, 157, 190, 191);
			else if (tile == WorldTiles::GlasswaterDoor) fillRect(tileRect, 57, 111, 137);
			else if (tile == WorldTiles::GlasswaterArena) fillRect(tileRect, 104, 164, 188);
			else if (tile == WorldTiles::RootmazeGround ||
				tile == WorldTiles::RootmazeMarker) fillRect(tileRect, 72, 126, 63);
			else if (tile == WorldTiles::RootmazeRoot) fillRect(tileRect, 76, 55, 35);
			else if (tile == WorldTiles::RootmazeBridge) fillRect(tileRect, 130, 92, 53);
			else if (tile == WorldTiles::RootmazeRoof) fillRect(tileRect, 74, 116, 51);
			else if (tile == WorldTiles::RootmazeWall) fillRect(tileRect, 109, 78, 47);
			else if (tile == WorldTiles::RootmazeDoor) fillRect(tileRect, 126, 89, 50);
			else if (tile == WorldTiles::RootmazeArena) fillRect(tileRect, 105, 154, 76);
			else if (tile == WorldTiles::BlackstoneGround) fillRect(tileRect, 61, 59, 55);
			else if (tile == WorldTiles::BlackstoneWall) fillRect(tileRect, 38, 37, 40);
			else if (tile == WorldTiles::BlackstoneGate) fillRect(tileRect, 72, 67, 58);
			else if (tile == WorldTiles::Rocks) fillRect(tileRect, 61, 139, 61);
			else if (tile == WorldTiles::Bush) fillRect(tileRect, 49, 126, 54);
			else if (tile == WorldTiles::Shrub) fillRect(tileRect, 61, 139, 61);
			else if (tile == WorldTiles::CaveEntrance) fillRect(tileRect, 78, 70, 62);
			else if (tile == WorldTiles::TreeStump) fillRect(tileRect, 61, 139, 61);
			else fillRect(tileRect, 61, 139, 61);
			mWorldTileRenderer->drawTerrain(tile, tileRect);

			if (tile == WorldTiles::Water)
			{
				int wave = (int)((SDL_GetTicks() / 180 + x * 5 + y * 3) % 24);
				fillRect({ tileRect.x + 7, tileRect.y + 10 + wave / 3, 28, 3 }, 92, 189, 210, 190);
			}
			else if (tile == WorldTiles::CinderrailRubble)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 5, 42, 38 }, 72, 58, 54);
				fillRect({ tileRect.x + 7, tileRect.y + 11, 17, 4 }, 91, 70, 61);
				fillRect({ tileRect.x + 27, tileRect.y + 25, 14, 4 }, 39, 35, 37);
				fillRect({ tileRect.x + 18, tileRect.y + 15, 4, 13 }, 45, 39, 40);
			}
			else if (tile == WorldTiles::Forest || tile == WorldTiles::Tree)
			{
				fillRect({ tileRect.x + 19, tileRect.y + 27, 10, 18 }, 85, 48, 26);
				fillRect({ tileRect.x + 6, tileRect.y + 5, 36, 30 }, 41, 116, 49);
			}
			else if (tile == WorldTiles::House)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 4, 42, 16 }, 186, 76, 46);
				fillRect({ tileRect.x + 18, tileRect.y + 23, 13, 25 }, 54, 31, 24);
			}
			else if (tile == WorldTiles::TimberBridge)
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
			else if (tile == WorldTiles::RockyCliff)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 91, 80, 69);
				fillRect({ tileRect.x + 5, tileRect.y + 8, 19, 8 }, 116, 99, 78);
				fillRect({ tileRect.x + 27, tileRect.y + 18, 15, 9 }, 58, 55, 54);
				fillRect({ tileRect.x + 10, tileRect.y + 27, 24, 4 }, 66, 60, 56);
				fillRect({ tileRect.x + 20, tileRect.y + 14, 3, 14 }, 54, 51, 50);
				fillRect({ tileRect.x + 33, tileRect.y + 7, 7, 3 }, 143, 118, 84);
			}
			else if (tile == WorldTiles::Rocks)
			{
				fillRect({ tileRect.x + 5, tileRect.y + 26, 21, 16 }, 96, 91, 82);
				fillRect({ tileRect.x + 17, tileRect.y + 13, 24, 27 }, 125, 117, 103);
				fillRect({ tileRect.x + 21, tileRect.y + 16, 14, 5 }, 163, 153, 134);
				fillRect({ tileRect.x + 8, tileRect.y + 28, 9, 4 }, 137, 128, 111);
			}
			else if (tile == WorldTiles::Bush || tile == WorldTiles::Shrub)
			{
				int top = tile == WorldTiles::Bush ? 9 : 24;
				int height = tile == WorldTiles::Bush ? 34 : 18;
				fillRect({ tileRect.x + 5, tileRect.y + top + 7, 38, height - 7 },
					tile == WorldTiles::Bush ? 34 : 52,
					tile == WorldTiles::Bush ? 105 : 137, 48);
				fillRect({ tileRect.x + 10, tileRect.y + top, 18, 15 }, 71, 153, 65);
				fillRect({ tileRect.x + 25, tileRect.y + top + 5, 15, 14 }, 45, 124, 53);
				fillRect({ tileRect.x + 17, tileRect.y + top + 12, 5, 4 }, 99, 173, 76);
			}
			else if (tile == WorldTiles::CaveEntrance)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 7, 42, 41 }, 112, 99, 82);
				fillRect({ tileRect.x + 10, tileRect.y + 15, 28, 33 }, 30, 31, 33);
				fillRect({ tileRect.x + 6, tileRect.y + 8, 13, 8 }, 151, 135, 108);
				fillRect({ tileRect.x + 32, tileRect.y + 12, 9, 5 }, 73, 67, 61);
				fillRect({ tileRect.x + 15, tileRect.y + 41, 18, 4 }, 49, 44, 40);
			}
			else if (tile == WorldTiles::TreeStump)
			{
				fillRect({ tileRect.x + 15, tileRect.y + 21, 20, 21 }, 105, 65, 36);
				fillRect({ tileRect.x + 11, tileRect.y + 16, 28, 11 }, 155, 103, 53);
				fillRect({ tileRect.x + 17, tileRect.y + 19, 16, 5 }, 91, 57, 34);
				fillRect({ tileRect.x + 7, tileRect.y + 39, 13, 5 }, 75, 52, 31);
				fillRect({ tileRect.x + 31, tileRect.y + 38, 11, 5 }, 75, 52, 31);
			}
			else if (tile == WorldTiles::TimberRoof)
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
				if (x == 0 || WorldTiles::fromGlyph(map[y][x - 1]) != WorldTiles::TimberRoof)
					fillRect({ tileRect.x, tileRect.y + 4, 4, 41 }, 48, 33, 30);
				if (x + 1 >= (int)map[y].size() ||
					WorldTiles::fromGlyph(map[y][x + 1]) != WorldTiles::TimberRoof)
					fillRect({ tileRect.x + 44, tileRect.y + 4, 4, 41 }, 48, 33, 30);
			}
			else if (tile == WorldTiles::MarbleRoof)
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
				if (x == 0 || WorldTiles::fromGlyph(map[y][x - 1]) != WorldTiles::MarbleRoof)
					fillRect({ tileRect.x, tileRect.y + 4, 4, 41 }, 139, 142, 139);
				if (x + 1 >= (int)map[y].size() ||
					WorldTiles::fromGlyph(map[y][x + 1]) != WorldTiles::MarbleRoof)
					fillRect({ tileRect.x + 44, tileRect.y + 4, 4, 41 }, 139, 142, 139);
			}
			else if (tile == WorldTiles::WoodWall)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 124, 72, 38);
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tileRect.x + 3, tileRect.y + 8 + plank * 10, 42, 2 }, 76, 42, 27);
				fillRect({ tileRect.x + 5, tileRect.y + 2, 5, 46 }, 67, 38, 26);
				fillRect({ tileRect.x + 38, tileRect.y + 2, 5, 46 }, 67, 38, 26);
			}
			else if (tile == WorldTiles::Door || tile == WorldTiles::CinderrailDoor)
			{
				bool cinderDoor = tile == WorldTiles::CinderrailDoor;
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
					cinderDoor ? 75 : 104, cinderDoor ? 71 : 57, cinderDoor ? 68 : 31);
				fillRect({ tileRect.x + 11, tileRect.y + 6, 26, 38 }, 24, 20, 19);
				int doorWidth = std::max(2, (int)std::round(26.f * (1.f - open)));
				fillRect({ tileRect.x + 11, tileRect.y + 6, doorWidth, 38 },
					cinderDoor ? 107 : 139, cinderDoor ? 111 : 78, cinderDoor ? 112 : 39);
				if (cinderDoor && doorWidth > 6)
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
			else if (tile == WorldTiles::WoodFloor)
			{
				for (int plank = 0; plank < 4; ++plank)
					fillRect({ tileRect.x, tileRect.y + plank * 12, 48, 2 }, 96, 58, 36);
				fillRect({ tileRect.x + 23, tileRect.y + 2, 2, 10 }, 109, 67, 39);
				fillRect({ tileRect.x + 10, tileRect.y + 26, 2, 10 }, 109, 67, 39);
			}
			else if (tile == WorldTiles::Counter)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 10, 44, 32 }, 112, 62, 34);
				fillRect({ tileRect.x, tileRect.y + 7, 48, 7 }, 166, 105, 53);
				fillRect({ tileRect.x + 8, tileRect.y + 18, 4, 22 }, 71, 40, 27);
				fillRect({ tileRect.x + 36, tileRect.y + 18, 4, 22 }, 71, 40, 27);
			}
			else if (tile == WorldTiles::Bonfire)
			{
				fillRect({ tileRect.x + 5, tileRect.y + 32, 38, 8 }, 74, 70, 65);
				fillRect({ tileRect.x + 10, tileRect.y + 29, 28, 10 }, 111, 100, 83);
				int flicker = (int)((SDL_GetTicks() / 110 + x + y) % 3);
				fillRect({ tileRect.x + 17, tileRect.y + 13 + flicker, 15, 22 - flicker },
					221, 69, 32);
				fillRect({ tileRect.x + 20, tileRect.y + 8 - flicker, 10, 23 }, 249, 137, 36);
				fillRect({ tileRect.x + 23, tileRect.y + 16, 6, 15 }, 255, 222, 89);
			}
			else if (tile == WorldTiles::FeastTable)
			{
				fillRect({ tileRect.x + 6, tileRect.y + 6, 36, 5 }, 93, 52, 29);
				fillRect({ tileRect.x + 6, tileRect.y + 37, 36, 5 }, 93, 52, 29);
				fillRect({ tileRect.x + 4, tileRect.y + 15, 40, 18 }, 139, 82, 39);
				fillRect({ tileRect.x + 8, tileRect.y + 18, 32, 3 }, 181, 116, 54);
				fillRect({ tileRect.x + 12, tileRect.y + 23, 7, 6 }, 232, 208, 151);
				fillRect({ tileRect.x + 29, tileRect.y + 22, 8, 7 }, 175, 49, 37);
			}
			else if (tile == WorldTiles::DuelSand || tile == WorldTiles::CinderrailDuelSand)
			{
				if (tile == WorldTiles::CinderrailDuelSand)
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
			else if (tile == WorldTiles::Marble || tile == WorldTiles::OldRoadWaystone)
			{
				if (tile == WorldTiles::OldRoadWaystone)
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
			else if (tile == WorldTiles::WorkshopTools)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 24, 42, 17 }, 104, 58, 32);
				fillRect({ tileRect.x + 5, tileRect.y + 20, 38, 6 }, 171, 108, 51);
				fillRect({ tileRect.x + 9, tileRect.y + 10, 7, 11 }, 54, 151, 193);
				fillRect({ tileRect.x + 20, tileRect.y + 7, 6, 14 }, 151, 71, 183);
				fillRect({ tileRect.x + 31, tileRect.y + 12, 8, 9 }, 217, 158, 48);
			}
			else if (tile == WorldTiles::Rail || tile == WorldTiles::RailCrossing)
			{
				for (int tie = 2; tie < 48; tie += 10)
					fillRect({ tileRect.x + tie, tileRect.y + 5, 5, 38 },
						tile == WorldTiles::RailCrossing ? 155 : 100,
						tile == WorldTiles::RailCrossing ? 112 : 73,
						tile == WorldTiles::RailCrossing ? 55 : 48);
				fillRect({ tileRect.x, tileRect.y + 9, 48, 5 }, 151, 158, 158);
				fillRect({ tileRect.x, tileRect.y + 34, 48, 5 }, 151, 158, 158);
				fillRect({ tileRect.x, tileRect.y + 11, 48, 2 }, 221, 225, 220);
				fillRect({ tileRect.x, tileRect.y + 36, 48, 2 }, 221, 225, 220);
				if (tile == WorldTiles::RailCrossing)
					for (int plate = 3; plate < 48; plate += 9)
						fillRect({ tileRect.x + plate, tileRect.y + 16, 6, 15 }, 186, 151, 72);
			}
			else if (tile == WorldTiles::MetalGrate)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 2, 44, 44 }, 88, 96, 101);
				for (int grate = 7; grate < 44; grate += 9)
					fillRect({ tileRect.x + grate, tileRect.y + 4, 3, 40 }, 45, 50, 54);
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 3 }, 222, 168, 57);
				fillRect({ tileRect.x + 5, tileRect.y + 8, 4, 4 }, 196, 204, 202);
				fillRect({ tileRect.x + 39, tileRect.y + 36, 4, 4 }, 196, 204, 202);
			}
			else if (tile == WorldTiles::IndustrialRoof)
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
				if (x == 0 || WorldTiles::fromGlyph(map[y][x - 1]) != WorldTiles::IndustrialRoof)
					fillRect({ tileRect.x, tileRect.y + 2, 4, 43 }, 38, 42, 44);
				if (x + 1 >= (int)map[y].size() ||
					WorldTiles::fromGlyph(map[y][x + 1]) != WorldTiles::IndustrialRoof)
					fillRect({ tileRect.x + 44, tileRect.y + 2, 4, 43 }, 38, 42, 44);
			}
			else if (tile == WorldTiles::IndustrialBrick)
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
			else if (tile == WorldTiles::Machinery)
			{
				fillRect({ tileRect.x + 5, tileRect.y + 6, 38, 37 }, 82, 88, 89);
				fillRect({ tileRect.x + 8, tileRect.y + 10, 32, 6 }, 176, 75, 43);
				fillRect({ tileRect.x + 21, tileRect.y + 16, 8, 23 }, 169, 178, 175);
				fillRect({ tileRect.x + 14, tileRect.y + 23, 22, 8 }, 169, 178, 175);
				int pulse = (SDL_GetTicks() / 240 + x + y) % 2 == 0 ? 0 : 3;
				fillRect({ tileRect.x + 12 + pulse, tileRect.y + 34, 7, 5 }, 235, 174, 54);
				fillRect({ tileRect.x + 32 - pulse, tileRect.y + 20, 5, 5 }, 97, 213, 182);
			}
			else if (tile == WorldTiles::Furnace)
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
			else if (tile == WorldTiles::Grass || tile == WorldTiles::CinderrailGround)
			{
				if (tile == WorldTiles::CinderrailGround)
				{
					fillRect({ tileRect.x + 7, tileRect.y + 35, 5, 3 }, 117, 94, 61);
					fillRect({ tileRect.x + 32, tileRect.y + 13, 3, 3 }, 58, 57, 52);
				}
				else fillRect({ tileRect.x + 7, tileRect.y + 34, 3, 8 }, 111, 180, 64);
			}
			if (tile == WorldTiles::OldRoadPath)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 5, 18, 15 }, 143, 130, 101);
				fillRect({ tileRect.x + 24, tileRect.y + 7, 21, 13 }, 105, 98, 82);
				fillRect({ tileRect.x + 6, tileRect.y + 25, 25, 14 }, 104, 98, 82);
				fillRect({ tileRect.x + 34, tileRect.y + 26, 12, 12 }, 148, 131, 95);
				fillRect({ tileRect.x + 19, tileRect.y + 8, 3, 8 }, 62, 82, 60);
				fillRect({ tileRect.x + 31, tileRect.y + 34, 5, 3 }, 67, 91, 62);
			}
			else if (tile == WorldTiles::CinderrailPath)
			{
				SDL_Color route = color(190, 145, 55);
				int localX = cinderrailRegion == NULL ? x : x - cinderrailRegion->x;
				int localY = cinderrailRegion == NULL ? y : y - cinderrailRegion->y;
				if ((localY == 25 || localY == 26) && localX <= 25) route = color(88, 170, 93);
				else if (localY >= 32 && localX >= 15 && localX <= 33) route = color(141, 91, 177);
				else if (localX >= 45 && localY >= 15 && localY <= 21) route = color(222, 225, 216);
				bool verticalRoute = (localX == 25 || localX == 26) && localY != 10 &&
					localY != 11 && localY != 16 && localY != 17 && localY != 25 &&
					localY != 26 && localY != 34;
				if (verticalRoute)
					fillRect({ tileRect.x + 4, tileRect.y, 3, 48 }, route.r, route.g, route.b);
				else fillRect({ tileRect.x, tileRect.y + 4, 48, 3 }, route.r, route.g, route.b);
				fillRect({ tileRect.x + 8, tileRect.y + 26, 5, 5 }, 74, 66, 57);
				fillRect({ tileRect.x + 36, tileRect.y + 14, 4, 4 }, 205, 174, 104);
			}
			else if (tile == WorldTiles::WatershedGround)
			{
				fillRect({ tileRect.x + 7, tileRect.y + 34, 3, 8 }, 99, 181, 96);
				fillRect({ tileRect.x + 31, tileRect.y + 16, 3, 10 }, 77, 158, 88);
				fillRect({ tileRect.x + 18, tileRect.y + 39, 12, 3 }, 74, 143, 91);
			}
			else if (tile == WorldTiles::WatershedPath)
			{
				fillRect({ tileRect.x + 4, tileRect.y + 7, 18, 12 }, 174, 163, 122);
				fillRect({ tileRect.x + 27, tileRect.y + 28, 15, 10 }, 118, 111, 88);
			}
			else if (tile == WorldTiles::WatershedMarker)
			{
				fillRect({ tileRect.x + 21, tileRect.y + 13, 6, 31 }, 82, 54, 32);
				fillRect({ tileRect.x + 7, tileRect.y + 8, 34, 12 }, 203, 171, 69);
				fillRect({ tileRect.x + 10, tileRect.y + 12, 8, 4 }, 58, 133, 193);
				fillRect({ tileRect.x + 20, tileRect.y + 12, 8, 4 }, 66, 157, 79);
				fillRect({ tileRect.x + 30, tileRect.y + 12, 7, 4 }, 215, 169, 57);
			}
			else if (tile == WorldTiles::GlasswaterGround)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 22, 42, 2 }, 128, 181, 181);
				fillRect({ tileRect.x + 17, tileRect.y + 3, 2, 19 }, 116, 174, 176);
				fillRect({ tileRect.x + 32, tileRect.y + 25, 2, 20 }, 105, 164, 168);
			}
			else if (tile == WorldTiles::GlasswaterPaving)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 2, 44, 44 }, 184, 211, 211);
				fillRect({ tileRect.x + 3, tileRect.y + 22, 42, 2 }, 102, 164, 177);
				fillRect({ tileRect.x + 22, tileRect.y + 3, 2, 42 }, 112, 174, 184);
				fillRect({ tileRect.x + 7, tileRect.y + 7, 9, 3 }, 222, 232, 220);
			}
			else if (tile == WorldTiles::GlasswaterRoof)
			{
				for (int wave = 0; wave < 4; ++wave)
				{
					int waveY = tileRect.y + 7 + wave * 10;
					fillRect({ tileRect.x + (wave % 2 == 0 ? 0 : 7), waveY, 41, 5 },
						49, 117, 159);
					fillRect({ tileRect.x + (wave % 2 == 0 ? 8 : 0), waveY + 4, 40, 3 },
						27, 70, 116);
				}
				fillRect({ tileRect.x, tileRect.y + 43, 48, 4 }, 111, 75, 143);
			}
			else if (tile == WorldTiles::GlasswaterDock)
			{
				for (int plank = 3; plank < 48; plank += 9)
					fillRect({ tileRect.x + plank, tileRect.y + 3, 3, 42 }, 68, 49, 38);
				fillRect({ tileRect.x, tileRect.y + 5, 48, 4 }, 73, 156, 180);
				fillRect({ tileRect.x, tileRect.y + 39, 48, 4 }, 73, 156, 180);
			}
			else if (tile == WorldTiles::GlasswaterWall)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 174, 207, 207);
				for (int course = 10; course < 43; course += 11)
					fillRect({ tileRect.x + 3, tileRect.y + course, 42, 2 }, 98, 151, 163);
				fillRect({ tileRect.x + 7, tileRect.y + 14, 12, 13 }, 47, 118, 157);
				fillRect({ tileRect.x + 29, tileRect.y + 14, 12, 13 }, 47, 118, 157);
			}
			else if (tile == WorldTiles::GlasswaterDoor)
			{
				fillRect({ tileRect.x + 5, tileRect.y + 2, 38, 46 }, 177, 207, 205);
				fillRect({ tileRect.x + 10, tileRect.y + 8, 28, 40 }, 39, 98, 133);
				fillRect({ tileRect.x + 14, tileRect.y + 12, 20, 32 }, 49, 129, 158);
				fillRect({ tileRect.x + 29, tileRect.y + 27, 4, 4 }, 230, 199, 87);
			}
			else if (tile == WorldTiles::GlasswaterArena)
			{
				outlineRect({ tileRect.x + 4, tileRect.y + 4, 40, 40 }, 220, 231, 222, 255, 3);
				fillRect({ tileRect.x + 22, tileRect.y + 6, 4, 36 }, 69, 119, 166);
				fillRect({ tileRect.x + 6, tileRect.y + 22, 36, 4 }, 111, 75, 143);
			}
			else if (tile == WorldTiles::GlasswaterMarker)
			{
				fillRect({ tileRect.x + 21, tileRect.y + 9, 6, 34 }, 49, 74, 91);
				fillRect({ tileRect.x + 10, tileRect.y + 7, 28, 9 }, 211, 222, 205);
				fillRect({ tileRect.x + 13, tileRect.y + 10, 9, 3 }, 52, 142, 184);
				fillRect({ tileRect.x + 26, tileRect.y + 10, 8, 3 }, 82, 171, 132);
			}
			else if (tile == WorldTiles::RootmazeGround)
			{
				fillRect({ tileRect.x + 7, tileRect.y + 33, 4, 10 }, 111, 174, 73);
				fillRect({ tileRect.x + 29, tileRect.y + 19, 3, 13 }, 91, 157, 66);
				fillRect({ tileRect.x + 18, tileRect.y + 39, 12, 3 }, 52, 105, 52);
			}
			else if (tile == WorldTiles::RootmazePath)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 7, 19, 12 }, 151, 146, 91);
				fillRect({ tileRect.x + 26, tileRect.y + 27, 18, 11 }, 89, 103, 66);
				fillRect({ tileRect.x + 16, tileRect.y + 20, 4, 4 }, 190, 167, 83);
			}
			else if (tile == WorldTiles::RootmazeRoot)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 87, 62, 38);
				fillRect({ tileRect.x + 6, tileRect.y + 7, 9, 36 }, 112, 76, 43);
				fillRect({ tileRect.x + 24, tileRect.y + 2, 7, 43 }, 63, 48, 34);
				fillRect({ tileRect.x + 35, tileRect.y + 12, 7, 31 }, 101, 72, 42);
				fillRect({ tileRect.x + 9, tileRect.y + 8, 5, 8 }, 80, 143, 55);
			}
			else if (tile == WorldTiles::RootmazeBridge)
			{
				for (int slat = 2; slat < 48; slat += 8)
					fillRect({ tileRect.x + slat, tileRect.y + 5, 6, 38 }, 151, 108, 61);
				fillRect({ tileRect.x, tileRect.y + 4, 48, 5 }, 61, 95, 47);
				fillRect({ tileRect.x, tileRect.y + 39, 48, 5 }, 61, 95, 47);
				fillRect({ tileRect.x + 4, tileRect.y + 7, 3, 35 }, 112, 169, 70);
			}
			else if (tile == WorldTiles::RootmazeRoof)
			{
				fillRect({ tileRect.x + 1, tileRect.y + 3, 46, 42 }, 78, 124, 54);
				for (int row = 8; row < 42; row += 9)
					fillRect({ tileRect.x + 2, tileRect.y + row, 44, 3 }, 45, 86, 42);
				fillRect({ tileRect.x + 7, tileRect.y + 5, 9, 6 }, 121, 170, 73);
				fillRect({ tileRect.x + 30, tileRect.y + 17, 11, 7 }, 97, 153, 66);
				fillRect({ tileRect.x, tileRect.y + 42, 48, 5 }, 91, 58, 37);
			}
			else if (tile == WorldTiles::RootmazeWall)
			{
				fillRect({ tileRect.x + 2, tileRect.y + 3, 44, 42 }, 122, 87, 52);
				for (int beam = 7; beam < 45; beam += 12)
					fillRect({ tileRect.x + beam, tileRect.y + 3, 5, 42 }, 70, 50, 35);
				fillRect({ tileRect.x + 10, tileRect.y + 13, 12, 13 }, 79, 139, 106);
				fillRect({ tileRect.x + 29, tileRect.y + 13, 10, 13 }, 79, 139, 106);
			}
			else if (tile == WorldTiles::RootmazeDoor)
			{
				fillRect({ tileRect.x + 5, tileRect.y + 2, 38, 46 }, 82, 58, 39);
				fillRect({ tileRect.x + 10, tileRect.y + 8, 28, 40 }, 138, 99, 55);
				fillRect({ tileRect.x + 14, tileRect.y + 13, 20, 31 }, 101, 142, 67);
				fillRect({ tileRect.x + 29, tileRect.y + 27, 4, 4 }, 229, 187, 76);
			}
			else if (tile == WorldTiles::RootmazeArena)
			{
				outlineRect({ tileRect.x + 4, tileRect.y + 4, 40, 40 }, 213, 210, 136, 255, 3);
				fillRect({ tileRect.x + 7, tileRect.y + 22, 34, 4 }, 53, 111, 67);
				fillRect({ tileRect.x + 22, tileRect.y + 7, 4, 34 }, 78, 127, 60);
			}
			else if (tile == WorldTiles::RootmazeMarker)
			{
				fillRect({ tileRect.x + 21, tileRect.y + 8, 6, 35 }, 91, 61, 37);
				fillRect({ tileRect.x + 7, tileRect.y + 7, 34, 11 }, 147, 126, 65);
				fillRect({ tileRect.x + 10, tileRect.y + 10, 7, 4 }, 62, 146, 77);
				fillRect({ tileRect.x + 20, tileRect.y + 10, 7, 4 }, 52, 124, 181);
				fillRect({ tileRect.x + 30, tileRect.y + 10, 7, 4 }, 202, 151, 63);
			}
			else if (tile == WorldTiles::BlackstoneGround)
			{
				fillRect({ tileRect.x + 7, tileRect.y + 11, 6, 4 }, 133, 126, 107);
				fillRect({ tileRect.x + 31, tileRect.y + 34, 8, 4 }, 42, 41, 39);
			}
			else if (tile == WorldTiles::BlackstonePath)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 6, 19, 13 }, 142, 136, 121);
				fillRect({ tileRect.x + 27, tileRect.y + 28, 17, 11 }, 77, 75, 70);
				fillRect({ tileRect.x + 20, tileRect.y + 20, 4, 4 }, 190, 151, 60);
			}
			else if (tile == WorldTiles::BlackstoneWall)
			{
				for (int course = 9; course < 45; course += 11)
					fillRect({ tileRect.x + 2, tileRect.y + course, 44, 3 }, 20, 20, 22);
				fillRect({ tileRect.x + 35, tileRect.y + 7, 6, 6 }, 192, 149, 52);
			}
			else if (tile == WorldTiles::BlackstoneGate)
			{
				fillRect({ tileRect.x + 3, tileRect.y + 2, 7, 44 }, 27, 27, 29);
				fillRect({ tileRect.x + 38, tileRect.y + 2, 7, 44 }, 27, 27, 29);
				if (!hasCrest("confluence"))
					for (int bar = 13; bar < 38; bar += 8)
						fillRect({ tileRect.x + bar, tileRect.y + 4, 4, 40 }, 202, 158, 57);
				else
					fillRect({ tileRect.x + 13, tileRect.y + 4, 24, 4 }, 202, 158, 57);
			}
			mWorldTileRenderer->drawDecorationTile(tile, tileRect);
		}
	}
	if (cinderrailRegion != NULL)
	{
		if (visibleTiles.intersects(cinderrailRegion->x + 8, cinderrailRegion->y + 1,
			cinderrailRegion->x + 11, cinderrailRegion->y + 4))
		{
			int stationX = mapX + (cinderrailRegion->x + 8) * TILE;
			int stationY = mapY + (cinderrailRegion->y + 1) * TILE;
			fillRect({ stationX + 8, stationY, 80, 102 }, 116, 50, 39);
			fillRect({ stationX + 3, stationY, 90, 9 }, 55, 54, 56);
			fillRect({ stationX + 28, stationY + 17, 39, 39 }, 44, 42, 43);
			fillRect({ stationX + 33, stationY + 22, 29, 29 }, 223, 207, 159);
			fillRect({ stationX + 46, stationY + 25, 3, 13 }, 61, 56, 52);
			fillRect({ stationX + 47, stationY + 36, 10, 3 }, 61, 56, 52);
		}
	}

	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		if (!npcVisible((int)i) || mNpcs[i].mapId != currentMapId()) continue;
		if (!visibleTiles.contains((int)std::floor(mNpcs[i].visualX),
			(int)std::floor(mNpcs[i].visualY))) continue;
		drawCharacter(mNpcs[i].visualX, mNpcs[i].visualY, mNpcs[i].appearance,
			mNpcs[i].isComplete(), mNpcs[i].isMoving(),
			mNpcs[i].facingX, mNpcs[i].facingY);
		bool trainerChallenge = mRouteChallengeNpc == (int)i;
		if (npcHasStoryMarker((int)i) || trainerChallenge)
		{
			int markerX = mapX + (int)std::round(mNpcs[i].visualX * TILE) + 17;
			int markerY = mapY + (int)std::round(mNpcs[i].visualY * TILE) - 20;
			fillRect({ markerX - 4, markerY - 2, 20, 22 },
				trainerChallenge ? 75 : 31, trainerChallenge ? 18 : 24, 14, 235);
			outlineRect({ markerX - 4, markerY - 2, 20, 22 },
				trainerChallenge ? 255 : 246, trainerChallenge ? 88 : 203,
				trainerChallenge ? 68 : 78, 255, 2);
			drawText("!", markerX + 2, markerY,
				trainerChallenge ? color(255, 151, 92) : color(255, 225, 111), 16);
		}
	}
	for (size_t i = 0; i < mWorldObjects.size(); ++i)
	{
		const WorldObject& object = mWorldObjects[i];
		if (object.mapId != currentMapId() || !visibleTiles.contains(object.x, object.y))
			continue;
		int x = mapX + object.x * TILE;
		int y = mapY + object.y * TILE;
		if (object.kind == WorldObjectKind::Signpost)
		{
			fillRect({ x + 21, y + 17, 6, 29 }, 77, 48, 29, 255);
			fillRect({ x + 6, y + 7, 36, 18 }, 166, 111, 50, 255);
			fillRect({ x + 9, y + 10, 30, 3 }, 213, 159, 76, 255);
			outlineRect({ x + 6, y + 7, 36, 18 }, 62, 38, 24, 255, 2);
			fillRect({ x + 14, y + 16, 20, 3 }, 85, 55, 30, 255);
		}
		else
		{
			bool opened = mOpenedWorldObjects.count(object.id) != 0;
			fillRect({ x + 7, y + 23, 34, 20 }, 111, 65, 31, 255);
			outlineRect({ x + 7, y + 23, 34, 20 }, 50, 31, 22, 255, 2);
			fillRect({ x + 10, y + 26, 28, 4 }, opened ? 38 : 151,
				opened ? 30 : 92, opened ? 28 : 42, 255);
			if (opened)
			{
				fillRect({ x + 8, y + 12, 32, 6 }, 105, 61, 31, 255);
				fillRect({ x + 10, y + 8, 28, 5 }, 146, 88, 40, 255);
				outlineRect({ x + 8, y + 8, 32, 10 }, 53, 34, 23, 255, 2);
			}
			else
			{
				fillRect({ x + 7, y + 16, 34, 10 }, 151, 88, 38, 255);
				outlineRect({ x + 7, y + 16, 34, 10 }, 50, 31, 22, 255, 2);
			}
			fillRect({ x + 21, y + 25, 7, 10 }, 218, 171, 62, 255);
			fillRect({ x + 23, y + 27, 3, 4 }, 255, 222, 116, 255);
		}
	}
	for (size_t i = 0; i < mMercerStock.shards.size(); ++i)
	{
		const MercerShard& shard = mMercerStock.shards[i];
		if (shard.mapId != currentMapId() || mCollectedShards.count(shard.id)) continue;
		if (!visibleTiles.contains(shard.x, shard.y)) continue;
		int x = mapX + shard.x * TILE;
		int y = mapY + shard.y * TILE;
		int shimmer = (int)((SDL_GetTicks() / 180 + i * 3) % 5);
		fillRect({ x + 19, y + 10 - shimmer / 2, 12, 28 }, 42, 24, 65, 220);
		fillRect({ x + 14, y + 15 - shimmer / 2, 22, 18 }, 139, 82, 204, 245);
		fillRect({ x + 19, y + 19 - shimmer / 2, 12, 10 }, 225, 184, 255, 255);
	}
	bool playerWalking = std::fabs(mPlayerX - mVisualX) > 0.001f ||
		std::fabs(mPlayerY - mVisualY) > 0.001f;
	drawCharacter(mVisualX, mVisualY, CharacterAppearance::Player, false, playerWalking,
		mFacingX, mFacingY);
	SDL_RenderSetClipRect(mRenderer, NULL);

	renderStoryTracker();
	renderRegionBanner();

	if (!mNotice.empty() && SDL_GetTicks() < mNoticeUntil)
	{
		fillRect({ 36, 10, 1208, 42 }, 17, 28, 43, 230);
		drawText(mNotice, 48, 19, color(113, 232, 143), 16, 1178);
	}

	if (mDialogueNpc >= 0 || mDialogueObject >= 0)
	{
		fillRect({ 40, 646, 1200, 128 }, 16, 22, 36, 248);
		outlineRect({ 40, 646, 1200, 128 }, 194, 148, 62, 255, 3);
		const std::string speaker = mDialogueObject >= 0 ?
			mWorldObjects[mDialogueObject].name : mNpcs[mDialogueNpc].name;
		drawText(speaker, 68, 664, color(244, 206, 103), 25);
		std::string dialogue = mDialogueText.substr(0, mDialogueVisibleBytes);
		bool fullyRevealed = mDialogueVisibleBytes >= mDialogueText.size();
		std::string prompt = "E / Click: continue";
		if (mDialogueObject >= 0)
			prompt = "E / Click: close";
		else if (mDialogueAction == DialogueAction::OpenNpcMenu)
			prompt = "E / Click: choices";
		else if (mDialogueAction == DialogueAction::ReturnToNpcMenu)
			prompt = "E / Click: back to choices";
		else if (mDialogueAction == DialogueAction::ForcedBattle)
			prompt = "Trainer challenge  •  E / Click: battle";
		else if (mDialogueAction == DialogueAction::NpcInteraction &&
			mNpcs[mDialogueNpc].isComplete())
		{
			prompt = "E / Click: close";
		}
		else if (mDialogueAction == DialogueAction::NpcInteraction &&
			mNpcs[mDialogueNpc].isDuelist())
			prompt = mNpcs[mDialogueNpc].rankName() + "  •  E / Click: battle";
		drawText(dialogue, 68, 704, color(232, 237, 246), 19, 1080);
		if (fullyRevealed)
		{
			drawText(prompt, 850, 668, color(126, 176, 242), 15, 340);
			int arrowOffset = (SDL_GetTicks() / 220) % 2 == 0 ? 0 : 3;
			drawText("▼", 1191, 739 + arrowOffset, color(244, 206, 103), 17);
		}
	}
	if (mDialogueNpc < 0 && mDialogueObject < 0) renderStoryScene();
	if (mNpcMenuNpc >= 0) renderNpcMenu();
	if (mPauseMenuOpen) renderPauseMenu();
}

void Application::drawCharacter(float gridX, float gridY, CharacterAppearance appearance,
	bool completed, bool walking, int facingX, int facingY)
{
	const std::vector<std::string>& map = currentMap();
	const bool worldBuilder = mScreen == Screen::WorldBuilder;
	float cameraX = worldBuilder ? (float)mWorldBuilderCameraX : overworldCameraX();
	float cameraY = worldBuilder ? (float)mWorldBuilderCameraY : overworldCameraY();
	int x;
	int y;
	if (worldBuilder)
	{
		int baseX = MAP_X + std::max(0,
			MAP_VIEW_WIDTH - (int)map[0].size() * mWorldBuilderTileSize) / 2;
		int baseY = MAP_Y + std::max(0,
			MAP_VIEW_HEIGHT - (int)map.size() * mWorldBuilderTileSize) / 2;
		x = baseX + (int)std::round((gridX - cameraX) * mWorldBuilderTileSize) +
			(mWorldBuilderTileSize - TILE) / 2;
		y = baseY + (int)std::round((gridY - cameraY) * mWorldBuilderTileSize) +
			(mWorldBuilderTileSize - TILE) / 2;
	}
	else
	{
		x = mapOriginX((int)map[0].size(), OVERWORLD_VIEW_COLUMNS) +
			(int)std::round((gridX - cameraX) * TILE);
		y = mapOriginY((int)map.size()) +
			(int)std::round((gridY - cameraY) * TILE);
	}
	drawCharacterSprite(x, y, appearance, completed, walking, facingX, facingY);
}

void Application::drawCharacterSprite(int x, int y, CharacterAppearance appearance,
	bool completed, bool walking, int facingX, int facingY)
{
	int stride = walking && (SDL_GetTicks() / 110) % 2 == 0 ? 2 : (walking ? -2 : 0);
	int bob = walking && (SDL_GetTicks() / 110) % 2 == 0 ? -1 : 0;
	fillRect({ x + 9, y + 39, 31, 6 }, 8, 14, 18, 100);
	CharacterSpriteDefinition sprite = characterSprite(appearance);
	SDL_Rect spriteDestination = worldBuilderTileRect({ x, y, TILE, TILE });
	if (mSpriteSheets != NULL && mSpriteSheets->drawCharacter(sprite, facingX, facingY,
		walking, SDL_GetTicks(), spriteDestination))
	{
		if (completed)
		{
			fillRect({ x + 34, y + 17, 8, 10 }, 33, 39, 48);
			fillRect({ x + 36, y + 19, 4, 4 }, 220, 193, 92);
			fillRect({ x + 37, y + 23, 2, 3 }, 151, 124, 55);
		}
		return;
	}
	y += bob;

	const int appearanceValue = static_cast<int>(appearance);
	const int genericMaleFirst = static_cast<int>(CharacterAppearance::GenericMale1);
	const int genericFemaleFirst = static_cast<int>(CharacterAppearance::GenericFemale1);
	const bool genericMale = appearanceValue >= genericMaleFirst &&
		appearanceValue <= static_cast<int>(CharacterAppearance::GenericMale10);
	const bool genericFemale = appearanceValue >= genericFemaleFirst &&
		appearanceValue <= static_cast<int>(CharacterAppearance::GenericFemale10);
	const int genericVariant = genericMale ? appearanceValue - genericMaleFirst :
		(genericFemale ? appearanceValue - genericFemaleFirst : -1);
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
	else if (appearance == CharacterAppearance::Neris)
		trousers = color(28, 47, 78);
	else if (appearance == CharacterAppearance::Oren)
		trousers = color(38, 71, 61);
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
	auto forwardFace = [&block, skinShadow](SDL_Color eyes)
	{
		block(21, 13, 2, 2, eyes);
		block(29, 13, 2, 2, eyes);
		block(25, 15, 2, 2, skinShadow);
		block(24, 18, 5, 1, color(125, 76, 67));
	};

	switch (appearance)
	{
	case CharacterAppearance::Player:
		block(12, 18, 25, 21, outline);
		block(14, 20, 21, 17, color(31, 88, 185));
		block(14, 30, 21, 4, color(22, 54, 111));
		block(18, 7, 15, 15, skin);
		block(15, 4, 21, 8, color(91, 48, 22));
		block(15, 10, 4, 9, color(91, 48, 22));
		forwardFace(color(38, 31, 27));
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
		forwardFace(color(57, 28, 52));
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
		forwardFace(color(24, 50, 62));
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
		forwardFace(color(45, 34, 26));
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
		forwardFace(color(71, 58, 37));
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
		forwardFace(color(71, 43, 36));
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
		forwardFace(color(24, 40, 44));
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
		forwardFace(color(44, 40, 25));
		block(23, 28, 5, 6, color(157, 110, 48));
		break;

	case CharacterAppearance::Neris:
		block(10, 18, 30, 22, outline);
		block(12, 20, 26, 20, color(30, 64, 112));
		block(9, 20, 8, 8, color(66, 42, 91));
		block(35, 20, 7, 8, color(66, 42, 91));
		block(18, 7, 15, 15, skin);
		block(14, 4, 23, 7, color(33, 45, 71));
		block(14, 9, 5, 12, color(33, 45, 71));
		block(16, 29, 21, 4, color(111, 75, 143));
		block(34, 1, 3, 13, color(235, 231, 207));
		block(37, 0, 3, 8, color(204, 222, 223));
		forwardFace(color(29, 42, 55));
		block(8, 28, 5, 11, color(210, 216, 208));
		break;

	case CharacterAppearance::Oren:
		block(9, 18, 32, 22, outline);
		block(11, 20, 28, 20, color(49, 112, 73));
		block(9, 19, 9, 18, color(74, 145, 68));
		block(34, 19, 8, 18, color(54, 128, 61));
		block(18, 7, 15, 15, skin);
		block(14, 4, 23, 7, color(68, 86, 45));
		block(14, 9, 5, 14, color(68, 86, 45));
		block(33, 8, 5, 14, color(68, 86, 45));
		block(12, 16, 7, 6, color(119, 171, 68));
		block(33, 17, 8, 6, color(102, 161, 62));
		block(14, 29, 24, 4, color(37, 91, 132));
		block(41, 6, 4, 35, color(104, 72, 41));
		block(38, 3, 10, 8, color(91, 154, 66));
		forwardFace(color(39, 48, 33));
		break;

	case CharacterAppearance::GenericMale1:
	case CharacterAppearance::GenericMale2:
	case CharacterAppearance::GenericMale3:
	case CharacterAppearance::GenericMale4:
	case CharacterAppearance::GenericMale5:
	case CharacterAppearance::GenericMale6:
	case CharacterAppearance::GenericMale7:
	case CharacterAppearance::GenericMale8:
	case CharacterAppearance::GenericMale9:
	case CharacterAppearance::GenericMale10:
	case CharacterAppearance::GenericFemale1:
	case CharacterAppearance::GenericFemale2:
	case CharacterAppearance::GenericFemale3:
	case CharacterAppearance::GenericFemale4:
	case CharacterAppearance::GenericFemale5:
	case CharacterAppearance::GenericFemale6:
	case CharacterAppearance::GenericFemale7:
	case CharacterAppearance::GenericFemale8:
	case CharacterAppearance::GenericFemale9:
	case CharacterAppearance::GenericFemale10:
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
		const bool broad = genericMale &&
			(genericVariant == 2 || genericVariant == 6 || genericVariant == 7);
		if (genericFemale)
		{
			block(13, 7, 5, 18, hair);
			block(33, 7, 5, 18, hair);
		}
		block(broad ? 9 : 11, 18, broad ? 32 : 28, 22, outline);
		block(broad ? 11 : 13, 20, broad ? 28 : 24, 19, outfit);
		block(broad ? 8 : 10, 21, 7, 8, broad && genericVariant == 6 ? genericSkin : outfit);
		block(broad ? 36 : 35, 21, 7, 8, broad && genericVariant == 6 ? genericSkin : outfit);
		block(14, 29, 23, 4, accent);
		block(18, 7, 15, 15, genericSkin);
		forwardFace(color(43, 35, 31));

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
		forwardFace(color(72, 48, 32));
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
