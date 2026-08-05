#include "Application.h"

#include "AppSupport.h"

#include <algorithm>

using namespace AppSupport;

namespace
{
	const int CLUE_AURELIA = 1;
	const int CLUE_FLINT = 2;
	const int CLUE_MIRA = 4;
	const int ALL_CLUES = CLUE_AURELIA | CLUE_FLINT | CLUE_MIRA;
}

void Application::initializeStory()
{
	if (mStoryStage == 0)
	{
		mStoryScene = StoryScene::Intro;
		mStoryScenePage = 0;
	}
	else
		updateStoryProgress();
}

bool Application::handleStoryEvent(const SDL_Event& event)
{
	if (mStoryScene == StoryScene::None) return false;
	if (event.type != SDL_KEYDOWN || event.key.repeat) return false;
	SDL_Keycode key = event.key.keysym.sym;
	if (key != SDLK_e && key != SDLK_SPACE && key != SDLK_RETURN) return true;

	int pageCount = mStoryScene == StoryScene::Intro ? 3 : 2;
	if (++mStoryScenePage < pageCount) return true;

	StoryScene completed = mStoryScene;
	mStoryScene = StoryScene::None;
	mStoryScenePage = 0;
	if (completed == StoryScene::Intro)
	{
		mStoryStage = 1;
		mNotice = "Act I: speak with Aurelia, Flint, and Mira.";
		mNoticeUntil = SDL_GetTicks() + 6000;
		updateStoryProgress();
	}
	savePlayerProgress();
	return true;
}

void Application::discoverStoryClue(int npcIndex)
{
	if (mStoryStage != 1 || npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return;
	int clue = 0;
	if (mNpcs[npcIndex].id == "aurelia") clue = CLUE_AURELIA;
	else if (mNpcs[npcIndex].id == "flint") clue = CLUE_FLINT;
	else if (mNpcs[npcIndex].id == "mira") clue = CLUE_MIRA;
	if (clue == 0 || (mStoryClues & clue) != 0) return;
	mStoryClues |= clue;
	mNotice = "New clue discovered (" + std::to_string(
		((mStoryClues & CLUE_AURELIA) != 0) + ((mStoryClues & CLUE_FLINT) != 0) +
		((mStoryClues & CLUE_MIRA) != 0)) + "/3).";
	mNoticeUntil = SDL_GetTicks() + 4500;
	updateStoryProgress();
	savePlayerProgress();
}

void Application::updateStoryProgress()
{
	if (mStoryStage == 1 && (mStoryClues & ALL_CLUES) == ALL_CLUES)
	{
		mStoryStage = 2;
		mNotice = "The clues agree: stabilize three signature echoes through dueling.";
		mNoticeUntil = SDL_GetTicks() + 6500;
	}
	if (mStoryStage == 2)
	{
		int stabilized = 0;
		for (size_t i = 0; i < mNpcs.size(); ++i)
			if (mNpcs[i].isTownNpc() && mNpcs[i].isDuelist() && mNpcs[i].wins > 0)
				++stabilized;
		if (stabilized >= 3)
		{
			mStoryStage = 3;
			mStoryScene = StoryScene::BossReveal;
			mStoryScenePage = 0;
		}
	}
	if (mStoryStage == 3)
	{
		for (size_t i = 0; i < mNpcs.size(); ++i)
		{
			if (!mNpcs[i].isBoss() || !mNpcs[i].isComplete()) continue;
			mStoryStage = 4;
			mStoryScene = StoryScene::ActComplete;
			mStoryScenePage = 0;
			break;
		}
	}
}

bool Application::npcVisible(int npcIndex) const
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return false;
	return !mNpcs[npcIndex].isBoss() || mStoryStage >= 3;
}

bool Application::npcHasStoryMarker(int npcIndex) const
{
	if (!npcVisible(npcIndex)) return false;
	const Npc& npc = mNpcs[npcIndex];
	if (mStoryStage == 1)
	{
		if (npc.id == "aurelia") return (mStoryClues & CLUE_AURELIA) == 0;
		if (npc.id == "flint") return (mStoryClues & CLUE_FLINT) == 0;
		if (npc.id == "mira") return (mStoryClues & CLUE_MIRA) == 0;
	}
	if (mStoryStage == 2) return npc.isTownNpc() && npc.isDuelist() && npc.wins == 0;
	if (mStoryStage == 3) return npc.isBoss() && !npc.isComplete();
	return false;
}

std::string Application::storyObjective() const
{
	if (mStoryStage == 0) return "Festival of Five Civilizations";
	if (mStoryStage == 1)
	{
		int clues = ((mStoryClues & CLUE_AURELIA) != 0) + ((mStoryClues & CLUE_FLINT) != 0) +
			((mStoryClues & CLUE_MIRA) != 0);
		return "Investigate the fading: speak with Aurelia, Flint, and Mira (" +
			std::to_string(clues) + "/3)";
	}
	if (mStoryStage == 2)
	{
		int stabilized = 0;
		for (size_t i = 0; i < mNpcs.size(); ++i)
			if (mNpcs[i].isTownNpc() && mNpcs[i].isDuelist() && mNpcs[i].wins > 0)
				++stabilized;
		return "Stabilize signature echoes by defeating three duelists (" +
			std::to_string(std::min(3, stabilized)) + "/3)";
	}
	if (mStoryStage == 3) return "Confront the Veiled One at the central bridge";
	return "Act I complete: prepare for the road beyond Emberglen";
}

