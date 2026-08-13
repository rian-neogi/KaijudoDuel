#include "Mcts.h"

#include "AiParams.h"
#include "AiScoring.h"
#include "HeuristicBot.h"
#include "MctsTiming.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <random>

namespace
{
	struct MctsTimingCounters
	{
		long long cloneNs;
		long long treeEnumerationNs;
		long long rolloutEnumerationNs;
		long long rolloutSelectionNs;
		long long actionExecutionNs;
		long long evaluationNs;
		long long luaCallbackNs;

		MctsTimingCounters()
			: cloneNs(0), treeEnumerationNs(0), rolloutEnumerationNs(0),
			  rolloutSelectionNs(0), actionExecutionNs(0), evaluationNs(0),
			  luaCallbackNs(0)
		{
		}
	};

	long long elapsedNanoseconds(const std::chrono::steady_clock::time_point& started)
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now() - started).count();
	}

	struct MctsNode;

	struct PlanNode
	{
		DecisionPlan plan;
		DecisionChoice incomingChoice;
		bool hasIncomingChoice;
		bool leaf;
		int player;
		int visits;
		double valueSum;
		std::vector<std::unique_ptr<PlanNode> > children;
		std::unique_ptr<MctsNode> stateChild;

		PlanNode()
			: hasIncomingChoice(false), leaf(false), player(-1), visits(0), valueSum(0.0)
		{
		}
	};

	struct MctsNode
	{
		int player;
		int visits;
		double valueSum;
		std::string stateKey;
		int validationGeneration;
		bool initialized;
		bool hasForcedPlan;
		DecisionPlan forcedPlan;
		std::unique_ptr<MctsNode> forcedChild;
		std::unique_ptr<PlanNode> plans;

		MctsNode()
			: player(-1), visits(0), valueSum(0.0), validationGeneration(-1), initialized(false),
			  hasForcedPlan(false)
		{
		}
	};

	void appendStateInt(std::string& key, int value)
	{
		for (size_t byte = 0; byte < sizeof(value); ++byte)
			key.push_back(static_cast<char>((static_cast<unsigned int>(value) >>
				(byte * 8)) & 0xffU));
	}

	void appendStateString(std::string& key, const std::string& value)
	{
		appendStateInt(key, static_cast<int>(value.size()));
		key.append(value);
	}

	void appendCardIds(std::string& key, const std::vector<Card*>& cards)
	{
		appendStateInt(key, static_cast<int>(cards.size()));
		for (std::vector<Card*>::const_iterator card = cards.begin(); card != cards.end(); ++card)
			appendStateInt(key, (*card)->mUniqueId);
	}

	void appendIntValues(std::string& key, const std::vector<int>& values)
	{
		appendStateInt(key, static_cast<int>(values.size()));
		for (std::vector<int>::const_iterator value = values.begin(); value != values.end(); ++value)
			appendStateInt(key, *value);
	}

	std::string duelStateKey(const Duel& duel)
	{
		std::string key;
		key.reserve(duel.mCardList.size() * 64);
		appendStateInt(key, duel.mTurn);
		appendStateInt(key, duel.mTurnPhase);
		appendStateInt(key, duel.mManaUsed);
		appendStateInt(key, duel.mWinner);
		appendStateInt(key, duel.mPlayerType[0]);
		appendStateInt(key, duel.mPlayerType[1]);
		appendStateInt(key, duel.mAttacker);
		appendStateInt(key, duel.mDefender);
		appendStateInt(key, duel.mDefenderType);
		appendStateInt(key, duel.mBreakCount);
		appendStateInt(key, duel.mAttackphase);
		appendIntValues(key, duel.mShieldTargets);
		for (int player = 0; player < 2; ++player)
		{
			std::vector<int> breakers(duel.mShieldBreakersThisTurn[player].begin(),
				duel.mShieldBreakersThisTurn[player].end());
			std::sort(breakers.begin(), breakers.end());
			appendIntValues(key, breakers);
			appendStateInt(key, duel.mShieldsBrokenThisTurn[player]);
			appendStateInt(key, duel.mCardsDrawnThisTurn[player]);
		}
		std::vector<std::string> ruleNames;
		for (std::unordered_map<std::string, std::unordered_map<int, int> >::const_iterator rule =
			duel.mLuaRuleState.begin(); rule != duel.mLuaRuleState.end(); ++rule)
			ruleNames.push_back(rule->first);
		std::sort(ruleNames.begin(), ruleNames.end());
		appendStateInt(key, static_cast<int>(ruleNames.size()));
		for (std::vector<std::string>::const_iterator name = ruleNames.begin();
			name != ruleNames.end(); ++name)
		{
			appendStateString(key, *name);
			std::unordered_map<std::string, std::unordered_map<int, int> >::const_iterator rule =
				duel.mLuaRuleState.find(*name);
			std::vector<std::pair<int, int> > values(rule->second.begin(), rule->second.end());
			std::sort(values.begin(), values.end());
			appendStateInt(key, static_cast<int>(values.size()));
			for (std::vector<std::pair<int, int> >::const_iterator value = values.begin();
				value != values.end(); ++value)
			{
				appendStateInt(key, value->first);
				appendStateInt(key, value->second);
			}
		}
		appendStateInt(key, duel.mCastingCard);
		appendStateInt(key, duel.mCastingCivilizations);
		appendStateInt(key, duel.mCastingCost);
		appendStateInt(key, duel.mCastingEvobait);
		appendStateInt(key, duel.mCastingEvobait2);
		appendIntValues(key, duel.mCastingManaCards);
		appendStateInt(key, duel.mNextUniqueId);
		appendStateInt(key, static_cast<int>(duel.mCardList.size()));
		for (std::vector<Card*>::const_iterator card = duel.mCardList.begin();
			card != duel.mCardList.end(); ++card)
		{
			// Lua registry references are clone-local identities. Until modifiers
			// have a stable engine-side type ID, declining reuse is safer than
			// treating two behaviorally different closures as the same state.
			if (!(*card)->mModifiers.empty()) return std::string();
			appendStateInt(key, (*card)->mUniqueId);
			appendStateInt(key, (*card)->mCardId);
			appendStateInt(key, (*card)->mOwner);
			appendStateInt(key, (*card)->mZone);
			appendStateInt(key, (*card)->mPower);
			appendStateInt(key, (*card)->mBreaker);
			appendStateInt(key, (*card)->mIsBlocker);
			appendStateInt(key, (*card)->mIsShieldTrigger);
			appendStateInt(key, (*card)->mIsTapped ? 1 : 0);
			appendStateInt(key, (*card)->mIsFlipped ? 1 : 0);
			appendStateInt(key, (*card)->mSummoningSickness);
			appendStateInt(key, (*card)->mIsVisible[0] ? 1 : 0);
			appendStateInt(key, (*card)->mIsVisible[1] ? 1 : 0);
			appendCardIds(key, (*card)->mEvoStack);
			appendStateInt(key, 0);
		}
		for (int player = 0; player < 2; ++player)
		{
			appendCardIds(key, duel.mDecks[player].mCards);
			appendCardIds(key, duel.mHands[player].mCards);
			appendCardIds(key, duel.mManazones[player].mCards);
			appendCardIds(key, duel.mGraveyards[player].mCards);
			appendCardIds(key, duel.mShields[player].mCards);
			appendStateInt(key, duel.mShields[player].mSlotsUsed);
			appendCardIds(key, duel.mBattlezones[player].mCards);
		}
		return key;
	}

	void findMatchingPlanState(PlanNode& planNode, const std::string& stateKey,
		std::unique_ptr<MctsNode>** best);

	void findMatchingState(std::unique_ptr<MctsNode>& node, const std::string& stateKey,
		std::unique_ptr<MctsNode>** best)
	{
		if (node == NULL) return;
		if (!stateKey.empty() && node->stateKey == stateKey &&
			(*best == NULL || node->visits > (**best)->visits))
			*best = &node;
		findMatchingState(node->forcedChild, stateKey, best);
		if (node->plans != NULL) findMatchingPlanState(*node->plans, stateKey, best);
	}

	void findMatchingPlanState(PlanNode& planNode, const std::string& stateKey,
		std::unique_ptr<MctsNode>** best)
	{
		findMatchingState(planNode.stateChild, stateKey, best);
		for (std::vector<std::unique_ptr<PlanNode> >::iterator child = planNode.children.begin();
			child != planNode.children.end(); ++child)
			findMatchingPlanState(**child, stateKey, best);
	}

	void pruneUnkeyedPlanStates(PlanNode& planNode);

	void pruneUnkeyedStates(MctsNode& node)
	{
		if (node.forcedChild != NULL)
		{
			if (node.forcedChild->stateKey.empty()) node.forcedChild.reset();
			else pruneUnkeyedStates(*node.forcedChild);
		}
		if (node.plans != NULL) pruneUnkeyedPlanStates(*node.plans);
	}

	void pruneUnkeyedPlanStates(PlanNode& planNode)
	{
		if (planNode.stateChild != NULL)
		{
			if (planNode.stateChild->stateKey.empty()) planNode.stateChild.reset();
			else pruneUnkeyedStates(*planNode.stateChild);
		}
		for (std::vector<std::unique_ptr<PlanNode> >::iterator child = planNode.children.begin();
			child != planNode.children.end(); ++child)
			pruneUnkeyedPlanStates(**child);
	}

	struct TurnHorizon
	{
		int rootPlayer;
		bool aiTurnSeen;
		bool opponentTurnSeenAfterAi;
		bool cutoff;
		int extraTurnDepthExtensions;

		TurnHorizon(const Duel& root, int player)
			: rootPlayer(player), aiTurnSeen(root.mTurn == player),
			  opponentTurnSeenAfterAi(false), cutoff(false), extraTurnDepthExtensions(0)
		{
		}

		int depthLimit(int baseDepth) const
		{
			int multiplier = 1 + extraTurnDepthExtensions;
			if (baseDepth > std::numeric_limits<int>::max() / multiplier)
				return std::numeric_limits<int>::max();
			return baseDepth * multiplier;
		}

		void observe(const Duel& position, bool immediateExtraTurn)
		{
			if (cutoff) return;
			if (immediateExtraTurn &&
				extraTurnDepthExtensions <
					std::max(0, aiIntParam("search.max_extra_turn_depth_extensions")))
				extraTurnDepthExtensions++;
			if (!aiTurnSeen)
			{
				if (position.mTurn == rootPlayer) aiTurnSeen = true;
				return;
			}
			if (!opponentTurnSeenAfterAi)
			{
				if (position.mTurn != rootPlayer) opponentTurnSeenAfterAi = true;
				return;
			}
			if (position.mTurn == rootPlayer) cutoff = true;
		}
	};

	template <typename Node>
	double meanValue(const Node& node)
	{
		return node.visits == 0 ? 0.0 : node.valueSum / node.visits;
	}

	std::string messageType(const Message& message)
	{
		std::map<std::string, std::string>::const_iterator found =
			message.map.find("msgtype");
		return found == message.map.end() ? "" : found->second;
	}

	bool isDirectCombatAction(const std::string& type)
	{
		return type == "creatureattack" || type == "creatureblock" ||
			type == "blockskip";
	}

	bool startedImmediateExtraTurn(const DecisionPlan& plan, int previousTurn,
		const Duel& position)
	{
		return messageType(plan.action) == "endturn" && position.mWinner == -1 &&
			position.mTurn == previousTurn;
	}

	bool sameActionAndPayment(const DecisionPlan& first, const DecisionPlan& second)
	{
		return first.player == second.player && first.action.map == second.action.map &&
			first.manaCards == second.manaCards;
	}

	bool addChoicePath(PlanNode& node, const DecisionPlan& plan, size_t choiceIndex)
	{
		if (choiceIndex == plan.choices.size())
		{
			if (!node.children.empty()) return false;
			if (node.leaf) return node.plan == plan;
			node.leaf = true;
			node.plan = plan;
			return true;
		}

		if (node.leaf) return false;
		const DecisionChoice& choice = plan.choices[choiceIndex];
		if (node.player == -1) node.player = choice.player;
		else if (node.player != choice.player) return false;

		PlanNode* childNode = NULL;
		for (std::vector<std::unique_ptr<PlanNode> >::iterator child = node.children.begin();
			child != node.children.end(); ++child)
		{
			if ((*child)->hasIncomingChoice && (*child)->incomingChoice == choice)
			{
				childNode = child->get();
				break;
			}
		}
		if (childNode == NULL)
		{
			std::unique_ptr<PlanNode> child(new PlanNode());
			child->hasIncomingChoice = true;
			child->incomingChoice = choice;
			node.children.push_back(std::move(child));
			childNode = node.children.back().get();
		}
		return addChoicePath(*childNode, plan, choiceIndex + 1);
	}

	bool buildPlanTree(const std::vector<DecisionPlan>& plans, int player, PlanNode& root,
		const std::function<bool()>& shouldStop = std::function<bool()>())
	{
		root.player = player;
		for (std::vector<DecisionPlan>::const_iterator plan = plans.begin(); plan != plans.end(); ++plan)
		{
			if (shouldStop && shouldStop()) return false;
			PlanNode* actionNode = NULL;
			for (std::vector<std::unique_ptr<PlanNode> >::iterator child = root.children.begin();
				child != root.children.end(); ++child)
			{
				if (!(*child)->hasIncomingChoice && sameActionAndPayment((*child)->plan, *plan))
				{
					actionNode = child->get();
					break;
				}
			}
			if (actionNode == NULL)
			{
				std::unique_ptr<PlanNode> child(new PlanNode());
				child->plan = *plan;
				child->plan.choices.clear();
				root.children.push_back(std::move(child));
				actionNode = root.children.back().get();
			}
			if (!addChoicePath(*actionNode, *plan, 0)) return false;
		}
		return true;
	}

	double evaluate(Duel& duel, int rootPlayer)
	{
		if (duel.mWinner == rootPlayer) return aiParam("evaluation.win_value");
		if (duel.mWinner == 1 - rootPlayer) return aiParam("evaluation.loss_value");
		double difference = AiScoring::playerValue(duel, rootPlayer) -
			AiScoring::playerValue(duel, 1 - rootPlayer);
		double scale = std::max(0.000001, aiParam("evaluation.normalization_scale"));
		return std::tanh(difference / scale);
	}

	bool initializeNode(MctsNode& node, Duel& position,
		const std::function<bool()>& shouldStop, std::mt19937& random)
	{
		if (node.initialized) return true;
		if (shouldStop && shouldStop()) return false;
		ActiveDuelGuard activeGuard(position);
		int player = position.getPlayerToMove();
		DecisionPlanEnumerationOptions options;
		options.heuristicMana = true;
		options.heuristicCardPlay = true;
		options.heuristicChoices = true;
		options.randomShieldTarget = true;
		options.randomIndex = [&random](size_t count) -> size_t
		{
			std::uniform_int_distribution<size_t> choice(0, count - 1);
			return choice(random);
		};
		options.shouldStop = shouldStop;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(position, options);
		if (shouldStop && shouldStop()) return false;
		node.player = player;
		if (plans.size() == 1)
		{
			node.hasForcedPlan = true;
			node.forcedPlan = plans.front();
			node.initialized = true;
			return true;
		}
		std::unique_ptr<PlanNode> plansRoot(new PlanNode());
		if (!buildPlanTree(plans, player, *plansRoot, shouldStop)) return false;
		node.plans = std::move(plansRoot);
		node.initialized = true;
		return true;
	}

	PlanNode* selectBanditChild(PlanNode& node, int rootPlayer, double exploration,
		std::mt19937& random)
	{
		std::vector<PlanNode*> unvisited;
		for (std::vector<std::unique_ptr<PlanNode> >::iterator child = node.children.begin();
			child != node.children.end(); ++child)
		{
			if ((*child)->visits == 0) unvisited.push_back(child->get());
		}
		if (!unvisited.empty())
		{
			std::uniform_int_distribution<size_t> choice(0, unvisited.size() - 1);
			return unvisited[choice(random)];
		}

		PlanNode* selected = NULL;
		double bestScore = -std::numeric_limits<double>::infinity();
		double parentVisits = std::max(1, node.visits);
		for (std::vector<std::unique_ptr<PlanNode> >::iterator child = node.children.begin();
			child != node.children.end(); ++child)
		{
			double exploitation = meanValue(**child);
			if (node.player != rootPlayer) exploitation = -exploitation;
			double explorationTerm = exploration *
				std::sqrt(std::log(parentVisits) / (*child)->visits);
			double score = exploitation + explorationTerm;
			if (selected == NULL || score > bestScore)
			{
				selected = child->get();
				bestScore = score;
			}
		}
		return selected;
	}

	PlanNode* selectPlanLeaf(PlanNode& root, int rootPlayer, double exploration,
		std::mt19937& random, std::vector<PlanNode*>& path)
	{
		PlanNode* node = &root;
		path.push_back(node);
		while (!node->leaf)
		{
			if (node->children.empty()) return NULL;
			node = selectBanditChild(*node, rootPlayer, exploration, random);
			if (node == NULL) return NULL;
			path.push_back(node);
		}
		return node;
	}

	bool selectRolloutPlan(Duel& position, const std::vector<DecisionPlan>& plans, int player,
		std::mt19937& random, DecisionPlan& selected,
		const std::function<bool()>& shouldStop)
	{
		std::unique_ptr<PlanNode> root(new PlanNode());
		if (!buildPlanTree(plans, player, *root, shouldStop)) return false;
		PlanNode* node = root.get();
		bool primaryAction = true;
		while (!node->leaf)
		{
			if (shouldStop && shouldStop()) return false;
			if (node->children.empty()) return false;
			size_t selectedIndex = 0;
			if (primaryAction)
			{
				std::vector<size_t> combatActions;
				bool hasDirectCombatAction = false;
				for (size_t i = 0; i < node->children.size(); ++i)
				{
					if (isDirectCombatAction(messageType(node->children[i]->plan.action)))
						hasDirectCombatAction = true;
				}
				for (size_t i = 0; i < node->children.size(); ++i)
				{
					const std::string type = messageType(node->children[i]->plan.action);
					if (isDirectCombatAction(type) ||
						(hasDirectCombatAction && type == "endturn"))
						combatActions.push_back(i);
				}

				std::vector<double> weights(node->children.size(), 1.0);
				if (combatActions.size() > 1)
				{
					ActiveDuelGuard activeGuard(position);
					HeuristicBot bot(player);
					std::vector<double> scores(combatActions.size(),
						-std::numeric_limits<double>::infinity());
					double maximumScore = -std::numeric_limits<double>::infinity();
					size_t finiteCount = 0;
					for (size_t i = 0; i < combatActions.size(); ++i)
					{
						scores[i] = bot.scoreMove(position,
							node->children[combatActions[i]]->plan.action);
						if (std::isfinite(scores[i]))
						{
							maximumScore = std::max(maximumScore, scores[i]);
							finiteCount++;
						}
					}

					if (finiteCount > 0)
					{
						std::vector<double> exponentials(combatActions.size(), 0.0);
						double exponentialSum = 0.0;
						for (size_t i = 0; i < combatActions.size(); ++i)
						{
							if (!std::isfinite(scores[i])) continue;
							double temperature = std::max(0.000001,
								aiParam("search.rollout_combat_temperature"));
							exponentials[i] = std::exp(
								(scores[i] - maximumScore) / temperature);
							exponentialSum += exponentials[i];
						}
						for (size_t i = 0; i < combatActions.size(); ++i)
						{
							if (!std::isfinite(scores[i]))
							{
								weights[combatActions[i]] = 0.0;
								continue;
							}
							double uniformExploration = std::max(0.0, std::min(1.0,
								aiParam("search.rollout_uniform_exploration")));
							double probability =
								(1.0 - uniformExploration) *
									exponentials[i] / exponentialSum +
								uniformExploration / finiteCount;
							// Preserve the combat group's old aggregate probability. Card
							// plays and other action classes therefore remain equal-weight.
							weights[combatActions[i]] = combatActions.size() * probability;
						}
					}
				}
				std::discrete_distribution<size_t> choice(weights.begin(), weights.end());
				selectedIndex = choice(random);
			}
			else
			{
				// Spell targets and other ordered choices remain uniform.
				std::uniform_int_distribution<size_t> choice(0, node->children.size() - 1);
				selectedIndex = choice(random);
			}
			node = node->children[selectedIndex].get();
			primaryAction = false;
		}
		selected = node->plan;
		return true;
	}

	bool executeCompletePlan(Duel& duel, const DecisionPlan& plan)
	{
		return executeDecisionPlan(duel, plan).status == DecisionPlanStatus::Complete;
	}

	bool rollout(Duel& position, int& depth, int maxDepth, std::mt19937& random,
		const std::function<bool()>& shouldStop, bool& stopped, TurnHorizon& horizon,
		int& forcedMovesApplied, MctsTimingCounters& timings)
	{
		while (position.mWinner == -1 && depth < horizon.depthLimit(maxDepth) &&
			!horizon.cutoff)
		{
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return true;
			}
			DecisionPlanEnumerationOptions options;
			options.heuristicMana = true;
			options.heuristicCardPlay = true;
			options.heuristicChoices = true;
			options.randomChoices = true;
			options.randomShieldTarget = true;
			options.randomIndex = [&random](size_t count) -> size_t
			{
				std::uniform_int_distribution<size_t> choice(0, count - 1);
				return choice(random);
			};
			options.shouldStop = shouldStop;
			std::chrono::steady_clock::time_point enumerationStarted =
				std::chrono::steady_clock::now();
			std::vector<DecisionPlan> plans = enumerateDecisionPlans(position, options);
			timings.rolloutEnumerationNs += elapsedNanoseconds(enumerationStarted);
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return true;
			}
			if (plans.empty()) break;
			int player = -1;
			{
				ActiveDuelGuard activeGuard(position);
				player = position.getPlayerToMove();
			}
			DecisionPlan selected;
			bool forced = plans.size() == 1;
			if (forced)
				selected = plans.front();
			else
			{
				std::chrono::steady_clock::time_point selectionStarted =
					std::chrono::steady_clock::now();
				bool selectedPlan = selectRolloutPlan(position, plans, player, random,
					selected, shouldStop);
				timings.rolloutSelectionNs += elapsedNanoseconds(selectionStarted);
				if (!selectedPlan)
				{
					if (shouldStop && shouldStop())
					{
						stopped = true;
						return true;
					}
					return false;
				}
			}
			int previousTurn = position.mTurn;
			std::chrono::steady_clock::time_point executionStarted =
				std::chrono::steady_clock::now();
			bool executed = executeCompletePlan(position, selected);
			timings.actionExecutionNs += elapsedNanoseconds(executionStarted);
			if (!executed)
			{
				if (shouldStop && shouldStop()) stopped = true;
				return false;
			}
			if (forced) forcedMovesApplied++;
			++depth;
			horizon.observe(position, startedImmediateExtraTurn(selected, previousTurn, position));
		}
		return true;
	}

	bool runIteration(Duel& root, MctsNode& rootNode, int rootPlayer,
		const MctsConfig& config, int validationGeneration, std::mt19937& random,
		const std::function<bool()>& shouldStop, bool& stopped, bool& horizonCutoff,
		int& forcedMovesApplied, MctsTimingCounters& timings)
	{
		if (shouldStop && shouldStop())
		{
			stopped = true;
			return false;
		}
		Duel position;
		position.mIsSimulation = true;
		position.mInputLoopRunning = false;
		std::chrono::steady_clock::time_point cloneStarted =
			std::chrono::steady_clock::now();
		bool cloned = position.copyFrom(root);
		timings.cloneNs += elapsedNanoseconds(cloneStarted);
		if (!cloned) return false;

		std::vector<MctsNode*> statePath;
		std::vector<PlanNode*> planPath;
		MctsNode* node = &rootNode;
		statePath.push_back(node);
		TurnHorizon horizon(root, rootPlayer);
		int depth = 0;
		while (position.mWinner == -1 && depth < horizon.depthLimit(config.maxDepth) &&
			!horizon.cutoff)
		{
			bool needsEnumeration = !node->initialized;
			std::chrono::steady_clock::time_point enumerationStarted =
				std::chrono::steady_clock::now();
			bool initialized = initializeNode(*node, position, shouldStop, random);
			if (needsEnumeration)
				timings.treeEnumerationNs += elapsedNanoseconds(enumerationStarted);
			if (!initialized)
			{
				if (shouldStop && shouldStop()) stopped = true;
				if (stopped) return false;
				break;
			}
			if (node->hasForcedPlan)
			{
				int previousTurn = position.mTurn;
				std::chrono::steady_clock::time_point executionStarted =
					std::chrono::steady_clock::now();
				bool executed = executeCompletePlan(position, node->forcedPlan);
				timings.actionExecutionNs += elapsedNanoseconds(executionStarted);
				if (!executed) return false;
				forcedMovesApplied++;
				++depth;
				horizon.observe(position, startedImmediateExtraTurn(node->forcedPlan,
					previousTurn, position));
				if (shouldStop && shouldStop())
				{
					stopped = true;
					break;
				}
				if (position.mWinner == -1 && (node->forcedChild == NULL ||
					node->forcedChild->validationGeneration != validationGeneration))
				{
					std::string stateKey = duelStateKey(position);
					if (node->forcedChild != NULL && !stateKey.empty() &&
						node->forcedChild->stateKey != stateKey)
						node->forcedChild.reset();
					if (node->forcedChild == NULL) node->forcedChild.reset(new MctsNode());
					{
						ActiveDuelGuard activeGuard(position);
						node->forcedChild->player = position.getPlayerToMove();
					}
					node->forcedChild->stateKey = stateKey;
					node->forcedChild->validationGeneration = validationGeneration;
				}
				if (position.mWinner != -1) break;
				node = node->forcedChild.get();
				statePath.push_back(node);
				if (depth >= horizon.depthLimit(config.maxDepth) || horizon.cutoff)
					break;
				continue;
			}
			if (node->plans == NULL ||
				node->plans->children.empty())
				break;

			PlanNode* leaf = selectPlanLeaf(*node->plans, rootPlayer, config.exploration,
				random, planPath);
			int previousTurn = position.mTurn;
			if (leaf == NULL) return false;
			std::chrono::steady_clock::time_point executionStarted =
				std::chrono::steady_clock::now();
			bool executed = executeCompletePlan(position, leaf->plan);
			timings.actionExecutionNs += elapsedNanoseconds(executionStarted);
			if (!executed) return false;
			++depth;
			horizon.observe(position, startedImmediateExtraTurn(leaf->plan, previousTurn, position));
			if (shouldStop && shouldStop())
			{
				stopped = true;
				break;
			}
			bool newStateChild = leaf->stateChild == NULL;
			if (newStateChild ||
				leaf->stateChild->validationGeneration != validationGeneration)
			{
				std::string stateKey = duelStateKey(position);
				if (leaf->stateChild != NULL && !stateKey.empty() &&
					leaf->stateChild->stateKey != stateKey)
				{
					leaf->stateChild.reset();
					newStateChild = true;
				}
				if (leaf->stateChild == NULL) leaf->stateChild.reset(new MctsNode());
				{
					ActiveDuelGuard activeGuard(position);
					leaf->stateChild->player = position.getPlayerToMove();
				}
				leaf->stateChild->stateKey = stateKey;
				leaf->stateChild->validationGeneration = validationGeneration;
			}
			node = leaf->stateChild.get();
			statePath.push_back(node);
			if (horizon.cutoff || newStateChild) break;
		}

		if (!rollout(position, depth, config.maxDepth, random, shouldStop, stopped, horizon,
			forcedMovesApplied, timings))
			return false;
		horizonCutoff = horizon.cutoff;
		std::chrono::steady_clock::time_point evaluationStarted =
			std::chrono::steady_clock::now();
		double reward = evaluate(position, rootPlayer);
		timings.evaluationNs += elapsedNanoseconds(evaluationStarted);
		for (std::vector<MctsNode*>::iterator visited = statePath.begin();
			visited != statePath.end(); ++visited)
		{
			(*visited)->visits++;
			(*visited)->valueSum += reward;
		}
		for (std::vector<PlanNode*>::iterator visited = planPath.begin();
			visited != planPath.end(); ++visited)
		{
			(*visited)->visits++;
			(*visited)->valueSum += reward;
		}
		return true;
	}

	const PlanNode* robustLeaf(const PlanNode& root, int rootPlayer)
	{
		const PlanNode* node = &root;
		while (!node->leaf)
		{
			const PlanNode* selected = NULL;
			for (std::vector<std::unique_ptr<PlanNode> >::const_iterator child =
				node->children.begin(); child != node->children.end(); ++child)
			{
				if (selected == NULL || (*child)->visits > selected->visits)
					selected = child->get();
				else if ((*child)->visits == selected->visits)
				{
					double candidate = meanValue(**child);
					double current = meanValue(*selected);
					if ((node->player == rootPlayer && candidate > current) ||
						(node->player != rootPlayer && candidate < current))
						selected = child->get();
				}
			}
			if (selected == NULL) return NULL;
			node = selected;
		}
		return node;
	}

	MctsResult collectResult(const MctsNode& rootNode, int rootPlayer,
		int iterationsCompleted, int failedIterations, bool timeBudgetExpired,
		int turnHorizonCutoffs, int forcedMovesApplied)
	{
		MctsResult result;
		result.iterationsCompleted = iterationsCompleted;
		result.failedIterations = failedIterations;
		result.timeBudgetExpired = timeBudgetExpired;
		result.turnHorizonCutoffs = turnHorizonCutoffs;
		result.forcedMovesApplied = forcedMovesApplied;
		result.meanValue = meanValue(rootNode);
		if (rootNode.hasForcedPlan && rootNode.visits > 0)
		{
			MctsChildStatistics statistics;
			statistics.plan = rootNode.forcedPlan;
			statistics.visits = rootNode.visits;
			statistics.meanValue = meanValue(rootNode);
			result.rootChildren.push_back(statistics);
			result.hasPlan = true;
			result.plan = rootNode.forcedPlan;
			result.selectedVisits = rootNode.visits;
			result.selectedMeanValue = meanValue(rootNode);
			return result;
		}
		const PlanNode* bestAction = NULL;
		if (rootNode.plans != NULL)
		{
			for (std::vector<std::unique_ptr<PlanNode> >::const_iterator child =
				rootNode.plans->children.begin(); child != rootNode.plans->children.end(); ++child)
			{
				const PlanNode* representative = robustLeaf(**child, rootPlayer);
				if (representative == NULL) continue;
				MctsChildStatistics statistics;
				statistics.plan = representative->plan;
				statistics.visits = (*child)->visits;
				statistics.meanValue = meanValue(**child);
				result.rootChildren.push_back(statistics);

				if (bestAction == NULL || (*child)->visits > bestAction->visits ||
					((*child)->visits == bestAction->visits &&
					 meanValue(**child) > meanValue(*bestAction)))
					bestAction = child->get();
			}
		}
		if (bestAction != NULL && bestAction->visits > 0)
		{
			const PlanNode* selected = robustLeaf(*bestAction, rootPlayer);
			if (selected != NULL)
			{
				result.hasPlan = true;
				result.plan = selected->plan;
				result.selectedVisits = bestAction->visits;
				result.selectedMeanValue = meanValue(*bestAction);
			}
		}
		return result;
	}
}

