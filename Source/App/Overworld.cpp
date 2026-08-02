#include "Application.h"

#include "AppSupport.h"

#include <algorithm>

using namespace AppSupport;

void Application::handleOverworldEvent(const SDL_Event& event)
{
	if (event.type != SDL_KEYDOWN || event.key.repeat)
		return;
	SDL_Keycode key = event.key.keysym.sym;
	if (key == SDLK_ESCAPE)
	{
		if (mDialogueNpc >= 0) mDialogueNpc = -1;
		else mRunning = false;
		return;
	}
	if (mDialogueNpc >= 0)
	{
		if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN) interact();
		return;
	}
	if (key == SDLK_w || key == SDLK_UP) tryMove(0, -1);
	else if (key == SDLK_s || key == SDLK_DOWN) tryMove(0, 1);
	else if (key == SDLK_a || key == SDLK_LEFT) tryMove(-1, 0);
	else if (key == SDLK_d || key == SDLK_RIGHT) tryMove(1, 0);
	else if (key == SDLK_e || key == SDLK_SPACE || key == SDLK_RETURN) interact();
}

void Application::updateOverworld(Uint32 deltaTime)
{
	float amount = std::min(1.f, deltaTime / 85.f);
	mVisualX += (mPlayerX - mVisualX) * amount;
	mVisualY += (mPlayerY - mVisualY) * amount;
}

bool Application::isWalkable(int x, int y) const
{
	if (y < 0 || y >= (int)mMap.size() || x < 0 || x >= (int)mMap[y].size()) return false;
	char tile = mMap[y][x];
	return tile == '.' || tile == '=';
}

int Application::npcAt(int x, int y) const
{
	for (size_t i = 0; i < mNpcs.size(); ++i)
		if (mNpcs[i].x == x && mNpcs[i].y == y) return (int)i;
	return -1;
}

void Application::tryMove(int dx, int dy)
{
	mFacingX = dx;
	mFacingY = dy;
	int x = mPlayerX + dx;
	int y = mPlayerY + dy;
	if (isWalkable(x, y) && npcAt(x, y) < 0)
	{
		mPlayerX = x;
		mPlayerY = y;
	}
}

void Application::interact()
{
	if (mDialogueNpc >= 0)
	{
		if (mNpcs[mDialogueNpc].defeated)
			mDialogueNpc = -1;
		else
			startDuel(mDialogueNpc);
		return;
	}
	mDialogueNpc = npcAt(mPlayerX + mFacingX, mPlayerY + mFacingY);
}

void Application::renderOverworld()
{
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
		drawCharacter(mNpcs[i].x, mNpcs[i].y, true, mNpcs[i].defeated);
	drawCharacter((int)std::round(mVisualX), (int)std::round(mVisualY), false, false);

	fillRect({ 1012, 54, 238, 576 }, 21, 28, 45, 245);
	outlineRect({ 1012, 54, 238, 576 }, 190, 146, 61, 255, 2);
	drawText("EMBERGLEN", 1034, 76, color(242, 205, 99), 28);
	drawText("DUELISTS", 1034, 139, color(135, 162, 199), 16);
	for (size_t i = 0; i < mNpcs.size(); ++i)
	{
		drawText(mNpcs[i].name, 1034, 177 + (int)i * 76, color(235, 238, 245), 21);
		drawText(mNpcs[i].defeated ? "DEFEATED" : "READY", 1034, 205 + (int)i * 76,
			mNpcs[i].defeated ? color(92, 208, 121) : color(235, 151, 65), 15);
	}
	drawText("WASD / Arrows: move", 1034, 474, color(187, 200, 221), 15);
	drawText("E / Space: talk", 1034, 502, color(187, 200, 221), 15);
	drawText("Esc: back / quit", 1034, 530, color(187, 200, 221), 15);

	if (!mNotice.empty() && SDL_GetTicks() < mNoticeUntil)
	{
		fillRect({ 36, 10, 640, 38 }, 17, 28, 43, 230);
		drawText(mNotice, 48, 18, color(113, 232, 143), 18);
	}

	if (mDialogueNpc >= 0)
	{
		const Npc& npc = mNpcs[mDialogueNpc];
		fillRect({ 40, 646, 1200, 128 }, 16, 22, 36, 248);
		outlineRect({ 40, 646, 1200, 128 }, 194, 148, 62, 255, 3);
		drawText(npc.name, 68, 664, color(244, 206, 103), 25);
		drawText(npc.defeated ? "A fine duel. Keep exploring, duelist." : npc.challenge,
			68, 704, color(232, 237, 246), 19, 1080);
		drawText(npc.defeated ? "E / Space to close" : "E / Space to battle",
			965, 742, color(126, 176, 242), 15);
	}
}

void Application::drawCharacter(int gridX, int gridY, bool rival, bool defeated)
{
	int x = MAP_X + gridX * TILE;
	int y = MAP_Y + gridY * TILE;
	fillRect({ x + 11, y + 39, 28, 6 }, 8, 14, 18, 100);
	if (rival)
		fillRect({ x + 13, y + 20, 22, 22 }, defeated ? 91 : 111, defeated ? 94 : 46, defeated ? 103 : 143);
	else
		fillRect({ x + 13, y + 20, 22, 22 }, 31, 88, 185);
	fillRect({ x + 17, y + 8, 15, 15 }, 224, 172, 126);
	fillRect({ x + 14, y + 5, 21, 8 }, rival ? 41 : 91, rival ? 24 : 48, rival ? 58 : 22);
}