std::string Application::storyDialogueForNpc(int npcIndex) const
{
	if (npcIndex < 0 || npcIndex >= (int)mNpcs.size()) return "";
	const Npc& npc = mNpcs[npcIndex];
	if (npc.canTrade() && !npc.isDuelist())
	{
		if (mStoryStage >= 4) return npc.dialogueText("act_complete", npc.challenge);
		return mStoryStage < 2 ? npc.dialogueText("shop_early", npc.challenge) :
			npc.dialogueText("shop_late", npc.challenge);
	}
	if (npc.isBoss())
		return npc.isComplete() ?
			npc.dialogueText("complete", npc.challenge) : npc.dialogueText("greeting", npc.challenge);
	if (npc.isRouteDuelist())
		return npc.isComplete() ? npc.dialogueText("complete", npc.challenge) :
			npc.dialogueText("greeting", npc.challenge);
	if (npc.isComplete())
		return npc.dialogueText("complete",
			"My signature card remembers you now. I have nothing more to wager.");

	if (mStoryStage <= 2 && !npc.dialogueText("clue").empty())
		return npc.dialogueText("clue");
	if (mStoryStage == 1)
		return npc.dialogueText("investigation",
			"Something is wrong with the town's cards. Aurelia, Flint, and Mira each saw part of what happened.");
	if (mStoryStage == 2)
		return npc.wins == 0 ?
			npc.dialogueText("stabilize_before",
				"A true duel may stabilize my fading signature card. Show me what your deck can do.") :
			npc.dialogueText("stabilize_after",
				"That echo is stable. Win against other duelists so we can trace the Curator's signal.");
	if (mStoryStage == 3)
		return npc.dialogueText("boss_reveal",
			"The masked stranger has appeared at the central bridge. We'll hold Emberglen—go!");
	if (npc.isTownNpc())
	{
		std::string everydayTalk = npc.dialogueText("talk",
			npc.dialogueText("greeting", npc.challenge));
		return mStoryStage >= 4 ? npc.dialogueText("act_complete", everydayTalk) :
			everydayTalk;
	}
	return npc.dialogueText("act_complete",
		"You restored our cards and drove off the Curator's agent. The old road is waiting for you.");
}

void Application::renderStoryTracker()
{
	fillRect({ 36, 9, 1208, 43 }, 16, 27, 43, 238);
	outlineRect({ 36, 9, 1208, 43 }, 83, 121, 174, 255, 2);
	drawText("ACT I  •  " + storyObjective(), 50, 20, color(199, 216, 240), 15, 1178);
}

void Application::renderStoryScene()
{
	if (mStoryScene == StoryScene::None) return;
	fillRect({ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT }, 3, 6, 12, 205);
	fillRect({ 165, 142, 950, 500 }, 15, 22, 37, 252);
	outlineRect({ 165, 142, 950, 500 }, 211, 163, 67, 255, 4);

	std::string title;
	std::string body;
	if (mStoryScene == StoryScene::Intro)
	{
		title = mStoryScenePage == 0 ? "THE FESTIVAL OF FIVE" :
			(mStoryScenePage == 1 ? "THE FADING" : "ROWAN'S LAST ECHO");
		if (mStoryScenePage == 0)
			body = "Emberglen gathered to celebrate the five civilizations. Your mentor Rowan promised that tonight's exhibition duel would begin your journey as a true duelist.";
		else if (mStoryScenePage == 1)
			body = "Before the first card was played, the arena lights died. Names and rules vanished from hundreds of cards, leaving only pale fragments and a broken-circle mark.";
		else
			body = "Rowan ran toward the northern bridge and disappeared. One message remained inside your deck: 'Win their trust. Restore the echoes. Do not let the Curator finish the Hollow Deck.'";
	}
	else if (mStoryScene == StoryScene::BossReveal)
	{
		title = mStoryScenePage == 0 ? "THE SIGNAL" : "A MASK AT THE BRIDGE";
		body = mStoryScenePage == 0 ?
			"Three restored signature echoes resonate with the blank fragments. Their combined light draws a path across Emberglen toward the central bridge." :
			"A masked duelist steps from the distortion, carrying a deck with every civilization mark scratched away. 'The Curator wants those echoes returned.'";
	}
	else
	{
		title = mStoryScenePage == 0 ? "THE VEIL BREAKS" : "ACT I COMPLETE";
		body = mStoryScenePage == 0 ?
			"The masked deck scatters into blank fragments. Beneath the veil is only an enchanted shell—but its final message is spoken in the Curator's voice." :
			"'Rowan lives beyond the old road. Bring every echo you can restore.' The road out of Emberglen opens, and the duelists you helped promise to guard the town.";
	}
	drawText(title, 215, 190, color(245, 207, 106), 34, 850);
	drawText(body, 220, 285, color(229, 235, 245), 22, 835);
	drawText("E / Space / Enter: continue", 780, 585, color(126, 176, 242), 16);
}
