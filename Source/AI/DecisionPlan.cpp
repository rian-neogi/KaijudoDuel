#include "DecisionPlan.h"

#include "AiParams.h"
#include "HeuristicBot.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace
{
	std::string messageType(const Message& message)
	{
		std::map<std::string, std::string>::const_iterator type = message.map.find("msgtype");
		return type == message.map.end() ? "" : type->second;
	}

	int messageInt(const Message& message, const char* key, int fallback = -1)
	{
		std::map<std::string, std::string>::const_iterator value = message.map.find(key);
		return value == message.map.end() ? fallback : std::atoi(value->second.c_str());
	}

	bool containsMessage(const std::vector<Message>& messages, const Message& expected)
	{
		for (std::vector<Message>::const_iterator message = messages.begin();
			message != messages.end(); ++message)
		{
			if (message->map == expected.map)
				return true;
		}
		return false;
	}

	bool containsManaMove(const std::vector<Message>& moves, int card)
	{
		for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
		{
			if (messageType(*move) == "manatap" && messageInt(*move, "card") == card)
				return true;
		}
		return false;
	}

	std::vector<int> manaOptions(Duel& duel)
	{
		std::vector<int> result;
		std::vector<Message> moves = duel.getPossibleMoves();
		for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
		{
			if (messageType(*move) == "manatap")
				result.push_back(messageInt(*move, "card"));
		}
		return result;
	}

	class ResolverScope
	{
	public:
		ResolverScope(Duel& duel, const Duel::ChoiceResolver& resolver)
			: mDuel(duel), mPrevious(duel.mChoiceResolver),
			  mPreviousAnswersRemaining(duel.mChoiceResolverAnswersRemaining)
		{
			mDuel.setChoiceResolver(resolver);
		}

		~ResolverScope()
		{
			mDuel.setChoiceResolver(mPrevious, mPreviousAnswersRemaining);
		}

	private:
		Duel& mDuel;
		Duel::ChoiceResolver mPrevious;
		int mPreviousAnswersRemaining;
	};

	struct PlannedChoiceContext
	{
		std::vector<DecisionChoice> choices;
		size_t next;

		PlannedChoiceContext() : next(0)
		{
		}
	};

	bool shouldStop(const DecisionPlanEnumerationOptions& options)
	{
		return options.shouldStop && options.shouldStop();
	}

	void enumeratePlan(Duel& root, const DecisionPlan& prefix, std::vector<DecisionPlan>& plans,
		const DecisionPlanEnumerationOptions& options)
	{
		if (shouldStop(options)) return;
		Duel simulation;
		simulation.mIsSimulation = true;
		simulation.mInputLoopRunning = false;
		if (!simulation.copyFrom(root))
			return;

		DecisionPlanResult result = executeDecisionPlan(simulation, prefix);
		if (result.status == DecisionPlanStatus::Complete)
		{
			plans.push_back(prefix);
			return;
		}
		if (result.status == DecisionPlanStatus::NeedsMana)
		{
			if (options.heuristicMana)
			{
				HeuristicBot bot(prefix.player, options.personality);
				int option = bot.chooseManaPayment(simulation, result.options);
				if (option >= 0)
				{
					DecisionPlan child = prefix;
					child.manaCards.push_back(option);
					enumeratePlan(root, child, plans, options);
				}
				return;
			}
			int previousMana = prefix.manaCards.empty() ? -1 : prefix.manaCards.back();
			for (std::vector<int>::const_iterator option = result.options.begin();
				option != result.options.end(); ++option)
			{
				// Mana order has no rule meaning. Ascending IDs produce every legal
				// payment set once instead of producing every permutation of that set.
				if (*option <= previousMana)
					continue;
				DecisionPlan child = prefix;
				child.manaCards.push_back(*option);
				enumeratePlan(root, child, plans, options);
			}
			return;
		}
		if (result.status == DecisionPlanStatus::NeedsChoice)
		{
			if (options.heuristicChoices && result.aiPreferredChoice != RETURN_NOTHING &&
				std::find(result.options.begin(), result.options.end(), result.aiPreferredChoice) !=
					result.options.end())
			{
				DecisionPlan child = prefix;
				child.choices.push_back(DecisionChoice(result.choicePlayer,
					result.aiPreferredChoice));
				enumeratePlan(root, child, plans, options);
				return;
			}
			if (options.randomChoices && !result.options.empty())
			{
				size_t selected = options.randomIndex ?
					options.randomIndex(result.options.size()) : 0;
				DecisionPlan child = prefix;
				child.choices.push_back(DecisionChoice(result.choicePlayer,
					result.options[selected % result.options.size()]));
				enumeratePlan(root, child, plans, options);
				return;
			}
			for (std::vector<int>::const_iterator option = result.options.begin();
				option != result.options.end(); ++option)
			{
				DecisionPlan child = prefix;
				child.choices.push_back(DecisionChoice(result.choicePlayer, *option));
				enumeratePlan(root, child, plans, options);
			}
		}
	}
}

