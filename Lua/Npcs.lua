-- Overworld NPC metadata
-- NPC positions are owned by World/World.json and maintained by the World Builder.
--
-- Gold rewards use tiers rather than authored currency amounts. This global is
-- the sole payout table consumed by the NPC loader.
NpcGoldTiers = {
	T1 = 200,
	T2 = 400,
	T3 = 800,
	T4 = 1500,
	T5 = 3000,
}
--
-- Add one table to the returned array for each NPC. Required fields are:
--   id, name, kind, appearance. Crest Holders also set crest to one of:
--   dawn, tidal, forge, verdant, confluence, tempest, ashen, mirror, unity.
-- Town NPC capabilities are declared in options: Talk is always available,
-- while duel, trade, and wandering are independently configurable. Town NPCs
-- with duel enabled, route duelists, and bosses require max_battles plus
-- non-empty decks and rewards arrays. If max_battles is larger than either
-- array, that array's final entry is reused for the remaining battles.
-- Route duelists also require sight = { range = 1..12 }. This is a taxicab
-- radius and is not limited by the trainer's facing direction. They wander
-- automatically within the 3x3 area centered on their native world position.
--
-- Supported kinds: town_npc, route_duelist, boss
-- Appearances reference a file in Resources/Graphics/Characters followed by
-- the one-based character number in that sheet. For example, "Actor2-3" uses
-- the third character in Actor2.png. Normal sheets contain characters 1..8;
-- files whose names contain '$' are single-character sheets and use number 1.
--
-- Dialogue is a flat, extensible string table. The current application uses:
--   greeting, talk, defeat, victory, complete, clue, investigation,
--   stabilize_before, stabilize_after, boss_reveal, act_complete,
--   shop_early, shop_late
--
-- AI profiles are defined in Lua/AIParams.lua. Current personalities are
-- rush, tempo, and control; current difficulties are easy, medium, and hard.
--
-- Template:
-- {
--     id = "unique_id",
--     name = "Display Name",
--     kind = "town_npc",
--     options = { duel = true, trade = false, wander = true },
--     -- Route duelists use kind = "route_duelist" and:
--     -- sight = { range = 6 },
--     appearance = "Actor2-3",
--     crest = "dawn", -- optional; awarded by the first victory
--     max_battles = 4,
--     decks = { "Example.txt", "ExampleAdvanced.txt" },
--     rewards = {
--         { card = "First Reward Card", gold_tier = 1 },
--         { card = "Later Reward Card", gold_tier = 2 },
--     },
--     ai = { personality = "default", difficulty = "medium" },
-- personality may be default, rush, tempo, or control. "default" uses the
-- unmodified base evaluation, search, and heuristic parameters.
--     dialogue = {
--         greeting = "Ready to duel?",
--         defeat = "You won this time.",
--         victory = "Try again when you are ready.",
--         complete = "We have nothing left to prove."
--     }
-- },