MctsConfig::MctsConfig()
	: iterations(aiIntParam("search.max_rollouts")),
	  maxDepth(aiIntParam("search.max_depth")),
	  timeBudgetMs(aiIntParam("search.default_time_budget_ms")),
	  exploration(aiParam("search.uct_exploration")),
	  seed(aiSeedParam("search.random_seed"))
{
}

MctsChildStatistics::MctsChildStatistics() : visits(0), meanValue(0.0)
{
}

MctsResult::MctsResult()
	: hasPlan(false), iterationsCompleted(0), failedIterations(0), timeBudgetExpired(false),
	  turnHorizonCutoffs(0), forcedMovesApplied(0), reusedTree(false),
	  reusedRootVisits(0), cloneTimeMs(0.0), treeEnumerationTimeMs(0.0),
	  rolloutEnumerationTimeMs(0.0), rolloutSelectionTimeMs(0.0),
	  actionExecutionTimeMs(0.0), evaluationTimeMs(0.0), luaCallbackTimeMs(0.0),
	  meanValue(0.0),
	  selectedVisits(0), selectedMeanValue(0.0)
{
}

struct MctsSession::Impl
{
	int rootPlayer;
	MctsConfig config;
	Duel root;
	std::unique_ptr<MctsNode> rootNode;
	std::mt19937 random;
	int iterationsCompleted;
	int failedIterations;
	int turnHorizonCutoffs;
	int forcedMovesApplied;
	bool started;
	bool reusedTree;
	int reusedRootVisits;
	int validationGeneration;
	MctsTimingCounters timings;
	std::chrono::steady_clock::time_point deadline;
	bool hasDeadline;

