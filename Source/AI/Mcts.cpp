#include "Mcts.h"

#include "AiScoring.h"
#include "HeuristicBot.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <random>

namespace
{
	const double ROLLOUT_COMBAT_TEMPERATURE = 20.0;
	const double ROLLOUT_UNIFORM_EXPLORATION = 0.05;
	const int MAX_EXTRA_TURN_DEPTH_EXTENSIONS = 2;

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
		bool initialized;
		bool hasForcedPlan;
		DecisionPlan forcedPlan;
		std::unique_ptr<MctsNode> forcedChild;
		std::unique_ptr<PlanNode> plans;

		MctsNode()
			: player(-1), visits(0), valueSum(0.0), initialized(false),
			  hasForcedPlan(false)
		{
		}
	};

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
				extraTurnDepthExtensions < MAX_EXTRA_TURN_DEPTH_EXTENSIONS)
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
		if (duel.mWinner == rootPlayer) return 1.0;
		if (duel.mWinner == 1 - rootPlayer) return -1.0;
		double difference = AiScoring::playerValue(duel, rootPlayer) -
			AiScoring::playerValue(duel, 1 - rootPlayer);
		return std::tanh(difference / 24.0);
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
							exponentials[i] = std::exp(
								(scores[i] - maximumScore) / ROLLOUT_COMBAT_TEMPERATURE);
							exponentialSum += exponentials[i];
						}
						for (size_t i = 0; i < combatActions.size(); ++i)
						{
							if (!std::isfinite(scores[i]))
							{
								weights[combatActions[i]] = 0.0;
								continue;
							}
							double probability =
								(1.0 - ROLLOUT_UNIFORM_EXPLORATION) *
									exponentials[i] / exponentialSum +
								ROLLOUT_UNIFORM_EXPLORATION / finiteCount;
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
		int& forcedMovesApplied)
	{
		while (position.mWinner == -1 && depth < horizon.depthLimit(maxDepth) &&
			!horizon.cutoff)
		{
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return false;
			}
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
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return false;
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
			else if (!selectRolloutPlan(position, plans, player, random, selected, shouldStop))
			{
				if (shouldStop && shouldStop()) stopped = true;
				return false;
			}
			int previousTurn = position.mTurn;
			if (!executeCompletePlan(position, selected))
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
		const MctsConfig& config, std::mt19937& random,
		const std::function<bool()>& shouldStop, bool& stopped, bool& horizonCutoff,
		int& forcedMovesApplied)
	{
		if (shouldStop && shouldStop())
		{
			stopped = true;
			return false;
		}
		Duel position;
		position.mIsSimulation = true;
		position.mInputLoopRunning = false;
		if (!position.copyFrom(root)) return false;

		std::vector<MctsNode*> statePath;
		std::vector<PlanNode*> planPath;
		MctsNode* node = &rootNode;
		statePath.push_back(node);
		TurnHorizon horizon(root, rootPlayer);
		int depth = 0;
		while (position.mWinner == -1 && depth < horizon.depthLimit(config.maxDepth) &&
			!horizon.cutoff)
		{
			if (!initializeNode(*node, position, shouldStop, random))
			{
				if (shouldStop && shouldStop()) stopped = true;
				if (stopped) return false;
				break;
			}
			if (node->hasForcedPlan)
			{
				int previousTurn = position.mTurn;
				if (!executeCompletePlan(position, node->forcedPlan)) return false;
				forcedMovesApplied++;
				++depth;
				horizon.observe(position, startedImmediateExtraTurn(node->forcedPlan,
					previousTurn, position));
				if (shouldStop && shouldStop())
				{
					stopped = true;
					return false;
				}
				if (position.mWinner != -1 ||
					depth >= horizon.depthLimit(config.maxDepth) || horizon.cutoff)
					break;
				if (node->forcedChild == NULL)
					node->forcedChild.reset(new MctsNode());
				node = node->forcedChild.get();
				statePath.push_back(node);
				continue;
			}
			if (node->plans == NULL ||
				node->plans->children.empty())
				break;

			PlanNode* leaf = selectPlanLeaf(*node->plans, rootPlayer, config.exploration,
				random, planPath);
			int previousTurn = position.mTurn;
			if (leaf == NULL || !executeCompletePlan(position, leaf->plan)) return false;
			++depth;
			horizon.observe(position, startedImmediateExtraTurn(leaf->plan, previousTurn, position));
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return false;
			}
			if (horizon.cutoff) break;

			if (leaf->stateChild == NULL)
			{
				leaf->stateChild.reset(new MctsNode());
				{
					ActiveDuelGuard activeGuard(position);
					leaf->stateChild->player = position.getPlayerToMove();
				}
				node = leaf->stateChild.get();
				statePath.push_back(node);
				break;
			}

			node = leaf->stateChild.get();
			statePath.push_back(node);
		}

		if (!rollout(position, depth, config.maxDepth, random, shouldStop, stopped, horizon,
			forcedMovesApplied))
			return false;
		horizonCutoff = horizon.cutoff;
		double reward = evaluate(position, rootPlayer);
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
	: iterations(256), maxDepth(48), timeBudgetMs(0), exploration(std::sqrt(2.0)),
	  seed(0x4b41494aU)
{
}

MctsChildStatistics::MctsChildStatistics() : visits(0), meanValue(0.0)
{
}

MctsResult::MctsResult()
	: hasPlan(false), iterationsCompleted(0), failedIterations(0), timeBudgetExpired(false),
	  turnHorizonCutoffs(0), forcedMovesApplied(0), meanValue(0.0),
	  selectedVisits(0), selectedMeanValue(0.0)
{
}

struct MctsSession::Impl
{
	int rootPlayer;
	MctsConfig config;
	Duel root;
	MctsNode rootNode;
	std::mt19937 random;
	int iterationsCompleted;
	int failedIterations;
	int turnHorizonCutoffs;
	int forcedMovesApplied;
	bool started;
	std::chrono::steady_clock::time_point deadline;
	bool hasDeadline;

	Impl(int player, const MctsConfig& searchConfig)
		: rootPlayer(player), config(searchConfig), random(searchConfig.seed),
		  iterationsCompleted(0), failedIterations(0), turnHorizonCutoffs(0),
		  forcedMovesApplied(0), started(false), hasDeadline(false)
	{
		root.mIsSimulation = true;
		root.mInputLoopRunning = false;
		rootNode.player = player;
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
	if (!mImpl->root.copyFrom(root)) return false;
	mImpl->started = true;
	return true;
}

bool MctsSession::advance(int iterationBudget)
{
	if (!mImpl->started) return false;
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
		if (runIteration(mImpl->root, mImpl->rootNode, mImpl->rootPlayer,
			mImpl->config, mImpl->random, shouldStop, stopped, horizonCutoff,
			forcedMovesApplied))
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
	return collectResult(mImpl->rootNode, mImpl->rootPlayer,
		mImpl->iterationsCompleted, mImpl->failedIterations,
		mImpl->timeExpired() && attempted < mImpl->config.iterations,
		mImpl->turnHorizonCutoffs, mImpl->forcedMovesApplied);
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
