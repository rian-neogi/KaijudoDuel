-- Overworld NPC metadata
-- NPC positions are owned by Lua/World.lua and maintained by the World Builder.
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
-- radius and is not limited by the trainer's facing direction.
--
-- Supported kinds: town_npc, route_duelist, boss
-- Supported appearances: mira, marin, rook, aurelia, flint, nyx, tidal,
--                        briar, mercer, veiled_one, neris, oren,
--                        generic-male-1 through generic-male-10,
--                        generic-female-1 through generic-female-10
--
-- Dialogue is a flat, extensible string table. The current application uses:
--   greeting, talk, defeat, victory, complete, clue, investigation,
--   stabilize_before, stabilize_after, boss_reveal, act_complete,
--   shop_early, shop_late
--
-- AI personalities currently supported by the heuristic bot are:
--   balanced, aggressive, defensive, control, tempo, ramp, sacrifice, adaptive
--
-- Template:
-- {
--     id = "unique_id",
--     name = "Display Name",
--     kind = "town_npc",
--     options = { duel = true, trade = false, wander = true },
--     -- Route duelists use kind = "route_duelist" and:
--     -- sight = { range = 6 },
--     appearance = "mira",
--     crest = "dawn", -- optional; awarded by the first victory
--     max_battles = 4,
--     decks = { "Example.txt", "ExampleAdvanced.txt" },
--     rewards = {
--         { card = "First Reward Card", gold = 100 },
--         { card = "Later Reward Card", gold = 125 },
--     },
--     ai = { personality = "balanced" },
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
        appearance = "mira",
        max_battles = 4,
        decks = { "Zagaan.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
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
        appearance = "marin",
        max_battles = 4,
        decks = { "AquaSniper.txt", "KingDepthcon.txt" },
        rewards = {
            { card = "Aqua Sniper", gold = 100 },
        },
        ai = { personality = "tempo" },
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
        kind = "route_duelist",
        sight = { range = 8 },
        appearance = "rook",
        max_battles = 4,
        decks = { "RoaringGreathorn.txt", "DeathbladeBeetle.txt" },
        rewards = {
            { card = "Roaring Great-Horn", gold = 100 },
        },
        ai = { personality = "ramp" },
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
        appearance = "aurelia",
        crest = "dawn",
        max_battles = 4,
        decks = { "Hanusa.txt", "Urth.txt" },
        rewards = {
            { card = "Hanusa, Radiance Elemental", gold = 100 },
        },
        ai = { personality = "defensive" },
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
            act_complete = "The road is open. Return when you are ready for the official Dawn Crest match."
        }
    },
    {
        id = "flint",
        name = "Flint",
        kind = "town_npc",
        options = { duel = true },
        appearance = "flint",
        max_battles = 4,
        decks = { "AstrocometDragon.txt", "ScarletSkyterror.txt" },
        rewards = {
            { card = "Astrocomet Dragon", gold = 100 },
        },
        ai = { personality = "aggressive" },
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
        appearance = "nyx",
        max_battles = 4,
        decks = { "Deathliger.txt", "Zagaan.txt" },
        rewards = {
            { card = "Deathliger, Lion of Chaos", gold = 100 },
        },
        ai = { personality = "sacrifice" },
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
        name = "Tidal",
        kind = "town_npc",
        options = { duel = true },
        appearance = "tidal",
        max_battles = 4,
        decks = { "KingDepthcon.txt", "AquaSniper.txt" },
        rewards = {
            { card = "King Depthcon", gold = 100 },
        },
        ai = { personality = "control" },
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
        appearance = "briar",
        max_battles = 4,
        decks = { "DeathbladeBeetle.txt", "RoaringGreathorn.txt" },
        rewards = {
            { card = "Deathblade Beetle", gold = 100 },
        },
        ai = { personality = "ramp" },
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
        appearance = "mercer",
        ai = { personality = "none" },
        dialogue = {
            greeting = "Welcome! I trade hard-earned gold for cards.",
            shop_early = "These blank card fragments started appearing after the festival. Bring me gold and I'll keep you supplied.",
            shop_late = "Your restored echoes are holding their ink. The Curator will have noticed.",
            act_complete = "The old road is open, and so is my traveling shop. Convenient, isn't it?"
        }
    },
    {
        id = "veiled_one",
        name = "The Veiled One",
        kind = "boss",
        appearance = "veiled_one",
        max_battles = 1,
        decks = { "NPC/VeiledOne.txt" },
        rewards = {
            { card = "Urth, Purifying Elemental", gold = 250 },
        },
        ai = { personality = "adaptive" },
        dialogue = {
            greeting = "Every echo you restored belongs to the Curator. Hand them over.",
            defeat = "The veil is only a shell. The Curator has already measured what you restored.",
            victory = "Your echoes will be quieter in the Hollow Deck.",
            complete = "The Curator has already crossed the old road. Rowan still lives—but not for long."
        }
    },


	{
        id = "pip",
        name = "Pip",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-male-1",
        max_battles = 1,
        decks = { "DeathbladeBeetle.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "noma",
        name = "Noma",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-female-2",
        max_battles = 1,
        decks = { "AstrocometDragon.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "bram",
        name = "Bram",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-male-3",
        max_battles = 1,
        decks = { "Fire Generic 1.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "elia",
        name = "Elia",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-female-3",
        max_battles = 1,
        decks = { "Urth.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "tomas",
        name = "Tomas",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-male-5",
        max_battles = 1,
        decks = { "Hanusa.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "senn",
        name = "Senn",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-male-4",
        max_battles = 1,
        decks = { "TrenchdiveShark.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "kipp",
        name = "Kipp",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-male-6",
        max_battles = 2,
        decks = { "Fire Generic 2.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "ansa",
        name = "Ansa",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-female-7",
        max_battles = 2,
        decks = { "Zagaan.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
        }
    },
	{
        id = "holt",
        name = "Holt",
        kind = "town_npc",
        options = { duel = true },
        appearance = "generic-male-7",
        max_battles = 2,
        decks = { "RoaringGreathorn.txt", "Deathliger.txt" },
        rewards = {
            { card = "Zagaan, Knight of Darkness", gold = 100 },
        },
        ai = { personality = "control" },
        dialogue = {
            greeting = "Darkness answers my call. Ready to duel?",
            defeat = "The grave remembers that turn. I will not make the same mistake twice.",
            victory = "Listen more closely. Your discarded cards were warning you.",
			talk = ""
		}
	},

	--Glasswater
	{
		id = "neris",
		name = "Neris Quill",
		kind = "town_npc",
		options = { duel = true },
		appearance = "neris",
		crest = "tidal",
		max_battles = 4,
		decks = { "NPC/Neris Quill.txt", "NPC/Neris Quill 2.txt", "NPC/Neris Quill 3.txt" },
		rewards = {
			{ card = "Crystal Paladin", gold = 150 },
			{ card = "Crystal Paladin", gold = 150 },
			{ card = "Hydrooze, the Mutant Emperor", gold = 150 },
		},
		ai = { personality = "control" },
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
		appearance = "generic-male-5",
		max_battles = 1,
		decks = { "KingTsunami.txt" },
		rewards = {
			{ card = "Emeral", gold = 85 },
		},
		ai = { personality = "tempo" },
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
		appearance = "generic-female-4",
		max_battles = 1,
		decks = { "More2/WD Dark Tide v3.txt", "NPC/Neris Quill 2.txt" },
		rewards = {
			{ card = "Corile", gold = 90 },
		},
		ai = { personality = "control" },
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
		appearance = "generic-male-10",
		max_battles = 1,
		decks = { "KingDepthcon.txt" },
		rewards = {
			{ card = "Crystal Memory", gold = 90 },
		},
		ai = { personality = "adaptive" },
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
		appearance = "oren",
		crest = "verdant",
		max_battles = 1,
		decks = { "NPC/Oren.txt" },
		rewards = {
			{ card = "Cryptic Totem", gold = 160 },
		},
		ai = { personality = "adaptive" },
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
		appearance = "generic-female-2",
		max_battles = 1,
		decks = { "AnristVhal.txt", "StormWrangler.txt" },
		rewards = {
			{ card = "Barkwhip, the Smasher", gold = 95 },
		},
		ai = { personality = "ramp" },
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
		appearance = "generic-male-6",
		max_battles = 1,
		decks = { "AnristVhal.txt" },
		rewards = {
			{ card = "Essence Elf", gold = 95 },
		},
		ai = { personality = "sacrifice" },
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
		appearance = "generic-male-5",
		max_battles = 4,
		decks = { "StormWrangler.txt" },
		rewards = {
			{ card = "Dimension Gate", gold = 100 },
		},
		ai = { personality = "adaptive" },
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
		sight = { range = 8 },
		appearance = "generic-male-7",
		max_battles = 1,
		decks = { "Nature Generic 1.txt" },
		rewards = {
			{ card = "Aqua Sniper", gold = 200 },
		},
		ai = { personality = "tempo" },
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
		sight = { range = 8 },
		appearance = "generic-male-8",
		max_battles = 1,
		decks = { "Water Generic 1.txt" },
		rewards = {
			{ card = "Storm Shell", gold = 200 },
		},
		ai = { personality = "ramp" },
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
		sight = { range = 8 },
		appearance = "generic-male-1",
		max_battles = 1,
		decks = { "AstrocometDragon.txt" },
		rewards = {
			{ card = "Astrocomet Dragon", gold = 115 },
		},
		ai = { personality = "aggressive" },
		dialogue = {
			greeting = "The freight crews abandoned this cut. Good. Now there is room for a proper duel.",
			defeat = "You crossed the exposed ridge without flinching. That is worth remembering.",
			victory = "Out here, hesitation gets buried under the next rockfall.",
			complete = "The ridge has tested you four times. I will not pretend a fifth would reveal anything new."
		}
	},
	{
		id = "old-road-duelist-1",
		name = "Mara Flintway",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-4",
		max_battles = 1,
		decks = { "RoaringGreathorn.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 200 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-2",
		name = "Shi Li",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "generic-male-4",
		max_battles = 1,
		decks = { "Dark Generic 1.txt" },
		rewards = {
			{ card = "Bloody Squito", gold = 200 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-3",
		name = "Amber",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-2",
		max_battles = 1,
		decks = { "TrenchdiveShark.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-4",
		name = "Pol",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-male-9",
		max_battles = 4,
		decks = { "Fire Generic 1.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-5",
		name = "Ponna",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-4",
		max_battles = 4,
		decks = { "Fire Generic 2.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-6",
		name = "Clara",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-3",
		max_battles = 4,
		decks = { "AquaSniper.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-7",
		name = "Olmec",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-male-2",
		max_battles = 1,
		decks = { "Zagaan.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-8",
		name = "Totiana",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-7",
		max_battles = 1,
		decks = { "Dark Generic 2.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-9",
		name = "Flora",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-8",
		max_battles = 4,
		decks = { "Nature Generic 2.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
	{
		id = "old-road-duelist-10",
		name = "Berry",
		kind = "route_duelist",
		sight = { range = 8 },
		appearance = "generic-female-1",
		max_battles = 4,
		decks = { "WorldTree.txt" },
		rewards = {
			{ card = "Roaring Great-Horn", gold = 115 },
		},
		ai = { personality = "adaptive" },
		dialogue = {
			greeting = "Wayfarer Camp is safe, but the eastern loop is not. Show me your deck before you take it.",
			defeat = "That will do. You have the nerve to explore beyond the road stones.",
			victory = "A campfire is no substitute for preparation.",
			complete = "You have earned the road's trust four times over. Go find what waits beyond the marked trail."
		}
	},
}