	Impl(int player, const MctsConfig& searchConfig)
		: rootPlayer(player), config(searchConfig), rootNode(new MctsNode()),
		  random(searchConfig.seed),
		  iterationsCompleted(0), failedIterations(0), turnHorizonCutoffs(0),
		  forcedMovesApplied(0), started(false), reusedTree(false),
		  reusedRootVisits(0), validationGeneration(0), hasDeadline(false)
	{
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		rootNode->player = player;
	}

	bool timeExpired() const
	{
		return hasDeadline && std::chrono::steady_clock::now() >= deadline;
	}
};

MctsSession::MctsSession(int rootPlayer, const MctsConfig& config)
	: mImpl(new Impl(rootPlayer, config))
{
}

MctsSession::~MctsSession()
{
}

bool MctsSession::start(Duel& root)
{
	if (mImpl->started || (mImpl->rootPlayer != 0 && mImpl->rootPlayer != 1) ||
		mImpl->config.iterations <= 0 || mImpl->config.maxDepth <= 0 ||
		mImpl->config.timeBudgetMs < 0 ||
		!std::isfinite(mImpl->config.exploration) || mImpl->config.exploration < 0.0)
		return false;
	if (mImpl->config.timeBudgetMs > 0)
	{
		mImpl->hasDeadline = true;
		mImpl->deadline = std::chrono::steady_clock::now() +
			std::chrono::milliseconds(mImpl->config.timeBudgetMs);
	}
	if (!root.isCloneable())
		return false;
	{
		ActiveDuelGuard rootGuard(root);
		if (root.mWinner != -1 || root.getPlayerToMove() != mImpl->rootPlayer)
			return false;
	}
	std::chrono::steady_clock::time_point cloneStarted =
		std::chrono::steady_clock::now();
	bool cloned = mImpl->root.copyFrom(root);
	mImpl->timings.cloneNs += elapsedNanoseconds(cloneStarted);
	if (!cloned) return false;
	mImpl->rootNode.reset(new MctsNode());
	mImpl->rootNode->player = mImpl->rootPlayer;
	mImpl->rootNode->stateKey = duelStateKey(mImpl->root);
	mImpl->rootNode->validationGeneration = mImpl->validationGeneration;
	mImpl->started = true;
	return true;
}

