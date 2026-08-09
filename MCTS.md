# Monte Carlo Tree Search design

This document describes the intended MCTS architecture. The Lua state migration
and stable-boundary `Duel` cloning described below are implemented, but the MCTS
search and simulation choice resolver are not yet implemented.

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
state. An RAII guard should switch `ActiveDuel` for a simulation and restore the
previous pointer on every success and error path.

The Lua VM is not reset between rollouts. At a stable boundary it retains rule
definitions and Lua function objects, but no mutable state whose value defines
the game position. Restoring the simulation `Duel`, including its RNG and rule
state, therefore restores the semantic game state.

The shared VM must be used serially. A rollout must not run while rendering,
the live input loop, or another worker is executing Lua. The existing `gMutex`
rule still applies to the live duel. If parallel rollouts are added later, each
worker will need its own Lua VM and execution context, or the global bridge
architecture will need to be replaced.

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
payment             mana cards and evolution bait, when applicable
choices             targets and button answers produced while resolving it
```

For example, casting Crimson Hammer and selecting its target is one decision
plan. A spell with two related targets contains both choices in the same plan.

During simulation, `createChoice` must use a synchronous simulation resolver.
It validates and supplies the next choice without entering the live input loop.
Lua then continues the same `OnCast` or `HandleMessage` invocation. Temporary
locals connecting the first and second choices remain valid until that callback
returns, after which they disappear normally.

The tree does not recursively launch another rollout while Lua is on the stack.
When a choice is encountered, tree traversal selects or expands a choice child,
returns that answer synchronously, and lets the current action finish. A
different target is explored on a later iteration after restoring the root.

If control passes to the opposing player, their decision is a separate tree or
rollout-policy decision. In the real game, an AI plan supplies only choices
owned by the AI; a human-owned choice returns control to the UI.

Before committing the selected plan to the live duel, every payment and choice
must be validated again. If the live state does not match the plan, the engine
must discard it and search again or use a legal fallback.

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
suspended callback, a non-empty message queue, a pending zero-power pass, an
active race query, a non-empty Lua call stack, or invalid card unique IDs. This
enforces the stable-boundary invariant instead of pretending to copy a live Lua
stack. A failed copy leaves the destination unchanged.

`mIsSimulation` and `mInputLoopRunning` are deliberately not copied. They are
execution-context settings, not game-position data: the reusable rollout duel
must remain a simulation with no live input loop even when its root came from a
live duel. Deck RNG pointers are rebound to the destination RNG after its full
generator state is copied.

## Hidden information

The AI must not inspect the actual identities or order of cards hidden from it.
Copying the live duel verbatim and evaluating the opponent's real hand or shield
contents would make the AI cheat.

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
2. For each iteration, restore the simulation duel and sample hidden state.
3. Select children using UCT or another exploration policy.
4. Expand one legal decision plan not yet represented at that node.
5. Continue with a rollout policy until a terminal state or depth/time limit.
6. Evaluate the final position from the root AI's perspective.
7. Backpropagate the result through every visited node.
8. Commit the legal root plan with the strongest visit evidence to the live
   duel.

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

1. Introduce an `ActiveDuel` RAII guard and serialize all access to the Lua VM.
2. Define the `DecisionPlan` representation and enumerate legal plans without
   inspecting opposing hidden identities.
3. Add the synchronous simulation choice resolver and stable-boundary checks.
4. Add determinization for hidden zones.
5. Implement selection, expansion, rollout, evaluation, and backpropagation.
6. Commit selected plans through the same validated live-game action path used
   by ordinary input.
7. Add deterministic card-specific tests for multi-choice spells, modifier
   creation/destruction, extra turns, shield triggers, and repeated rollouts.

The first implementation should remain single-threaded. Parallelism can be
considered only after the single-VM version is deterministic and its state
isolation tests pass.
