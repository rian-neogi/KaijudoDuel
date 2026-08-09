# Monte Carlo Tree Search design

This document describes the intended MCTS architecture. The Lua state migration,
stable-boundary `Duel` cloning, scoped active-duel switching, synchronous
choice resolution, and complete decision-plan enumeration described
below are implemented. The initial single-threaded, full-information MCTS tree,
live-game plan commitment, and AI-driver integration are also implemented.

## Core Lua invariant

At every stable game boundary, all mutable gameplay state must be owned by the
C++ `Duel` being played or simulated. Lua may contain rule code and immutable
rule metadata, but it must not contain mutable gameplay state that survives a
completed action.

A stable boundary has all of these properties:

- No Lua callback is running or suspended.
- Every choice belonging to the action has been resolved.
- The engine message queue has been drained, including reactive card callbacks.
- The duel is ready for the next player decision or has reached a terminal state.

Lua locals are allowed for calculations within a callback. Locals may also live
while a spell resolves several synchronous choices because the original
`OnCast` call has not returned yet. They must not be needed after the complete
action and its callbacks finish.

The following Lua values are allowed to persist:

- `Cards`, `Abils`, `Checks`, and `Functions` rule tables.
- Card metadata and constants.
- Helper functions and immutable tables constructed while loading the rules.
- Read-only closures referenced by active C++ modifiers.

The following are forbidden:

- Mutable global or module-local card state.
- A modifier closure that mutates a captured Lua variable or table.
- Lua random-number state used for game outcomes.
- A coroutine, yielded callback, or unresolved choice at an MCTS node.

The existing persistent values for spell-cast tracking, Bailas Gale triggers,
and Storm Wrangler block counts are now stored in `Duel::mLuaRuleState`.
Bombazar's extra-turn flag is stored in the C++ `Modifier` that owns the effect.
The Lua bridge provides indexed integer state access for both kinds of state.
An optional third table passed to `createModifier` is copied immediately into
the new C++ modifier; Lua does not retain that table.

Any new card that needs data after its callback returns must put that data in
the `Duel`, card, or modifier. The rule deciding how to use the data can remain
in Lua.

## One Lua VM, two C++ duels

Sequential MCTS needs one process-wide Lua VM and two C++ duel instances:

- The live `Duel`, which is authoritative for the real match.
- A simulation `Duel`, which is repeatedly restored to an MCTS root state.

Before calling Lua, the engine points `ActiveDuel` at the duel being operated
on. Each Lua bridge function consequently reads and writes that duel's C++
state. `ActiveDuelGuard` switches `ActiveDuel` for a scoped simulation call and
restores the previous pointer on every exit path. Guards may be nested. A caller
must have exclusive ownership of the shared Lua VM for the complete guarded
operation. Live operations establish that through the duel lock; the background
search receives exclusive ownership while the live duel is explicitly frozen.

The Lua VM is not reset between rollouts. At a stable boundary it retains rule
definitions and Lua function objects, but no mutable state whose value defines
the game position. Restoring the simulation `Duel`, including its RNG and rule
state, therefore restores the semantic game state.

The shared VM is used serially. At a stable AI boundary, the main thread caches
the effective powers needed by rendering, captures the search root, and marks
the live duel as AI-thinking. The live input loop then stops dispatching rules,
and gameplay input that could query Lua is disabled. A single background worker
may use `LuaCards` and switch `ActiveDuel` among its private simulations without
holding the live-duel mutex. Rendering continues from the frozen live C++ state
and cached powers. The worker returns only a plan and statistics; the main
thread joins it, restores live Lua ownership, validates the plan, and commits it.

Parallel rollouts would still require one Lua VM per worker or a replacement for
the global bridge architecture.

## Modifiers and read-only closures

`createModifier(card, function)` stores a Lua registry reference in a C++
`Modifier`. Many of these functions capture an immutable choice such as an
owner, race, or selected card ID. This is safe with one Lua VM:

- The captured value never changes during a rollout.
- The function queries whichever C++ duel is currently in `ActiveDuel`.
- Live and simulated cards preserve the same unique IDs at the root.

Mutable captures are not safe because resetting C++ would not reset the
captured value. Bombazar was the existing instance of this pattern and its
mutable flag has been moved into its C++ modifier.