DecisionChoice::DecisionChoice() : player(-1), selection(RETURN_NOTHING)
{
}

DecisionChoice::DecisionChoice(int choicePlayer, int choiceSelection)
	: player(choicePlayer), selection(choiceSelection)
{
}

bool DecisionChoice::operator==(const DecisionChoice& other) const
{
	return player == other.player && selection == other.selection;
}

DecisionPlan::DecisionPlan() : player(-1)
{
}

DecisionPlan::DecisionPlan(int actingPlayer, const Message& primaryAction)
	: player(actingPlayer), action(primaryAction)
{
}

bool DecisionPlan::operator==(const DecisionPlan& other) const
{
	return player == other.player && action.map == other.action.map &&
		manaCards == other.manaCards && choices == other.choices;
}

DecisionPlanResult::DecisionPlanResult()
	: status(DecisionPlanStatus::Illegal), choicePlayer(-1),
	  aiPreferredChoice(RETURN_NOTHING)
{
}

DecisionPlanEnumerationOptions::DecisionPlanEnumerationOptions()
	: heuristicMana(false), heuristicCardPlay(false), heuristicChoices(false),
	  randomChoices(false), randomShieldTarget(false), personality("tempo")
{
}

DecisionPlanResult executeDecisionPlan(Duel& duel, const DecisionPlan& plan)
{
	DecisionPlanResult result;
	if (!duel.mIsSimulation || !duel.isCloneable())
		return result;

	ActiveDuelGuard activeGuard(duel);
	if (plan.player != duel.getPlayerToMove())
		return result;
	std::vector<Message> initialMoves = duel.getPossibleMoves();
	if (!containsMessage(initialMoves, plan.action))
		return result;

	size_t nextChoice = 0;
	bool missingChoice = false;
	bool mismatchedChoicePlayer = false;
	Duel::ChoiceResolver resolver =
		[&](const Duel& position) -> int
		{
			if (nextChoice < plan.choices.size())
			{
				const DecisionChoice& answer = plan.choices[nextChoice++];
				if (answer.player != position.mChoicePlayer)
				{
					mismatchedChoicePlayer = true;
					return RETURN_QUIT;
				}
				return answer.selection;
			}
			// Lua continues the current callback after RETURN_QUIT and may ask a
			// later dependent question. Preserve the first unresolved choice so
			// enumeration can add it before discovering subsequent choices.
			if (missingChoice)
				return RETURN_QUIT;

			missingChoice = true;
			result.choicePlayer = position.mChoicePlayer;
			result.aiPreferredChoice = position.mChoice == NULL ? RETURN_NOTHING :
				position.mChoice->mAiPreferredSelection;
			result.options.clear();
			if (position.mChoice != NULL && position.mChoice->mButtonCount >= 1)
				result.options.push_back(RETURN_BUTTON1);
			if (position.mChoice != NULL && position.mChoice->mButtonCount >= 2)
				result.options.push_back(RETURN_BUTTON2);
			result.options.insert(result.options.end(), position.mChoiceValidCards.begin(),
				position.mChoiceValidCards.end());
			return RETURN_QUIT;
		};
	ResolverScope resolverScope(duel, resolver);
	duel.clearSimulationChoiceFailure();

	Message action = plan.action;
	duel.handleInterfaceInput(action);
	duel.dispatchAllMessages();
	if (duel.hasSimulationChoiceFailure())
	{
		result.status = missingChoice && !mismatchedChoicePlayer ?
			DecisionPlanStatus::NeedsChoice : DecisionPlanStatus::Illegal;
		return result;
	}

	for (std::vector<int>::const_iterator mana = plan.manaCards.begin();
		mana != plan.manaCards.end(); ++mana)
	{
		if (duel.mCastingCard == -1)
			return result;
		std::vector<Message> moves = duel.getPossibleMoves();
		if (!containsManaMove(moves, *mana))
			return result;
		Message tap("manatap");
		tap.addValue("card", *mana);
		duel.handleInterfaceInput(tap);
		duel.dispatchAllMessages();
		if (duel.hasSimulationChoiceFailure())
		{
			result.status = missingChoice && !mismatchedChoicePlayer ?
				DecisionPlanStatus::NeedsChoice : DecisionPlanStatus::Illegal;
			return result;
		}
	}

	if (duel.mCastingCard != -1)
	{
		result.options = manaOptions(duel);
		if (!result.options.empty())
			result.status = DecisionPlanStatus::NeedsMana;
		return result;
	}
	if (nextChoice != plan.choices.size() || duel.mIsChoiceActive ||
		duel.mChoice != NULL || duel.mMsgMngr.hasMoreMessages() ||
		duel.mLuaCallbackSuspended.load())
		return result;

	result.status = DecisionPlanStatus::Complete;
	return result;
}

std::vector<DecisionPlan> enumerateDecisionPlans(Duel& root)
{
	return enumerateDecisionPlans(root, DecisionPlanEnumerationOptions());
}