bool MctsSession::restart(Duel& root, const MctsConfig& config)
{
	if (!mImpl->started || !isComplete() || config.iterations <= 0 ||
		config.maxDepth <= 0 || config.timeBudgetMs < 0 ||
		!std::isfinite(config.exploration) || config.exploration < 0.0 ||
		!root.isCloneable())
		return false;
	{
		ActiveDuelGuard rootGuard(root);
		if (root.mWinner != -1 || root.getPlayerToMove() != mImpl->rootPlayer)
			return false;
	}

	const std::string stateKey = duelStateKey(root);
	std::unique_ptr<MctsNode>* matchingNode = NULL;
	findMatchingState(mImpl->rootNode, stateKey, &matchingNode);
	mImpl->timings = MctsTimingCounters();
	std::chrono::steady_clock::time_point cloneStarted =
		std::chrono::steady_clock::now();
	bool cloned = mImpl->root.copyFrom(root);
	mImpl->timings.cloneNs += elapsedNanoseconds(cloneStarted);
	if (!cloned) return false;
	std::unique_ptr<MctsNode> nextRoot;
	if (matchingNode != NULL) nextRoot = std::move(*matchingNode);
	if (nextRoot == NULL) nextRoot.reset(new MctsNode());

	mImpl->config = config;
	mImpl->rootNode = std::move(nextRoot);
	mImpl->rootNode->player = mImpl->rootPlayer;
	mImpl->rootNode->stateKey = stateKey;
	mImpl->reusedTree = matchingNode != NULL;
	mImpl->reusedRootVisits = mImpl->reusedTree ? mImpl->rootNode->visits : 0;
	mImpl->validationGeneration++;
	mImpl->rootNode->validationGeneration = mImpl->validationGeneration;
	if (mImpl->reusedTree) pruneUnkeyedStates(*mImpl->rootNode);
	mImpl->iterationsCompleted = 0;
	mImpl->failedIterations = 0;
	mImpl->turnHorizonCutoffs = 0;
	mImpl->forcedMovesApplied = 0;
	mImpl->hasDeadline = config.timeBudgetMs > 0;
	if (mImpl->hasDeadline)
		mImpl->deadline = std::chrono::steady_clock::now() +
			std::chrono::milliseconds(config.timeBudgetMs);
	if (!mImpl->reusedTree) mImpl->random.seed(config.seed);
	return true;
}