Every cloned modifier receives an independent Lua registry reference to the
same read-only Lua function. The root and simulated modifiers can consequently
be destroyed in either order without invalidating one another. Their C++ rule
state maps are also independent copies.

A modifier made only during a rollout belongs only to that simulated state and
must release its reference when the rollout state is discarded. Destroying a
simulated modifier must never invalidate the corresponding live/root modifier.
Modifier C++ rule state must be copied along with the modifier.

## Decisions and choices

An MCTS edge represents a complete decision by the acting player, not just the
first UI click. A decision plan can contain:

```text
primary action      cast, summon, charge mana, attack, tap ability, or end turn
payment             mana-card IDs, when applicable
choices             targets and button answers produced while resolving it
```

For example, casting Crimson Hammer and selecting its target is one decision
plan. A spell with two related targets contains both choices in the same plan.
`DecisionPlan::action` stores the complete primary engine message, so evolution
bait IDs are already part of a card-play action. `manaCards` stores a canonical
ascending list of the unique IDs tapped to pay for it. Each ordered
`DecisionChoice` stores both the player who owns the choice and the selected
card or button value.

`enumerateDecisionPlans` begins with every primary message returned by
`Duel::getPossibleMoves`. It replays each partial plan on a fresh clone of the
same root. Execution reports one of four outcomes:

- `Complete`: the action and all of its payment and choices resolved at a stable
  boundary.
- `NeedsMana`: the cast is still awaiting payment, together with the legal next
  mana-card IDs.
- `NeedsChoice`: Lua requested another answer, together with its owning player
  and current legal buttons/cards.
- `Illegal`: the primary action, a supplied payment, or a supplied choice no
  longer matches the replayed position.

Enumeration extends a `NeedsMana` or `NeedsChoice` prefix once for every legal
answer and replays each child from the root. This restart is essential: probing
a missing Lua choice returns from the current `OnCast`, so that particular
scratch duel is deliberately discarded. Replaying the prefix re-enters
`OnCast`, supplies all earlier answers synchronously, and reaches the next
choice with its Lua locals reconstructed normally.

Mana payment order has no gameplay meaning in the current rules. Restricting
payments to ascending card IDs therefore enumerates every legal payment set
once rather than enumerating all permutations. Every intermediate tap is still
checked through `Duel::getPossibleMoves` and `canTapManaForCasting`, so
multi-civilization coverage is enforced by the engine rather than duplicated
in the AI layer.

During simulation, `createChoice` uses the `Duel`'s synchronous simulation
resolver. The resolver sees the active choice and its validated card list and
supplies the next planned selection without entering the live input loop. The
engine validates the answer, releases the transient `Choice`, and continues the
same `OnCast` or `HandleMessage` invocation. Temporary locals connecting the
first and second choices remain valid until that callback returns, after which
they disappear normally.

If a callback queued engine messages before requesting its next choice,
`createChoice` drains those messages first and then restores the outer callback's
`mCurrentMessage`. The nested effects remain applied to the `Duel`, while later
code in the same callback continues to observe the message that invoked it.

Simulation selections do not enqueue the UI-only `choiceselect` message. No Lua
rule reacts to that message; omitting it also prevents re-entering the Lua VM if
a callback immediately creates a second choice. A missing resolver or illegal
answer returns `RETURN_QUIT`, cleans up the choice, and sets a sticky simulation
choice-failure flag. The rollout executor must discard such a plan. Restoring a
root clears the failure flag while preserving the simulation duel's configured
resolver.

The tree does not recursively launch another rollout while Lua is on the stack.
Complete plans are enumerated first and organized into a trie. Its first level
groups the primary action and canonical mana payment. Each later level is one
ordered choice and records the player who owns it. A complete plan is stored
only at a leaf. Tree traversal chooses a leaf, replays it synchronously, and
reaches the next stable C++ position.

Choice-trie nodes owned by the root player maximize the root-perspective value;
nodes owned by the opponent minimize it. The AI therefore cannot improve a
spell's score by selecting a cooperative answer on the opponent's behalf. This
works without treating an unresolved Lua callback as a clonable MCTS state.

`commitDecisionPlan` preflights the complete selected leaf on a simulation
clone. If valid, it queues the primary action and exact mana payment on the live
duel. It installs a bounded resolver containing only the leading choices owned
by the AI actor. Those answers resolve synchronously on the normal duel thread.
The resolver clears itself after its final expected answer. If the next choice
belongs to the human, no simulated answer is submitted: the existing live
choice wait and UI take over. Any later AI choice following that human answer
is handled as a new live AI decision rather than assuming the simulated human
branch occurred.