std::vector<DecisionPlan> enumerateDecisionPlans(Duel& root,
	const DecisionPlanEnumerationOptions& options)
{
	AiPersonalityScope personalityScope(options.personality);
	std::vector<DecisionPlan> plans;
	if (shouldStop(options) || !root.isCloneable())
		return plans;

	ActiveDuelGuard rootGuard(root);
	int player = root.getPlayerToMove();
	std::vector<Message> moves = root.getPossibleMoves();
	if (options.heuristicCardPlay)
	{
		std::vector<Message> filtered;
		for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
		{
			if (messageType(*move) == "cardplay" &&
				root.getCardAiCanCast(messageInt(*move, "card")) == 0)
				continue;
			filtered.push_back(*move);
		}
		moves.swap(filtered);
	}
	if (options.randomShieldTarget && root.mAttackphase == PHASE_TARGET)
	{
		std::vector<Message> targets;
		for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
			if (messageType(*move) == "targetshield") targets.push_back(*move);
		if (!targets.empty())
		{
			size_t selected = options.randomIndex ? options.randomIndex(targets.size()) : 0;
			moves.clear();
			moves.push_back(targets[selected % targets.size()]);
		}
	}
	if (options.heuristicMana)
	{
		HeuristicBot bot(player, options.personality);
		if (root.mCastingCard != -1)
		{
			std::vector<int> paymentOptions;
			for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
				if (messageType(*move) == "manatap")
					paymentOptions.push_back(messageInt(*move, "card"));
			int selected = bot.chooseManaPayment(root, paymentOptions);
			std::vector<Message> filtered;
			for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
				if (messageType(*move) == "manatap" && messageInt(*move, "card") == selected)
					filtered.push_back(*move);
			moves.swap(filtered);
		}
		else
		{
			Message placement;
			bool placeMana = bot.chooseManaPlacement(root, moves, placement);
			bool hasManaPlacement = false;
			for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
				if (messageType(*move) == "cardmana") hasManaPlacement = true;
			if (hasManaPlacement)
			{
				std::vector<Message> filtered;
				if (placeMana)
					filtered.push_back(placement);
				else
				{
					for (std::vector<Message>::const_iterator move = moves.begin();
						move != moves.end(); ++move)
						if (messageType(*move) != "cardmana") filtered.push_back(*move);
				}
				moves.swap(filtered);
			}
		}
	}
	for (std::vector<Message>::const_iterator move = moves.begin(); move != moves.end(); ++move)
	{
		if (shouldStop(options)) break;
		enumeratePlan(root, DecisionPlan(player, *move), plans, options);
	}
	return plans;
}

DecisionPlanCommitStatus commitDecisionPlan(Duel& duel, const DecisionPlan& plan)
{
	if (duel.mIsSimulation || !duel.isCloneable() || plan.player < 0 || plan.player > 1 ||
		duel.mPlayerType[plan.player] != PLAYER_AI)
		return DecisionPlanCommitStatus::Illegal;

	Duel preflight;
	preflight.mIsSimulation = true;
	preflight.mInputLoopRunning = false;
	if (!preflight.copyFrom(duel) ||
		executeDecisionPlan(preflight, plan).status != DecisionPlanStatus::Complete)
		return DecisionPlanCommitStatus::Illegal;

	ActiveDuelGuard activeGuard(duel);
	if (plan.player != duel.getPlayerToMove())
		return DecisionPlanCommitStatus::Illegal;
	std::vector<Message> initialMoves = duel.getPossibleMoves();
	if (!containsMessage(initialMoves, plan.action))
		return DecisionPlanCommitStatus::Illegal;

	std::shared_ptr<PlannedChoiceContext> context(new PlannedChoiceContext());
	for (std::vector<DecisionChoice>::const_iterator choice = plan.choices.begin();
		choice != plan.choices.end(); ++choice)
	{
		if (choice->player != plan.player) break;
		context->choices.push_back(*choice);
	}
	if (context->choices.empty())
		duel.clearChoiceResolver();
	else
	{
		duel.setChoiceResolver(
			[context](const Duel& position) -> int
			{
				if (context->next >= context->choices.size()) return RETURN_NOTHING;
				const DecisionChoice& answer = context->choices[context->next];
				if (answer.player != position.mChoicePlayer) return RETURN_NOTHING;
				context->next++;
				return answer.selection;
			}, (int)context->choices.size());
	}

	Message action = plan.action;
	duel.handleInterfaceInput(action);
	for (std::vector<int>::const_iterator mana = plan.manaCards.begin();
		mana != plan.manaCards.end(); ++mana)
	{
		std::vector<Message> moves = duel.getPossibleMoves();
		if (duel.mCastingCard == -1 || !containsManaMove(moves, *mana))
		{
			duel.clearChoiceResolver();
			return DecisionPlanCommitStatus::Illegal;
		}
		Message tap("manatap");
		tap.addValue("card", *mana);
		duel.handleInterfaceInput(tap);
	}
	if (duel.mCastingCard != -1)
	{
		duel.clearChoiceResolver();
		return DecisionPlanCommitStatus::Illegal;
	}
	return DecisionPlanCommitStatus::Committed;
}
