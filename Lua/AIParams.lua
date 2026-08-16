-- AI tuning values are loaded into C++ once when the card rules initialize.
-- This table is configuration, not duel state: AI code never mutates it.
AIParams = {
	evaluation = {
		-- Total non-terminal leaf-score value for exactly 1 through 5 shields.
		-- Each shield count can be tuned independently.
		shield_count_1_value = 6.0,
		shield_count_2_value = 12.0,
		shield_count_3_value = 18.0,
		shield_count_4_value = 22.0,
		shield_count_5_value = 25.0,
		-- Added for every shield beyond five, on top of shield_count_5_value.
		shield_above_5_value = 2.0,
		-- Bonus when the player has a guaranteed immediate KO through optimal blocks.
		knockout_bonus = 6.0,
		-- Base value contributed by each card in the mana zone.
		mana_card_value = 3.0,
		-- One-time bonus for each distinct civilization represented in mana.
		mana_civilization_bonus = 0.1,
		-- Minimum value of a card in hand, even when it is unaffordable.
		hand_base_value = 2.0,
		-- Extra hand value per point of the card's printed mana cost.
		hand_cost_bonus = 0.05,
		-- Hand-value penalty for each mana still missing from the card's cost.
		hand_missing_mana_penalty = 0.1,
		-- Small reserve value per card left in deck; discourages needless milling.
		deck_card_value = 0.00,
		-- Creature power is divided by this before entering the leaf score.
		creature_power_divisor = 1000.0,
		-- Leaf-score value of each shield the creature can break per attack.
		creature_breaker_value = 3.0,
		-- Divides the raw player-value difference before tanh normalization.
		-- Larger values keep evaluations nearer zero and delay saturation.
		normalization_scale = 30.0,
		-- Exact rewards backpropagated for terminal wins and losses.
		win_value = 1.0,
		loss_value = -1.0,
		-- Sentinel score used to reject an illegal mana-placement candidate.
		invalid_mana_delta = -1000000.0,
	},

	search = {
		-- Hard ceiling on completed plus failed MCTS attempts per decision.
		max_rollouts = 1024,
		-- Maximum complete decisions in an ordinary simulation trajectory.
		max_depth = 12,
		-- Generic/test search deadline. Zero means no wall-clock deadline.
		default_time_budget_ms = 0,
		-- Live-play deadlines for ordinary and combat-phase decisions.
		main_time_budget_ms = 1500,
		combat_time_budget_ms = 2500,
		-- C in UCT: mean + C * sqrt(log(parent visits) / child visits).
		-- Larger values spend more visits exploring less-tested actions.
		uct_exploration = math.sqrt(2.0),
		-- Initial deterministic seed used by MCTS and rollout sampling.
		random_seed = 0x4b41494a,
		-- Softmax temperature for rollout combat scores. Higher is flatter;
		-- lower values make rollouts follow the best heuristic action more often.
		rollout_combat_temperature = 20.0,
		-- Fraction of combat rollout probability reserved for uniform exploration.
		rollout_uniform_exploration = 0.1,
		-- Number of extra full depth allowances granted for consecutive extra turns.
		max_extra_turn_depth_extensions = 2,
		-- Safety cap when the heuristic repeatedly taps mana for one cast.
		max_mana_payment_steps = 40,
	},

	heuristic = {
		-- At or below this hand size, excess mana encourages skipping a charge.
		low_hand_card_count = 4,

		card = {
			-- Identity-independent value assigned to an opposing hidden card.
			hidden_value = 1.0,
			-- Base value shared by visible creatures and spells.
			base_value = 1.0,
			-- General card-value bonus per printed mana cost.
			cost_weight = 0.7,
			-- Visible creature power is divided by this for heuristic card value.
			power_divisor = 1000.0,
			-- Heuristic card-value bonus per breaker count.
			breaker_weight = 0.8,
			-- Breaker floor used when valuing creatures with missing/bad metadata.
			minimum_breaker = 1,
			-- Additional visible card value for being a blocker.
			blocker_bonus = 1.5,
			-- Extra cost-based value applied only to spells.
			spell_cost_weight = 0.5,
			-- Additional value for a card with the shield-trigger property.
			shield_trigger_bonus = 1.0,
		},

		choice = {
			-- Dominating score for a card-authored AiPreferredChoice result.
			preferred_score = 100000.0,
			-- Score for a choice whose card ID is outside the duel card list.
			invalid_score = -1000.0,
			-- Scores for the first button when its prompt mentions draw/use,
			-- for other first-button prompts, and for later buttons respectively.
			button_draw_score = 40.0,
			button_use_score = 25.0,
			button_default_score = 5.0,
			button_other_score = 0.0,
			-- Destroy/discard score is base +/- target value * weight. Friendly
			-- targets subtract the weighted value; opposing targets add it.
			destroy_base = 30.0,
			destroy_value_weight = 5.0,
			-- Target-value formula for prompts mentioning "graveyard".
			graveyard_base = 20.0,
			graveyard_friendly_weight = 4.0,
			graveyard_opponent_weight = -3.0,
			-- Target-value formula for prompts mentioning "return".
			return_base = 20.0,
			return_friendly_weight = -2.0,
			return_opponent_weight = 4.0,
			-- Fallback target formula for an opponent's card or a friendly card
			-- in the graveyard, where selecting a more valuable card is preferred.
			opponent_base = 15.0,
			opponent_value_weight = 3.0,
			-- Fallback target formula for other friendly cards.
			friendly_base = 10.0,
			friendly_value_weight = 1.0,
		},

		mana_payment = {
			-- Reward for leaving at least one untapped card of a civilization.
			civilization_coverage_score = 30.0,
			-- Additional reward for redundant untapped sources, up to this count.
			civilization_count_cap = 3,
			civilization_count_weight = 3.0,
			-- Reward for each remaining hand card whose civilizations stay covered.
			castable_card_base = 2.0,
			-- Cost portion of that reward, capped to stop huge costs dominating.
			castable_cost_cap = 8,
			castable_cost_weight = 0.25,
		},

		attack = {
			-- Dominating rollout score for attacking a player with no shields.
			lethal_score = 100000.0,
			-- Direct-player attack score before breaker and blocker adjustments.
			player_base = 35.0,
			breaker_weight = 8.0,
			-- Penalty applied once for each opposing blocker on the field.
			blocker_penalty = 4.0,
			-- Score for safely attacking a weaker creature.
			winning_creature_base = 65.0,
			winning_target_weight = 5.0,
			-- Equal-power trade score uses (defender value - attacker value).
			trade_base = 35.0,
			trade_value_weight = 5.0,
		},

		block = {
			-- Score returned when blocker or attacker state is invalid.
			invalid_score = -1000.0,
			-- Blocking receives the urgent base at or below this shield count.
			urgent_shield_count = 1,
			urgent_base = 80.0,
			normal_base = 20.0,
			-- Dominating base for a block where the blocker survives.
			surviving_base = 1000.0,
			-- Added as numerator/(blocker power+1), preferring the weakest blocker
			-- that still wins and conserving stronger blockers for later.
			weakest_winner_numerator = 1000000.0,
			-- Bonus for an equal-power trade; losing blocks instead lose card value.
			trade_bonus = 25.0,
			losing_card_penalty = 5.0,
		},

		move = {
			-- Base and visible-card-value multiplier for summoning/casting.
			cardplay_base = 42.0,
			cardplay_value_weight = 4.0,
			-- Added once per evolution bait supplied by the play action.
			evolution_bait_bonus = 8.0,
			-- Generic rollout scores for engine actions without richer formulas.
			mana_tap_score = 20.0,
			block_skip_score = 1.0,
			shield_target_score = 20.0,
			-- Trigger-use score additionally includes the trigger card's value.
			trigger_base = 70.0,
			trigger_skip_score = 0.0,
			tap_ability_score = 35.0,
			-- Negative value makes productive actions preferable to ending early.
			end_turn_score = -100.0,
			-- Score for any move type without a dedicated heuristic.
			default_score = 0.0,
		},

		personality = {
			-- Additive adjustments applied after the base move score. Positive
			-- values encourage an action type; negative values discourage it.
			-- Aggressive favors attacking/playing cards and is more willing to
			-- decline blocks during rollout decisions.
			aggressive_attack = 16.0,
			aggressive_cardplay = 6.0,
			aggressive_blockskip = 3.0,
			-- Defensive favors blocking/triggers and suppresses attacks.
			defensive_block_or_trigger = 12.0,
			defensive_attack = -5.0,
			-- Control favors targeted choices, tap abilities, and triggers.
			control_choice_or_tap = 5.0,
			control_trigger = 7.0,
			-- Tempo favors immediate board actions and is less eager to charge mana.
			tempo_cardplay_or_attack = 7.0,
			tempo_mana = -2.0,
			-- Ramp favors charging mana, with a smaller preference for card plays.
			ramp_mana = 10.0,
			ramp_cardplay = 2.0,
			-- Sacrifice favors choices and plays that may enable sacrifice lines.
			sacrifice_choice_or_cardplay = 4.0,
		},
	},
}

return AIParams