## Snapshot contract

`Duel::copyFrom` now deeply and deterministically reproduces:

- cards and all mutable card fields;
- every zone and zone order;
- evolution-stack links remapped to the cloned cards;
- modifiers, modifier C++ rule state, and safe Lua-function references;
- turn, phase, attack, casting, and temporary combat state;
- shield-break and cards-drawn trackers;
- `mLuaRuleState`;
- player types, winner state, and unique-ID allocation;
- the duel RNG state;
- message history and the current message.

`isCloneable` and `copyFrom` reject positions with an active `Choice`, a
suspended callback, a failed simulation choice, a non-empty message queue, a
pending zero-power pass, an active race query, a non-empty Lua call stack, or
invalid card unique IDs. This enforces the stable-boundary invariant instead of
pretending to copy a live Lua stack. A failed copy leaves the destination
unchanged.

`mIsSimulation`, `mInputLoopRunning`, and the choice resolver are
deliberately not copied. They are execution-context settings, not game-position
data: the reusable rollout duel must remain a simulation with no live input
loop and retain its resolver even when its root came from a live duel. The
choice-failure flag is cleared on a successful restore. Deck RNG pointers are
rebound to the destination RNG after its full generator state is copied.

## Hidden information (deferred)

The first MCTS implementation will intentionally use full information. This is
useful for validating action execution, tree behavior, and evaluation, but it
means that version of the AI may use the real contents of hidden zones.

Before the MCTS becomes the fair production opponent, it must not inspect the
actual identities or order of cards hidden from it. Copying the live duel
verbatim and evaluating the opponent's real hand or shield contents would make
the AI cheat.

Each rollout should therefore use a determinization consistent with the AI's
observations:

1. Preserve public cards and cards previously revealed to the AI.
2. Collect identities still unknown to the AI without exposing their live
   allocation to tree policy or evaluation.
3. Sample those identities among the opponent's hidden hand, shields, and deck
   using the simulation duel's RNG.
4. Resample on later rollouts so the selected move is robust across plausible
   hidden positions.

Tree keys and statistics should represent the AI's information state rather
than the secretly known live arrangement. During a determinized rollout, the
simulated opponent may use the sampled cards in its own hand because that
opponent would know its own hand.

## Search loop

At an AI decision boundary:

1. Capture the observable root and initialize the root node.
2. For each iteration, restore the simulation duel. The initial full-information
   implementation skips hidden-state sampling.
3. Select children using UCT or another exploration policy.
4. Expand one legal decision plan not yet represented at that node.
5. Continue with a rollout policy until a terminal state or depth/time limit.
6. Evaluate the final position from the root AI's perspective.
7. Backpropagate the result through every visited node.
8. Commit the legal root plan with the strongest visit evidence to the live
   duel.

`MctsSearch` implements steps 1 through 7 with deterministic
iteration and decision-depth budgets. Its configurable inputs are the iteration
count, maximum complete decisions per simulation, UCT exploration constant, and
search seed. Every iteration creates a simulation duel, restores the root, and
replays the selected `DecisionPlan` path instead of storing Lua execution state
inside tree nodes.

Each stable node lazily enumerates all complete plans for its reconstructed
position and builds its action-and-choice trie. UCT runs at every trie level.
Nodes controlled by the root player maximize the root-perspective mean value;
nodes controlled by the opponent negate that exploitation term and therefore
minimize the same value. A leaf expands to the next stable `Duel` node. This
handles choices embedded inside one Lua callback, several consecutive decisions
by one player, and combat responses that temporarily pass control.

The initial rollout policy chooses uniformly at each action and choice-trie
level, rather than weighting an option by how many complete leaves happen to
exist beneath it. A
terminal win is `1`, a loss is `-1`, and a depth-limited position receives a
bounded material evaluation based on shields, hand size, mana, deck reserve,
and modifier-aware battle-zone creature value. The same root and seed reproduce
the same search. The selected root plan is the child with the most visits, with
mean value as a deterministic tie-breaker.