bool MctsSession::advance(int iterationBudget)
{
	if (!mImpl->started) return false;
	MctsTiming::SearchScope luaTimingScope(mImpl->timings.luaCallbackNs);
	int attempted = mImpl->iterationsCompleted + mImpl->failedIterations;
	int remaining = mImpl->config.iterations - attempted;
	int count = std::min(std::max(0, iterationBudget), remaining);
	std::function<bool()> shouldStop = [this]() { return mImpl->timeExpired(); };
	for (int iteration = 0; iteration < count; ++iteration)
	{
		if (mImpl->timeExpired()) break;
		bool stopped = false;
		bool horizonCutoff = false;
		int forcedMovesApplied = 0;
		if (runIteration(mImpl->root, *mImpl->rootNode, mImpl->rootPlayer,
			mImpl->config, mImpl->validationGeneration, mImpl->random, shouldStop,
			stopped, horizonCutoff, forcedMovesApplied, mImpl->timings))
		{
			mImpl->iterationsCompleted++;
			if (horizonCutoff) mImpl->turnHorizonCutoffs++;
			mImpl->forcedMovesApplied += forcedMovesApplied;
		}
		else if (!stopped)
			mImpl->failedIterations++;
		else
			break;
	}
	return isComplete();
}

bool MctsSession::isStarted() const
{
	return mImpl->started;
}