return {
	--Emberglen
    {
        id = "mira",
        name = "Mira",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor1-6",
        max_battles = 4,
        decks = { "Zagaan.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
        ai = { personality = "control", difficulty = "easy" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
            complete = "Four stable echoes. Zagaan remembers you now; I have nothing more to wager.",
            clue = "That broken-circle symbol belongs to the Curator, a duelist who steals the memories bound to cards.",
            investigation = "The silence in these cards is deliberate. Someone removed more than ink.",
            stabilize_before = "A true duel may stabilize Zagaan's fading echo. Show me what your deck remembers.",
            stabilize_after = "That echo is stable. Find other duelists before the Curator follows its signal.",
            boss_reveal = "The mask at the bridge has no living echo. Be careful.",
            act_complete = "The old road is full of memories the Curator failed to erase. Follow them."
        }
    },
    {
        id = "marin",
        name = "Marin",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor1-3",
        max_battles = 4,
        decks = { "NPC/Marin.txt", "NPC/Marin 2.txt", "NPC/Marin 3.txt", "NPC/Marin 4.txt" },
        rewards = {
            { card = "Aqua Sniper", gold_tier = 1 },
        },
        ai = { personality = "tempo", difficulty = "easy" },
        dialogue = {
            greeting = "Let us see whether you can read the currents.",
            defeat = "You changed course after I committed. Good. I will account for that next time.",
            victory = "Information is useful only before the decision. Review where your plan became fixed.",
            complete = "Four victories are enough evidence. Aqua Sniper's echo is secure with you.",
            investigation = "Every current through town changed at the same instant. That requires a single source.",
            stabilize_before = "My signature card is losing detail. A decisive duel may give the echo a path back.",
            stabilize_after = "The pattern is stable. We need two more signals before I can triangulate the source.",
            boss_reveal = "The bridge distortion is following our restored signals. The masked duelist planned for this.",
            act_complete = "I cannot predict the old road, which is exactly why you should take it."
        }
    },
    {
        id = "rook",
        name = "Rook",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor5-3",
        max_battles = 1,
        decks = { "RoaringGreathorn.txt"},
        rewards = {
            { card = "Roaring Great-Horn", gold_tier = 1 },
        },
        ai = { personality = "tempo", difficulty = "medium" },
        dialogue = {
            greeting = "Strength grows one turn at a time.",
            defeat = "You reached your strength before I reached mine. A solid victory.",
            victory = "Your foundation was too narrow. Build the mana before you build the tower.",
            complete = "Great-Horn answers you without hesitation. That is strength worth recognizing.",
            investigation = "I can repair roads and bridges, but I cannot hammer a name back onto a card.",
            stabilize_before = "Give my creatures a battle they can recognize. Their echoes may follow the rhythm home.",
            stabilize_after = "This bond will hold. Help the others establish theirs.",
            boss_reveal = "We will hold the town. You take the bridge.",
            act_complete = "The northern bridge is sound. Whatever waits beyond it is your next foundation."
        }
    },
    {
        id = "aurelia",
        name = "Aurelia",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor4-3",
        crest = "dawn",
        max_battles = 4,
        decks = { "NPC/Aurelia.txt", "NPC/Aurelia 2.txt", "NPC/Aurelia 3.txt", "NPC/Aurelia 4.txt" },
        rewards = {
            { card = "Hanusa, Radiance Elemental", gold_tier = 1 },
        },
        ai = { personality = "control", difficulty = "medium" },
        dialogue = {
            greeting = "The light judges every reckless move. Shall we begin?",
            defeat = "Your attack was measured, not reckless. I concede.",
            victory = "Patience is not passivity. Wait only while waiting improves your position.",
            complete = "Hanusa recognizes your discipline. I can offer no higher local trial.",
            clue = "When the festival lights failed, every blank card pointed north. Rowan ran toward the bridge before he vanished.",
            investigation = "The festival wards did not break. They were instructed to admit something they could not identify.",
            stabilize_before = "Let us conduct the exhibition duel we were denied. A formal match may restore the echo.",
            stabilize_after = "The resonance is clean. Earn the trust of two more duelists.",
            boss_reveal = "The masked stranger has appeared at the central bridge. We will protect Emberglen.",
            act_complete = "Follow the road when you are ready. Return for the official Dawn Crest match."
        }
    },
    {
        id = "flint",
        name = "Flint",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor5-4",
        max_battles = 4,
        decks = { "NPC/Flint.txt", "NPC/Flint 2.txt", "NPC/Flint 3.txt", "NPC/Flint 4.txt" },
        rewards = {
            { card = "Rothus, the Traveler", gold_tier = 1 },
			{ card = "Rothus, the Traveler", gold_tier = 1 },
			{ card = "Rothus, the Traveler", gold_tier = 1 },
			{ card = "Uberdragon Bajula", gold_tier = 1 },
        },
        ai = { personality = "rush", difficulty = "medium" },
        dialogue = {
            greeting = "My dragons have been waiting for a worthy opponent.",
            defeat = "Ha! You survived the heat and struck back harder. Again soon.",
            victory = "You cannot save every shield. Make me regret the ones you let me break.",
            complete = "Four wins? Fine, fine. Astrocomet Dragon clearly likes you.",
            clue = "I saw a masked duelist beside the arena. Their cards had no civilization mark—and no names.",
            investigation = "The arena junction was altered by someone who knew exactly how our equipment worked.",
            stabilize_before = "Nothing restores a Fire echo like a real fight. Try to keep up.",
            stabilize_after = "That did it! The dragon's name is burning bright again. Find two more.",
            boss_reveal = "Go get that mask! We will make sure Emberglen is still here when you return.",
            act_complete = "Next time I am coming with you. Someone has to keep this pursuit exciting."
        }
    },
    {
        id = "nyx",
        name = "Nyx",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor3-3",
        max_battles = 4,
        decks = { "NPC/Nyx.txt", "NPC/Nyx 2.txt", "NPC/Nyx 3.txt", "NPC/Nyx 4.txt" },
        rewards = {
            { card = "Horrid Worm", gold_tier = 1 },
			{ card = "Trox, General of Destruction", gold_tier = 1 },
			{ card = "Ballom, Master of Death", gold_tier = 1 },
			{ card = "Phantomach, the Gigatrooper", gold_tier = 1 },
        },
        ai = { personality = "control", difficulty = "medium" },
        dialogue = {
            greeting = "The abyss remembers every card you lose.",
            defeat = "A worthwhile loss. The graveyard gained a story instead of a silence.",
            victory = "You feared losing your creatures more than you desired the victory.",
            complete = "Deathliger's echo follows you willingly. Do not make it regret that trust.",
            investigation = "The graveyard went silent during the blackout. Even destroyed cards had their histories taken.",
            stabilize_before = "Let us give the fading echo a loss vivid enough to remember.",
            stabilize_after = "It speaks again. Other silent cards are waiting for the same kindness.",
            boss_reveal = "That thing wears absence like a body. Break the spell, not merely the mask.",
            act_complete = "The old road is haunted by living memories. I envy you."
        }
    },
    {
        id = "tidal",
        name = "Garran",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor1-7",
        max_battles = 1,
        decks = { "KingDepthcon.txt" },
        rewards = {
            { card = "King Depthcon", gold_tier = 1 },
        },
        ai = { personality = "control", difficulty = "easy" },
        dialogue = {
            greeting = "The deep favors patience. Can you keep your footing?",
            defeat = "You did not rush when the opening appeared. That restraint won the duel.",
            victory = "The current carried you exactly where I expected. Learn when to step out of it.",
            complete = "King Depthcon knows your pace now. Our remaining duels would prove nothing new.",
            investigation = "Nobody leaves by the southern road until we know whether the fading can spread through a caravan.",
            stabilize_before = "A long duel leaves a clear wake. Let us see whether the echo can follow it.",
            stabilize_after = "The wake is holding. Find other signature cards and compare their course.",
            boss_reveal = "I have the southern road. Do what must be done at the bridge.",
            act_complete = "The northern waters are rougher than they look. Patience will carry you farther than haste."
        }
    },
    {
        id = "briar",
        name = "Briar",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor2-3",
        max_battles = 1,
        decks = { "DeathbladeBeetle.txt" },
        rewards = {
            { card = "Deathblade Beetle", gold_tier = 1 },
        },
        ai = { personality = "tempo", difficulty = "easy" },
        dialogue = {
            greeting = "Nature rewards the duelist who grows strongest.",
            defeat = "Your deck adapted faster than mine. The forest approves, even if I do not.",
            victory = "You planted good ideas but never gave them enough mana to grow.",
            complete = "Deathblade Beetle recognizes you as part of its path. Treat it like an ally.",
            investigation = "The creatures are wandering as though their bonded duelists have become strangers.",
            stabilize_before = "Help my creatures remember what cooperation feels like. Duel me.",
            stabilize_after = "They know one another again. Other bonds in town still need help.",
            boss_reveal = "The western paths are guarded. Bring Rowan home.",
            act_complete = "The forest has already opened a path north. It seems to expect you."
        }
    },
    {
        id = "mercer",
        name = "Mercer",
        kind = "town_npc",
        options = { trade = true, wander = false },
        appearance = "People1-5",
        ai = { personality = "tempo", difficulty = "medium" },
        dialogue = {
            greeting = "Welcome! I trade hard-earned gold for cards.",
            shop_early = "These blank card fragments started appearing after the festival. Bring me gold and I'll keep you supplied.",
            shop_late = "Your restored echoes are holding their ink. The Curator will have noticed.",
            act_complete = "The old road leads onward, and so does my traveling shop. Convenient, isn't it?"
        }
    },
    {
        id = "veiled_one",
        name = "The Veiled One",
        kind = "boss",
        appearance = "Evil-6",
        max_battles = 1,
        decks = { "NPC/VeiledOne.txt" },
        rewards = {
            { card = "Urth, Purifying Elemental", gold_tier = 2 },
        },
        ai = { personality = "tempo", difficulty = "medium" },
        dialogue = {
            greeting = "Every echo you restored belongs to the Curator. Hand them over.",
            defeat = "The veil is only a shell. The Curator has already measured what you restored.",
            victory = "Your echoes will be quieter in the Hollow Deck.",
            complete = "The Curator has already crossed the old road. Rowan still lives—but not for long."
        }
    },

	--Cinderrail
	{
        id = "pip",
        name = "Pip",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People1-3",
        max_battles = 1,
        decks = { "DeathbladeBeetle.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Foundry beetles can chew through slag or shields. Want to see which goes first?",
			defeat = "You cracked my shell before I could harden the board. Nicely timed.",
			victory = "You kept striking the armor instead of the joints.",
			complete = "That was enough testing for one shift. The beetles need cooling down.",
			talk = "I collect the little metal-eaters nesting beneath the ore carts. They are gentler than they look. Usually."
		}
    },
	{
        id = "noma",
        name = "Noma",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-2",
        max_battles = 1,
        decks = { "AstrocometDragon.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "A comet over Cinderrail means sparks, noise, and a very short duel. Interested?",
			defeat = "You stepped through the blast instead of away from it. Bold move.",
			victory = "That hesitation gave my dragons all the sky they needed.",
			complete = "The comet has passed. Catch me again after the next furnace flare.",
			talk = "Night crews swear the smoke hides stars. I say the stars are hiding from our furnaces."
		}
    },
	{
        id = "bram",
        name = "Bram",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-3",
        max_battles = 1,
        decks = { "Fire Generic 1.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "No gauges, no safety rails—just Fire creatures and nerve. Duel me.",
			defeat = "You kept your cool better than half the furnace crew.",
			victory = "Around here, anything left unattended eventually catches fire.",
			complete = "Inspection finished. Your deck is cleared for the hot floor.",
			talk = "I tune burner valves by ear. A bad hiss ruins steel; a good roar makes it sing."
		}
    },
	{
        id = "elia",
        name = "Elia",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-4",
        max_battles = 1,
        decks = { "Urth.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Even this soot cannot dim Urth's light. Let me test the clarity of your deck.",
			defeat = "Your choices stayed clean when the field became clouded. Well done.",
			victory = "Power without order scatters like sparks in a draft.",
			complete = "The trial is concluded. Carry that discipline beyond the foundry.",
			talk = "I maintain the lamps along the night gantries. Light is a safety rule here, not decoration."
		}
    },
	{
        id = "tomas",
        name = "Tomas",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People2-1",
        max_battles = 1,
        decks = { "Hanusa.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Hanusa guards my shields like marble walls. Show me how you breach them.",
			defeat = "You found the one seam I failed to reinforce.",
			victory = "A fortress wins by making every attack cost more than it gains.",
			complete = "One siege is sufficient. I have repairs to calculate.",
			talk = "The foundry arches expand in the heat. I measure them before each shift so the roof stays where it belongs."
		}
    },
	{
        id = "senn",
        name = "Senn",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People2-3",
        max_battles = 1,
        decks = { "TrenchdiveShark.txt" },
        rewards = {
            { card = "Trenchdive Shark", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "The cooling trenches run deep. So does my deck. Care to dive in?",
			defeat = "You surfaced before the pressure could crush your plan.",
			victory = "You followed the ripples and missed what moved beneath them.",
			complete = "The trench is calm again. I will save the next dive for another traveler.",
			talk = "Water from the cooling channels leaves bright mineral scales on everything. The sharks seem to like them."
		}
    },
	{
        id = "kipp",
        name = "Kipp",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People7-6",
        max_battles = 1,
        decks = { "Fire Generic 2.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "I race sparks from the rail hammers. Your deck cannot be slower than a spark, can it?",
			defeat = "Fast and accurate? That hardly seems fair.",
			victory = "Too late! The best opening is the one that is already on fire.",
			complete = "Race over. I have a new hammer record to chase.",
			talk = "The foreman says sparks do not count as competitors. The foreman is wrong."
		}
    },
	{
        id = "ansa",
        name = "Ansa",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People3-1",
        max_battles = 1,
        decks = { "Zagaan.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "The furnace casts long shadows. Zagaan prefers to duel inside them.",
			defeat = "You carried your own light into the dark. I underestimated that.",
			victory = "Every bright plan leaves a shadow large enough to hide a counterattack.",
			complete = "Zagaan has learned your silhouette. Another duel would not surprise it.",
			talk = "I inspect the ash tunnels after shutdown. Things grow down there that never see the sun."
		}
    },
	{
        id = "holt",
        name = "Holt",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-5",
        max_battles = 1,
        decks = { "RoaringGreathorn.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Great-Horn can haul an ore wagon uphill. Can your deck stop that much momentum?",
			defeat = "You redirected the charge instead of meeting it head-on. Smart.",
			victory = "Once the heavy line starts moving, standing firm is not a strategy.",
			complete = "The wagon is parked and the duel is done. Good work.",
			talk = "I handle the ore teams. Machines are louder, but creatures know when a bridge is unsafe."
		}
	},

	--Glasswater
	{
		id = "neris",
		name = "Neris Quill",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor2-5",
		crest = "tidal",
		max_battles = 3,
		decks = { "NPC/Neris Quill.txt", "NPC/Neris Quill 2.txt", "NPC/Neris Quill 3.txt" },
		rewards = {
			{ card = "Crystal Paladin", gold_tier = 1 },
			{ card = "Crystal Paladin", gold_tier = 1 },
			{ card = "Hydrooze, the Mutant Emperor", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Glasswater rewards the plan that changes when the current does. Show me yours.",
			defeat = "You used the information without becoming trapped by it. A precise victory.",
			victory = "You saw the route I offered and never asked who chose it for you.",
			complete = "Four trials are enough. Keep questioning every perfect prediction.",
			investigation = "The engine knows private decisions no manifest could contain. That knowledge was taken, not inferred."
		}
	},
	{
		id = "pell",
		name = "Pell",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-5",
		max_battles = 1,
		decks = { "KingTsunami.txt" },
		rewards = {
			{ card = "Emeral", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Cargo shifts. Tides shift. A good deck shifts before either one.",
			defeat = "You changed balance before the load moved. Clean work.",
			victory = "The quay teaches quickly: brace before the cargo starts sliding.",
			complete = "You know this dock's rhythm now. I have no new surprise to unload."
		}
	},
	{
		id = "iri",
		name = "Iri",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People4-4",
		max_battles = 1,
		decks = { "More2/WD Dark Tide v3.txt" },
		rewards = {
			{ card = "Corile", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Cards in hand are assets. I intend to audit yours.",
			defeat = "Your position reconciles. Mine very much does not.",
			victory = "Unrecorded risks still appear in the final balance.",
			complete = "Your account is settled, with four victories in your favor."
		}
	},
	{
		id = "sol",
		name = "Sol",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People5-2",
		max_battles = 1,
		decks = { "KingDepthcon.txt" },
		rewards = {
			{ card = "Crystal Memory", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "I have mapped every route through the port except the one your deck will take.",
			defeat = "A useful correction. No chart survives without revisions.",
			victory = "Your route ended exactly where the current said it would.",
			complete = "Four revisions agree. I will mark your route as reliably unpredictable."
		}
	},

	--Rootmaze
	{
		id = "oren",
		name = "Oren Canopy",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor1-3",
		crest = "verdant",
		max_battles = 1,
		decks = { "NPC/Oren.txt" },
		rewards = {
			{ card = "Cryptic Totem", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Growth gives us another path, not an order to take it. Which path will your deck choose?",
			defeat = "You let the board tell you what to grow. Rootmaze recognizes that patience.",
			victory = "The largest branch is not always the one holding the nest.",
			complete = "The paths know you now. Four more duels would only lead us in circles.",
			investigation = "These creatures remember bonds their cards have forgotten. We must reunite them before the false roots harden."
		}
	},
	{
		id = "fern",
		name = "Fern",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People4-2",
		max_battles = 1,
		decks = { "Barkwhip.txt" },
		rewards = {
			{ card = "Barkwhip, the Smasher", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "I was looking for moonberries, but a good duel is nearly as rare.",
			defeat = "You found the opening before I found the next trail.",
			victory = "A little preparation keeps even the tallest growth from toppling.",
			complete = "Four excellent specimens. I should probably return to the berries now."
		}
	},
	{
		id = "toma",
		name = "Toma",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-6",
		max_battles = 1,
		decks = { "AnristVhal.txt" },
		rewards = {
			{ card = "Essence Elf", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Do not worry. The beetles only become agitated when they sense hesitation.",
			defeat = "They liked that duel. Even the ones in the graveyard are buzzing.",
			victory = "Every loss fed the colony. You should have ended the cycle sooner.",
			complete = "The beetles insist four trials are conclusive. I have learned not to argue."
		}
	},
	{
		id = "moss",
		name = "Moss",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-5",
		max_battles = 1,
		decks = { "StormWrangler.txt" },
		rewards = {
			{ card = "Dimension Gate", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Wrong turn. The way back is free; the way forward costs one duel.",
			defeat = "That route works. I will have to repaint the sign.",
			victory = "The marked path was safer, but I respect the experiment.",
			complete = "No more tolls. You have paid for every path Rootmaze can offer."
		}
	},

	---Watershed Crossroads
	{
		id = "crossroad-duelist-1",
		name = "Ford",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-1",
		max_battles = 1,
		decks = { "Nature Generic 1.txt" },
		rewards = {
			{ card = "Bronze-Arm Tribe", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "The western ford changes every week. A duel is the safest way to learn whether a traveler adapts.",
			defeat = "You read the current before committing. The long path north may reward that patience.",
			victory = "The shortest crossing is rarely the safest one.",
			complete = "You know all four moods of this ford now. The water has nothing left to teach by duel."
		}
	},
	{
		id = "crossroad-duelist-2",
		name = "Tony",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-3",
		max_battles = 1,
		decks = { "Water Generic 1.txt" },
		rewards = {
			{ card = "Teleportation", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "I came to chart the southern pools. They keep turning into paths when I am not looking.",
			defeat = "Perhaps a route does not need to stay still to be understood.",
			victory = "You chased the marker. I watched the marsh.",
			complete = "Four surveys are enough. Take the shell; it remembers this wetland better than my maps do."
		}
	},
	{
		id = "crossroad-duelist-3",
		name = "Cairn",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor4-1",
		max_battles = 1,
		decks = { "AstrocometDragon.txt" },
		rewards = {
			{ card = "Astrocomet Dragon", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "A true duelist turns a crossroads into a launchpad. Come on—let's fire up!",
			defeat = "Now that was a finishing move! You earned the right of way.",
			victory = "Your spirit sputtered just before your deck caught flame.",
			complete = "Our match settled it. The next road is yours to choose."
		}
	},
	{
		id = "crossroad-duelist-4",
		name = "Shobu",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People3-2",
		max_battles = 1,
		decks = { "Official/2 - F Shobu Fire.txt" },
		rewards = {
			{ card = "Rothus, the Traveler", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "This crossroads leads everywhere a duelist could dream of going. First, let's fire up!",
			defeat = "Awesome duel! You found a winning path I never saw coming.",
			victory = "A duelist chooses a direction and charges with everything they have!",
			complete = "We settled this match, but there is always another road and another rival."
		}
	},
	{
		id = "crossroad-duelist-5",
		name = "Anixa",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor2-8",
		max_battles = 1,
		decks = { "Dark Generic 2.txt" },
		rewards = {
			{ card = "Amber Piercer", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "Travelers drop useful things when startled. Let me startle your hand empty.",
			defeat = "Hmph. You held on to exactly the cards I needed gone.",
			victory = "The marsh keeps what sinks, and so does my graveyard.",
			complete = "I have taken all the measure of you that I need. Keep moving."
		}
	},
	{
		id = "crossroad-duelist-6",
		name = "Caroline",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor1-2",
		max_battles = 1,
		decks = { "Light Generic 1.txt" },
		rewards = {
			{ card = "Dia Nork, Moonlight Guardian", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "These roads need a guardian. Before I let you pass, show me whom your shields protect.",
			defeat = "You broke my formation without abandoning your own. Passage granted.",
			victory = "A rushed attack only proves why defenses are necessary.",
			complete = "I have judged your formation once. My verdict stands."
		}
	},
	{
		id = "crossroad-duelist-7",
		name = "Musaffir",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor1-5",
		max_battles = 1,
		decks = { "Nature Generic 1.txt" },
		rewards = {
			{ card = "Fear Fang", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "I have crossed deserts for rare creatures. A rare duel is harder to find—will you provide one?",
			defeat = "Yes, that will be worth telling at the next caravan fire.",
			victory = "A traveler survives by carrying answers for roads not yet seen.",
			complete = "One memorable duel weighs less than ten dull ones. Safe travels."
		}
	},
	{
		id = "crossroad-duelist-8",
		name = "Onix",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor1-7",
		max_battles = 1,
		decks = { "Nature Generic 2.txt" },
		rewards = {
			{ card = "Rumbling Terahorn", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "Hear that rumble? The earth already knows how this duel ends.",
			defeat = "You moved like water around stone. Even Terahorn could not pin you down.",
			victory = "The ground warned you before every charge. You should have listened.",
			complete = "The stones have delivered their verdict. I will not ask twice."
		}
	},
	{
		id = "crossroad-duelist-9",
		name = "Kokujo",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor1-8",
		max_battles = 1,
		decks = { "Official/2 - D Kokujo Darkness.txt" },
		rewards = {
			{ card = "Death Smoke", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "The crossroads is full of weak duelists choosing safe paths. Disappoint me and join them.",
			defeat = "Do not celebrate. Darkness sharpens humiliation into vengeance.",
			victory = "Your last useful card vanished before you understood the duel had begun.",
			complete = "I have nothing to gain from crushing the same strategy again."
		}
	},
	{
		id = "crossroad-duelist-10",
		name = "Asim",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People4-7",
		max_battles = 1,
		decks = { "Fire Generic 2.txt" },
		rewards = {
			{ card = "Armored Cannon Balbaro", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "My cannon can clear a blocked road and a shield line with the same shot. Stand clear—or duel.",
			defeat = "Direct hit on my blind side. I cannot argue with that aim.",
			victory = "When Balbaro fires, the only safe route is already behind you.",
			complete = "The barrel needs cooling. Consider yourself dismissed."
		}
	},

	--old-road
	{
		id = "old-road-duelist-1",
		name = "Mara Flintway",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-6",
		max_battles = 1,
		decks = { "RoaringGreathorn.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Great-Horn and I patrol the eastern loop. Prove you will not become our next rescue.",
			defeat = "Steady footing, clear choices—you are ready for the broken trail.",
			victory = "The road punishes travelers who build on a weak foundation.",
			complete = "Inspection over. Wayfarer Camp will hear that you passed."
		}
	},
	{
		id = "old-road-duelist-2",
		name = "Shi Li",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-4",
		max_battles = 1,
		decks = { "Dark Generic 1.txt" },
		rewards = {
			{ card = "Ghost Touch", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "The old stones remember every traveler. Let us see what your discarded cards remember.",
			defeat = "You left no regret behind for my darkness to use.",
			victory = "A secret carried too long becomes weight. I simply made you drop it.",
			complete = "Our shadows have crossed once. They need not cross again."
		}
	},
	{
		id = "old-road-duelist-3",
		name = "Amber",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-8",
		max_battles = 1,
		decks = { "TrenchdiveShark.txt" },
		rewards = {
			{ card = "Cetibols", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Rain filled the wagon ruts overnight. Perfect conditions for a trench duel.",
			defeat = "You kept your plan afloat through every undertow.",
			victory = "Never step into dark water before learning what circles beneath it.",
			complete = "The road is drying and our match is settled. I should move on."
		}
	},
	{
		id = "old-road-duelist-4",
		name = "Pol",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor4-1",
		max_battles = 1,
		decks = { "Fire Generic 1.txt" },
		rewards = {
			{ card = "Crimson Hammer", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "I light the road beacons with dragon flame. Your duel can be tonight's kindling.",
			defeat = "You smothered every spark before it reached the dry grass.",
			victory = "On the Old Road, a small flame becomes an emergency very quickly.",
			complete = "The beacon is lit and the challenge is answered. Travel while the light holds."
		}
	},
	{
		id = "old-road-duelist-5",
		name = "Ponna",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-2",
		max_battles = 1,
		decks = { "Fire Generic 2.txt" },
		rewards = {
			{ card = "Volcanic Arrows", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "The warning signs say 'no open flame.' Fortunately, my deck cannot read.",
			defeat = "You put out the blaze and barely singed your sleeves. Impressive.",
			victory = "Volcanic arrows make excellent shortcuts through stubborn defenses.",
			complete = "I promised the wardens only one duel here. Let us leave before they smell smoke."
		}
	},
	{
		id = "old-road-duelist-6",
		name = "Clara",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor4-4",
		max_battles = 1,
		decks = { "AquaSniper.txt" },
		rewards = {
			{ card = "Aqua Sniper", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Bandits hate Aqua Sniper. It returns their ambushes to sender. Want a demonstration?",
			defeat = "You gave me no safe target to send away. Excellent formation.",
			victory = "The road is easier when troublesome creatures take the long way around.",
			complete = "Demonstration complete. If you meet bandits, remember the timing."
		}
	},
	{
		id = "old-road-duelist-7",
		name = "Olmec",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People3-2",
		max_battles = 1,
		decks = { "Zagaan.txt" },
		rewards = {
			{ card = "Vampire Silphy", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "These mile markers were grave markers first. Zagaan would like to introduce itself.",
			defeat = "You showed proper respect to the dead—and none at all to my shields.",
			victory = "Ancient roads belong to ancient powers after sunset.",
			complete = "The stones have heard your name. I will not trouble them with a rematch."
		}
	},
	{
		id = "old-road-duelist-8",
		name = "Totiana",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People5-4",
		max_battles = 1,
		decks = { "Dark Generic 2.txt" },
		rewards = {
			{ card = "Gigazoul", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "I trade in rumors, but a duel tells me more than travelers ever do.",
			defeat = "Interesting. Your deck kept its ugliest answer hidden until the perfect moment.",
			victory = "I knew your plan before you finished pretending it was a secret.",
			complete = "I have the information I wanted. The rest of your journey is your business."
		}
	},
	{
		id = "old-road-duelist-9",
		name = "Flora",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People4-4",
		max_battles = 1,
		decks = { "Nature Generic 2.txt" },
		rewards = {
			{ card = "Enchanted Soil", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "The roadside soil is exhausted. Let us see whether your mana grows any better.",
			defeat = "You cultivated exactly the board you needed—nothing wasted.",
			victory = "Strong roots are planned long before the first creature appears.",
			complete = "One harvest is enough to judge the field. Yours is promising."
		}
	},
	{
		id = "old-road-duelist-10",
		name = "Berry",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People5-1",
		max_battles = 1,
		decks = { "TreeFolkNoEvo.txt" },
		rewards = {
			{ card = "Psyshroom", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Quiet—my Psyshrooms are predicting the duel. They say you should accept.",
			defeat = "They predicted that too. Admittedly, only after the final attack.",
			victory = "Never argue strategy with a mushroom that can see three turns ahead.",
			complete = "The spores have settled on one outcome. A second duel would only confuse them."
		}
	},

	--treacherous-pass
	{
		id = "treacherous-duelist-1",
		name = "Bandit Benzo",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Evil-1",
		max_battles = 1,
		decks = { "Deathliger.txt" },
		rewards = {
			{ card = "Terror Pit", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Toll road! Pay in gold, cards, or one humiliating defeat. I recommend the gold.",
			defeat = "Keep your coins. I suddenly have urgent business somewhere else.",
			victory = "Terror Pit collects faster than any tollkeeper I know.",
			complete = "You already beat my best trap. There is no profit in springing it twice."
		}
	},
	{
		id = "treacherous-duelist-2",
		name = "Bandit Pollo",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Evil-3",
		max_battles = 1,
		decks = { "BraidClaw.txt" },
		rewards = {
			{ card = "Deadly Fighter Braid Claw", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "Pollo strikes first, Braid Claw strikes second, and questions never get a turn!",
			defeat = "You were supposed to fall over before I ran out of creatures!",
			victory = "Fast hands fill slow travelers' graves—and my pockets.",
			complete = "No rematch. Surprise only works before someone knows the trick."
		}
	},
	{
		id = "treacherous-duelist-3",
		name = "Bandit Gras",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Evil-1",
		max_battles = 1,
		decks = { "FireDark Generic.txt" },
		rewards = {
			{ card = "Blasto, Explosive Soldier", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "Fire above, darkness below, and nowhere left for you to run. Perfect ambush, eh?",
			defeat = "You punched straight through the middle! Who plans for that?",
			victory = "Blasto clears the road; I collect what the explosion leaves behind.",
			complete = "The ambush site is spoiled now. Go on before I find another."
		}
	},
	{
		id = "treacherous-duelist-4",
		name = "Bandit Tillo",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Evil-3",
		max_battles = 1,
		decks = { "HandRemoval.txt" },
		rewards = {
			{ card = "Tyrant Worm", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "easy" },
		dialogue = {
			greeting = "Before I rob your pack, I will empty your hand. Professional pride demands an order.",
			defeat = "You kept producing answers after I took them all. Infuriating.",
			victory = "A traveler with no cards is simply luggage that walks itself.",
			complete = "I know what you hide now. Trying the same theft twice would be amateur work."
		}
	},
	{
		id = "treacherous-duelist-boss",
		name = "Banditlord Brocco",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Evil-3",
		max_battles = 1,
		decks = { "More2/WD Dark Tide.txt" },
		rewards = {
			{ card = "Corile", gold_tier = 2 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "You embarrassed my crew. Banditlord Brocco will take the reward from your deck personally.",
			defeat = "Impossible... I planned for every route except the one you made.",
			victory = "That is why I am the Banditlord and they are roadside scenery.",
			complete = "My crew has scattered and my reputation is bruised. Take the pass and be gone."
		}
	},
}