The live AI driver attempts at most 64 iterations with a maximum rollout depth
of 12 complete decisions and a 1500-millisecond wall-clock budget.
`BackgroundMctsSearch` runs the persistent `MctsSession` on a worker thread
while the UI continues rendering the frozen live duel. Effective
creature powers are refreshed before every background search, rather than only
once per turn, so summons and continuous effects from earlier AI actions are
visible at the next stable boundary. Before cloning or starting the worker, the
driver checks the engine's root move list. If it contains exactly one legal
action, that action is queued immediately and the terminal reports zero
rollouts. Deadline checks also occur during complete-plan enumeration and
between simulation steps, so an expensive partially enumerated iteration can
stop without being counted as failed. The best visited root plan is returned
when time expires. If no plan was visited, the position is not cloneable, or
the selected plan fails validation, the driver queues one action from
`HeuristicBot` instead. This is especially important for legacy suspended
choices, which are intentionally not MCTS roots.

Mana placement and payment are heuristic rather than MCTS branches. At a live
mana-placement boundary, `HeuristicBot` either charges its highest-scoring card
immediately or declines to charge; declined `cardmana` actions are excluded from
that search. During plan enumeration, a cast follows one deterministic legal
payment sequence that attempts to preserve untapped civilization coverage and
the civilization combinations needed by other cards in hand. Standalone live
payment boundaries, including those reached through the heuristic fallback,
are also completed by this policy without starting MCTS. Spell targets and
other ordered choices remain tree decisions. Shield selection during an attack
is an exception: the live AI chooses uniformly among the legal shields through
the Duel RNG, and simulated target nodes expose one seeded random shield rather
than allowing MCTS to optimize against face-down shield identities.

When a session completes, the terminal reports completed and attempted
rollouts, failed rollouts, elapsed wall time, whether the budget expired, the
root evaluation, number of root actions, selected action, selected action
evaluation, and visit count.
Evaluations are normalized to `[-1, 1]` from the AI's perspective.

This first tree deliberately has no transposition table, persistent tree reuse,
general heuristic rollout policy, or hidden-information determinization. Those
remain later refinements.

Opponent nodes invert the value or otherwise optimize for the opponent. Chance
outcomes are produced only through the simulation duel's cloned RNG. Search
reproducibility should be available by setting an explicit seed in tests.

The initial rollout policy can reuse improved phase-aware heuristics. MCTS then
provides lookahead while the heuristic supplies inexpensive move ordering,
rollout choices, and non-terminal evaluation.

## Why rollouts do not require a Lua reset

At the beginning of a rollout, Lua has an empty call stack and immutable loaded
rules. All state that can change future outcomes belongs to the simulation
duel. During an action, Lua may create temporary locals and resolve synchronous
choices. When the action finishes, those locals are unreachable unless they
belong to a read-only modifier closure. Persistent counters and flags are in
C++ and are restored with the snapshot.

Consequently, switching `ActiveDuel` to a restored simulation duel is
semantically equivalent to restoring the rules engine. Lua garbage-collection
timing and registry slot numbers do not affect game results, provided registry
references have correct C++ ownership.

## Implementation order

1. Define the `DecisionPlan` representation and enumerate legal full-information
   plans, including payments and ordered choices. **Implemented.**
2. Implement selection, expansion, rollout, evaluation, and backpropagation.
   **Implemented for the initial full-information tree.**
3. Commit selected plans through a validated live-game action path that mirrors
   the simulation executor while preserving normal UI behavior. **Implemented.**
4. Integrate selected plans with the AI turn driver and retain a legal fallback.
   **Implemented with a bounded live budget and `HeuristicBot` fallback.**
5. Add deterministic card-specific tests for modifier creation/destruction,
   extra turns, shield triggers, and repeated rollouts. Cloning, synchronous
   multi-choice resolution, canonical exhaustive mana enumeration, heuristic
   MCTS mana payment, bounded search, dependent ordered choices, and context
   restoration already have engine-level smoke coverage.
6. Add determinization for hidden zones and remove hidden identities from tree
   policy and evaluation before treating MCTS as a fair production opponent.

`executeDecisionPlan` remains simulation-only and validates the primary action,
every mana tap, the owner and value of every choice, and the final stable
boundary. `commitDecisionPlan` uses that executor for preflight, then queues the
validated action on a live AI-controlled duel without setting simulation mode.

Only one search worker may own the shared Lua VM. Parallel rollouts can be
considered only after each worker has an independent VM and execution context.
