#include "Mcts.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <random>

namespace
{
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
		std::unique_ptr<PlanNode> plans;

		MctsNode() : player(-1), visits(0), valueSum(0.0), initialized(false)
		{
		}
	};

	template <typename Node>
	double meanValue(const Node& node)
	{
		return node.visits == 0 ? 0.0 : node.valueSum / node.visits;
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

	double creatureValue(Duel& duel, Card* card)
	{
		int uid = card->mUniqueId;
		double value = 5.0 + std::max(0, duel.getCreaturePower(uid)) / 700.0;
		value += std::max(1, duel.getCreatureBreaker(uid)) * 1.5;
		if (duel.getCreatureIsBlocker(uid)) value += 1.5;
		if (!card->mIsTapped) value += 0.35;
		return value;
	}

	double playerValue(Duel& duel, int player)
	{
		double value = duel.mShields[player].mCards.size() * 6.0;
		value += duel.mHands[player].mCards.size() * 1.75;
		value += duel.mManazones[player].mCards.size() * 1.1;
		value += duel.mDecks[player].mCards.size() * 0.05;
		for (std::vector<Card*>::const_iterator card = duel.mBattlezones[player].mCards.begin();
			card != duel.mBattlezones[player].mCards.end(); ++card)
		{
			if ((*card)->mType == TYPE_CREATURE)
				value += creatureValue(duel, *card);
		}
		return value;
	}

	double evaluate(Duel& duel, int rootPlayer)
	{
		if (duel.mWinner == rootPlayer) return 1.0;
		if (duel.mWinner == 1 - rootPlayer) return -1.0;
		ActiveDuelGuard activeGuard(duel);
		double difference = playerValue(duel, rootPlayer) - playerValue(duel, 1 - rootPlayer);
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
		options.randomShieldTarget = true;
		options.randomIndex = [&random](size_t count) -> size_t
		{
			std::uniform_int_distribution<size_t> choice(0, count - 1);
			return choice(random);
		};
		options.shouldStop = shouldStop;
		std::vector<DecisionPlan> plans = enumerateDecisionPlans(position, options);
		if (shouldStop && shouldStop()) return false;
		std::unique_ptr<PlanNode> plansRoot(new PlanNode());
		if (!buildPlanTree(plans, player, *plansRoot, shouldStop)) return false;
		node.player = player;
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

	bool selectRandomPlan(const std::vector<DecisionPlan>& plans, int player,
		std::mt19937& random, DecisionPlan& selected,
		const std::function<bool()>& shouldStop)
	{
		std::unique_ptr<PlanNode> root(new PlanNode());
		if (!buildPlanTree(plans, player, *root, shouldStop)) return false;
		PlanNode* node = root.get();
		while (!node->leaf)
		{
			if (shouldStop && shouldStop()) return false;
			if (node->children.empty()) return false;
			std::uniform_int_distribution<size_t> choice(0, node->children.size() - 1);
			node = node->children[choice(random)].get();
		}
		selected = node->plan;
		return true;
	}

	bool executeCompletePlan(Duel& duel, const DecisionPlan& plan)
	{
		return executeDecisionPlan(duel, plan).status == DecisionPlanStatus::Complete;
	}

	bool rollout(Duel& position, int& depth, int maxDepth, std::mt19937& random,
		const std::function<bool()>& shouldStop, bool& stopped)
	{
		while (position.mWinner == -1 && depth < maxDepth)
		{
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return false;
			}
			DecisionPlanEnumerationOptions options;
			options.heuristicMana = true;
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
			if (!selectRandomPlan(plans, player, random, selected, shouldStop) ||
				!executeCompletePlan(position, selected))
			{
				if (shouldStop && shouldStop()) stopped = true;
				return false;
			}
			++depth;
		}
		return true;
	}

	bool runIteration(Duel& root, MctsNode& rootNode, int rootPlayer,
		const MctsConfig& config, std::mt19937& random,
		const std::function<bool()>& shouldStop, bool& stopped)
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
		int depth = 0;
		while (position.mWinner == -1 && depth < config.maxDepth)
		{
			if (!initializeNode(*node, position, shouldStop, random))
			{
				if (shouldStop && shouldStop()) stopped = true;
				if (stopped) return false;
				break;
			}
			if (node->plans == NULL ||
				node->plans->children.empty())
				break;

			PlanNode* leaf = selectPlanLeaf(*node->plans, rootPlayer, config.exploration,
				random, planPath);
			if (leaf == NULL || !executeCompletePlan(position, leaf->plan)) return false;
			++depth;
			if (shouldStop && shouldStop())
			{
				stopped = true;
				return false;
			}

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

		if (!rollout(position, depth, config.maxDepth, random, shouldStop, stopped)) return false;
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
		int iterationsCompleted, int failedIterations, bool timeBudgetExpired)
	{
		MctsResult result;
		result.iterationsCompleted = iterationsCompleted;
		result.failedIterations = failedIterations;
		result.timeBudgetExpired = timeBudgetExpired;
		result.meanValue = meanValue(rootNode);
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
	  meanValue(0.0),
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
	bool started;
	std::chrono::steady_clock::time_point deadline;
	bool hasDeadline;

	Impl(int player, const MctsConfig& searchConfig)
		: rootPlayer(player), config(searchConfig), random(searchConfig.seed),
		  iterationsCompleted(0), failedIterations(0), started(false), hasDeadline(false)
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
		if (runIteration(mImpl->root, mImpl->rootNode, mImpl->rootPlayer,
			mImpl->config, mImpl->random, shouldStop, stopped))
			mImpl->iterationsCompleted++;
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
		mImpl->timeExpired() && attempted < mImpl->config.iterations);
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