bool MctsSession::isComplete() const
{
	return mImpl->started &&
		(mImpl->iterationsCompleted + mImpl->failedIterations >= mImpl->config.iterations ||
		 mImpl->timeExpired());
}

MctsResult MctsSession::result() const
{
	if (!mImpl->started) return MctsResult();
	int attempted = mImpl->iterationsCompleted + mImpl->failedIterations;
	MctsResult result = collectResult(*mImpl->rootNode, mImpl->rootPlayer,
		mImpl->iterationsCompleted, mImpl->failedIterations,
		mImpl->timeExpired() && attempted < mImpl->config.iterations,
		mImpl->turnHorizonCutoffs, mImpl->forcedMovesApplied);
	result.reusedTree = mImpl->reusedTree;
	result.reusedRootVisits = mImpl->reusedRootVisits;
	const double nanosecondsToMilliseconds = 1.0 / 1000000.0;
	result.cloneTimeMs = mImpl->timings.cloneNs * nanosecondsToMilliseconds;
	result.treeEnumerationTimeMs =
		mImpl->timings.treeEnumerationNs * nanosecondsToMilliseconds;
	result.rolloutEnumerationTimeMs =
		mImpl->timings.rolloutEnumerationNs * nanosecondsToMilliseconds;
	result.rolloutSelectionTimeMs =
		mImpl->timings.rolloutSelectionNs * nanosecondsToMilliseconds;
	result.actionExecutionTimeMs =
		mImpl->timings.actionExecutionNs * nanosecondsToMilliseconds;
	result.evaluationTimeMs = mImpl->timings.evaluationNs * nanosecondsToMilliseconds;
	result.luaCallbackTimeMs =
		mImpl->timings.luaCallbackNs * nanosecondsToMilliseconds;
	return result;
}

MctsSearch::MctsSearch(int rootPlayer, const MctsConfig& config)
	: mRootPlayer(rootPlayer), mConfig(config)
{
}

MctsResult MctsSearch::search(Duel& root)
{
	MctsSession session(mRootPlayer, mConfig);
	if (!session.start(root)) return MctsResult();
	session.advance(mConfig.iterations);
	return session.result();
}
