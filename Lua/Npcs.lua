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
-- while duel, trade, and wandering are independently configurable. Traders
-- must set shop_stock to an id from Lua/ShopStock.lua, or to "mercer" for
-- Mercer's shard-based inventory. Town NPCs with duel enabled, route duelists,
-- and bosses require max_battles plus
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
--     shop_stock = "cinderrail", -- required when trade is true
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
            greeting = "I use Darkness cards. Ready to duel?",
            defeat = "I will not repeat the mistake I made during that turn.",
            victory = "You discarded cards that could have helped you.",
            complete = "We have stabilized four echoes. Zagaan recognizes you, and I have no more rewards to offer.",
            clue = "That broken-circle symbol belongs to the Curator, a duelist who steals the memories bound to cards.",
            investigation = "Someone deliberately removed the names and memories from these cards.",
            stabilize_before = "A full duel may stabilize Zagaan's fading echo. Show me how your deck performs.",
            stabilize_after = "Zagaan's echo is stable. Find other duelists before the Curator detects it.",
            boss_reveal = "The masked figure at the bridge has no living echo. Be careful.",
            act_complete = "The Curator failed to erase some memories on the old road. Go there and investigate."
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
			{ card = "Fighter Dual Fang", gold_tier = 1 },
			{ card = "Corile", gold_tier = 1 },
			{ card = "Craze Valkyrie, the Drastic", gold_tier = 1 },
        },
        ai = { personality = "tempo", difficulty = "easy" },
        dialogue = {
            greeting = "Show me how well you respond when the duel changes.",
            defeat = "You changed your strategy after I committed to mine. I will account for that next time.",
            victory = "You committed to your plan too early. Review when you stopped adapting.",
            complete = "You have beaten me four times. Aqua Sniper's echo is safe with you.",
            investigation = "Every card in town began fading at the same moment, so one source probably caused it.",
            stabilize_before = "My signature card is losing detail. A decisive duel may restore its echo.",
            stabilize_after = "The echo is stable. We need two more stable echoes to locate the source.",
            boss_reveal = "The distortion at the bridge is tracking our restored echoes. The masked duelist planned this.",
            act_complete = "I do not know what is on the old road. Go there and investigate."
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
            greeting = "I build mana before I attack. Let us see if you can stop me.",
            defeat = "Your deck became strong before mine did. You earned that victory.",
            victory = "You did not build enough mana to support your strongest cards.",
            complete = "Great-Horn responds to you without hesitation. You have earned its trust.",
            investigation = "I can repair roads and bridges, but I cannot restore a missing card name.",
            stabilize_before = "Duel me. A familiar battle may help my creatures recover their echoes.",
            stabilize_after = "My bond with these creatures is stable again. Help the other duelists do the same.",
            boss_reveal = "We will hold the town. You take the bridge.",
            act_complete = "The northern bridge is safe. Cross it when you are ready."
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
            { card = "Hanusa, Radiance Elemental", gold_tier = 2 },
			{ card = "Diamond Cutter", gold_tier = 2 },
			{ card = "Alcadeias, Lord of Spirits", gold_tier = 2 },
			{ card = "Warlord Ailzonius", gold_tier = 3 },
        },
        ai = { personality = "control", difficulty = "medium" },
        dialogue = {
            greeting = "I punish reckless attacks. Shall we begin?",
            defeat = "Your attack was measured, not reckless. I concede.",
            victory = "You waited too long. Delay only when it improves your position.",
            complete = "Hanusa recognizes your discipline. I have no harder trial to offer here.",
            clue = "When the festival lights failed, the marks on every blank card indicated the northern bridge. Rowan ran there before he vanished.",
            investigation = "The festival wards were not broken. Someone altered them to admit an unidentified visitor.",
            stabilize_before = "Let us hold the exhibition duel that was interrupted. A formal match may restore the echo.",
            stabilize_after = "The echo is stable. Earn the trust of two more duelists.",
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
			{ card = "Armored Blaster Valdios", gold_tier = 2 },
			{ card = "Magmadragon Jagalzor", gold_tier = 2 },
			{ card = "Uberdragon Bajula", gold_tier = 3 },
        },
        ai = { personality = "rush", difficulty = "medium" },
        dialogue = {
            greeting = "My Dragons need a strong opponent. Ready to duel?",
            defeat = "You survived my early attacks and made a stronger counterattack. Let us duel again soon.",
            victory = "You cannot protect every shield. Punish me after I break one.",
            complete = "You beat me four times. Astrocomet Dragon accepts you.",
            clue = "I saw a masked duelist beside the arena. Their cards had no civilization mark—and no names.",
            investigation = "The arena junction was altered by someone who knew exactly how our equipment worked.",
            stabilize_before = "A serious duel may restore this Fire echo. Try to keep up.",
            stabilize_after = "That worked. The Dragon's name is clear again. Find two more fading echoes.",
            boss_reveal = "Stop the masked duelist. We will protect Emberglen while you are gone.",
            act_complete = "Next time I am coming with you. I want to help with the search."
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
            greeting = "I use the graveyard to strengthen my deck. Ready to duel?",
            defeat = "That loss taught me something useful.",
            victory = "You protected your creatures when sacrificing them could have won the duel.",
            complete = "Your bond with Deathliger's echo is stable. Treat the card well.",
            investigation = "The graveyard became silent during the blackout. The destroyed cards also lost their histories.",
            stabilize_before = "A difficult duel may help the fading echo recover its memory.",
            stabilize_after = "The echo has recovered. Other fading cards need the same help.",
            boss_reveal = "The masked figure is being sustained by a spell. Break the spell, not only the mask.",
            act_complete = "The old road contains active card echoes. I wish I could examine them."
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
            greeting = "My deck rewards patience. Can yours win before it takes control?",
            defeat = "You did not rush when the opening appeared. That restraint won the duel.",
            victory = "You made the moves I expected. Change your plan when your opponent predicts it.",
            complete = "King Depthcon knows how you play now. Another duel would teach us nothing new.",
            investigation = "Nobody leaves by the southern road until we know whether the fading can spread through a caravan.",
            stabilize_before = "A long duel may give the echo enough time to recover. Let us test it.",
            stabilize_after = "The echo is stable. Find other signature cards and check whether they respond the same way.",
            boss_reveal = "I have the southern road. Do what must be done at the bridge.",
            act_complete = "The northern route is dangerous. Move carefully and do not rush."
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
            greeting = "My Nature deck gains strength over time. Ready to duel?",
            defeat = "Your deck adapted faster than mine. You deserved to win.",
            victory = "Your plan was sound, but you did not build enough mana to use it.",
            complete = "Deathblade Beetle recognizes you as an ally. Treat it well.",
            investigation = "The creatures no longer recognize their bonded duelists, so they have begun wandering.",
            stabilize_before = "Duel me. Working together in battle may restore my creatures' memories.",
            stabilize_after = "My creatures recognize one another again. Other bonds in town still need help.",
            boss_reveal = "The western paths are guarded. Bring Rowan home.",
            act_complete = "The path north is open. Continue when you are ready."
        }
    },
    {
        id = "mercer",
        name = "Mercer",
        kind = "town_npc",
        options = { trade = true, wander = false },
		shop_stock = "mercer",
        appearance = "People1-5",
        ai = { personality = "tempo", difficulty = "medium" },
        dialogue = {
            greeting = "Welcome. I sell cards for gold.",
            shop_early = "These blank card fragments started appearing after the festival. Bring me gold and I'll keep you supplied.",
            shop_late = "The restored echoes have remained stable. The Curator probably knows that.",
            act_complete = "I am taking my traveling shop to the old road. You can buy from me there."
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
            greeting = "The Curator claims every echo you restored. Hand them over.",
            defeat = "This body is expendable. The Curator already knows which echoes you restored.",
            victory = "The Hollow Deck will suppress your restored echoes.",
            complete = "The Curator has already crossed the old road. Rowan still lives—but not for long."
        }
    },

	--Cinderrail
	{
        id = "brassa",
        name = "Brassa",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor4-5",
        max_battles = 1,
		crest = "forge",
        decks = { "NPC/Brassa.txt" },
        rewards = {
            { card = "Q-Tronic Gargantua", gold_tier = 2 },
        },
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "My Survivors share abilities with one another. Show me how you handle a full team.",
			defeat = "You removed my Survivors before they could support one another.",
			victory = "You left too many Survivors in play, so Q-tronic Gargantua broke several shields.",
			complete = "You defeated my Survivor deck and earned the Forge Crest. I have no harder test for you.",
			talk = "I train Survivor teams for foundry emergencies. Each member supports the others."
		}
    },
	{
        id = "pip",
        name = "Pip",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People1-3",
        max_battles = 1,
        decks = { "DeathbladeBeetle.txt" },
        rewards = {
            { card = "Deathblade Beetle", gold_tier = 1 },
        },
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "These beetles chew through slag. Want to see how they handle shields?",
			defeat = "You attacked before I could establish my defense. Good timing.",
			victory = "You kept attacking my blockers instead of my vulnerable creatures.",
			complete = "That is enough testing for this shift. The beetles need a rest.",
			talk = "I collect the beetles that nest beneath the ore carts. They usually do not hurt anyone."
		}
    },
	{
        id = "noma",
        name = "Noma",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-2",
        max_battles = 1,
        decks = { "AstrocometDragon.txt" },
        rewards = {
            { card = "Astrocomet Dragon", gold_tier = 1 },
        },
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "My Astrocomet Dragon deck attacks quickly. Want to duel?",
			defeat = "You kept attacking when I expected you to retreat. That was bold.",
			victory = "You hesitated long enough for me to summon my Dragons.",
			complete = "That is enough dueling for now. I need to return to work.",
			talk = "The night crews say the foundry smoke blocks their view of the stars. They are right."
		}
    },
	{
        id = "bram",
        name = "Bram",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-3",
        max_battles = 1,
        decks = { "Fire Generic 1.txt" },
        rewards = {
            { card = "Crimson Hammer", gold_tier = 1 },
        },
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "I use an aggressive Fire deck. Duel me.",
			defeat = "You stayed calm during my early attacks. Well done.",
			victory = "You left your defense open, and my Fire creatures took advantage.",
			complete = "The test is finished. Your deck is ready for stronger opponents.",
			talk = "I tune burner valves by listening to them. The wrong sound means the steel may be ruined."
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
            { card = "Urth, Purifying Elemental", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Urth can control a disordered field. Let me test your deck.",
			defeat = "You continued making good choices when the board became complicated. Well done.",
			victory = "You had strong cards, but you did not use them in a coordinated plan.",
			complete = "The trial is over. Use the same discipline against other opponents.",
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
            { card = "Hanusa, Radiance Elemental", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "Hanusa makes my shields difficult to break. Show me how you attack them.",
			defeat = "You found the weakness in my defense.",
			victory = "My defense made each of your attacks cost too many cards.",
			complete = "One duel is enough. I have repairs to calculate.",
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
			greeting = "My Water deck takes time to establish control. Want to duel?",
			defeat = "You won before I could disrupt your plan.",
			victory = "You reacted to my minor plays and missed the threat I was preparing.",
			complete = "That is enough for now. I will duel another traveler next.",
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
            { card = "Crimson Hammer", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "I like fast contests. Show me how quickly your deck can win.",
			defeat = "Fast and accurate? That hardly seems fair.",
			victory = "You waited too long to use your opening.",
			complete = "The duel is over. I am going back to the rail hammers.",
			talk = "I race the sparks from the rail hammers. The foreman does not consider that a real competition."
		}
    },
	{
        id = "ansa",
        name = "Ansa",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People3-1",
        max_battles = 1,
        decks = { "Valkrowzer.txt" },
        rewards = {
            { card = "Valkrowzer, Ultra Rock Beast", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "My Darkness deck relies on hidden counterattacks. Ready to duel?",
			defeat = "You were prepared for my Darkness cards. I underestimated you.",
			victory = "You focused on your own plan and failed to prepare for my counterattack.",
			complete = "I know how you play now. Another duel would not prove anything.",
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
        decks = { "Doboulgyser.txt" },
        rewards = {
            { card = "Doboulgyser, Giant Rock Beast", gold_tier = 1 },
        },
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "My deck uses powerful Rock Beasts. Can your deck stop them?",
			defeat = "You avoided my strongest creatures and attacked elsewhere. Smart.",
			victory = "You tried to block every attack after my strongest creatures were already in play.",
			complete = "The duel is over. Good work.",
			talk = "I handle the ore teams. Machines are louder, but creatures know when a bridge is unsafe."
		}
	},
	{
        id = "veld",
        name = "Veld",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People4-6",
        max_battles = 1,
        decks = { "Valdios.txt" },
        rewards = {
			{ card = "Armored Blaster Valdios", gold_tier = 1 },
        },
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "Valdios becomes stronger when I assemble a team of Humans. Stop me before I do.",
			defeat = "You disrupted my team before Valdios could lead it. Your timing was excellent.",
			victory = "You let me assemble enough Humans to overwhelm your defense.",
			complete = "The formation test is finished. I have armor plates to refit.",
			talk = "I fit practice armor for the workers' duel club. It must survive a match without becoming too heavy for a real shift."
		}
	},
	{
        id = "kanna",
        name = "Kanna",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People3-2",
        max_battles = 1,
        decks = { "Balbaro.txt" },
        rewards = {
            { card = "Crimson Hammer", gold_tier = 1 },
        },
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "Balbaro gains power from every Human I summon. Let us see if your defense holds.",
			defeat = "You removed my Humans before Balbaro reached full power.",
			victory = "My Humans worked together and made Balbaro strong enough to win.",
			complete = "The test is complete. I need to check the cannon.",
			talk = "I mark safe firing lanes across the test yard. Anyone who paints over them gets broom duty for a month."
		}
	},
	{
        id = "blaze",
        name = "Blaze",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People5-3",
        max_battles = 1,
        decks = { "Dragonoids.txt" },
        rewards = {
            { card = "Deadly Fighter Braid Claw", gold_tier = 1 },
        },
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "My Dragonoids attack from the first turn. Ready for a fast duel?",
			defeat = "You survived my early attacks and defeated the creatures I had left.",
			victory = "You took too long to respond to Braid Claw.",
			complete = "That is enough for now. I will save my next challenge for someone else.",
			talk = "The name is a workshop nickname. Stand too close when I open a furnace and you will understand why."
		}
	},
	{
        id = "drake",
        name = "Drake",
        kind = "town_npc",
        options = { duel = true },
        appearance = "People7-2",
        max_battles = 1,
        decks = { "DragonRamp.txt" },
        rewards = {
            { card = "Bolshack Dragon", gold_tier = 1 },
        },
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "Cocco Lupia helps me summon powerful Dragons. Can you stop me before that happens?",
			defeat = "You attacked before I could summon my Dragons. That was the right choice.",
			victory = "You left Cocco Lupia in play long enough for me to summon several Dragons.",
			complete = "That is enough dueling for now. I need to feed my Dragons.",
			talk = "I chart warm updrafts above the foundry so courier birds—and the occasional Dragon—stay clear of the smoke towers."
		}
	},
	{
		id = "shopkeeper-brant",
		name = "Shopkeeper Brant",
		kind = "town_npc",
		options = { trade = true, wander = false },
		shop_stock = "cinderrail",
		appearance = "People4-5",
		dialogue = {
			greeting = "Welcome to Brant's Foundry Cards. I sell clean cards, durable sleeves, and other supplies at fair prices.",
			shop_early = "The latest shipment includes Fire creatures, support cards, and cards for aggressive decks.",
			shop_late = "The outer-road caravans are running again, so I now have rarer evolution cards. They cost more gold.",
			act_complete = "The foundry is clean, the rails are open, and my stock has passed inspection."
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
			{ card = "Crystal Lancer", gold_tier = 1 },
			{ card = "Hydrooze, the Mutant Emperor", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "A good plan changes when new information appears. Show me how you adapt.",
			defeat = "You used the available information without relying on it too much. You played well.",
			victory = "You followed the plan I expected you to follow.",
			complete = "Three trials are enough. Continue to question predictions that seem too accurate.",
			investigation = "The engine knows private decisions no manifest could contain. That knowledge was taken, not inferred.",
			talk = "A reliable investigator separates known facts from guesses and records who authorized access to the information."
		}
	},
	{
        id = "bryne",
        name = "Bryne",
        kind = "town_npc",
        options = { duel = true },
        appearance = "Actor4-7",
        max_battles = 1,
		decks = { "NPC/Bryne.txt" },
        rewards = {
            { card = "Legendary Bynor", gold_tier = 2 },
        },
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "My deck uses Leviathans and cards that return creatures to your hand. Ready to duel?",
			defeat = "You stopped my Leviathans before I could evolve one into Legendary Bynor.",
			victory = "Legendary Bynor let my Water creatures attack without being blocked.",
			complete = "You have already defeated my strongest Water deck. We do not need a rematch.",
			talk = "I inspect the deep channels used by large creatures. Ships must keep those channels clear."
		}
    },
	{
		id = "pell",
		name = "Pell",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-5",
		max_battles = 1,
		decks = { "Water Generic 3.txt" },
		rewards = {
			{ card = "Eureka Charger", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Conditions change quickly at this port. Show me that your deck can adapt.",
			defeat = "You adjusted your plan before I made my move. Good work.",
			victory = "You reacted too late. Prepare before your opponent's plan is complete.",
			complete = "You know how my deck works now. I have nothing new to test you with.",
			talk = "Long Quay never truly stops. Even at low tide, someone is tying off a ferry or arguing with a manifest."
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
		ai = { personality = "control", difficulty = "easy" },
		dialogue = {
			greeting = "I track every card in play and in hand. Let us see if you do the same.",
			defeat = "You managed your cards better than I managed mine.",
			victory = "You ignored risks because you had not accounted for them.",
			complete = "Our match is settled. Another one would add nothing useful.",
			talk = "A sealed ledger is useful only when people trust the seal. Glasswater lost money when forged seals were accepted."
		}
	},
	{
		id = "sol",
		name = "Sol",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People5-2",
		max_battles = 1,
		decks = { "Hanusa.txt" },
		rewards = {
			{ card = "Hanusa, Radiance Elemental", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "I know every route through the port, but I do not know your strategy. Show me.",
			defeat = "I predicted your plan incorrectly. I will revise my notes.",
			victory = "You followed the sequence of plays I predicted.",
			complete = "One duel is enough. I will record that your strategy is difficult to predict.",
			talk = "Road charts record fixed features. Current charts must be updated whenever the water changes."
		}
	},
	{
		id = "alexei",
		name = "Alexei",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People4-5",
		max_battles = 1,
		decks = { "KingDepthcon.txt" },
		rewards = {
			{ card = "King Depthcon", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Harbor safety requires watching for threats below the surface. Let Depthcon test your awareness.",
			defeat = "You recognized my plan before my strongest creatures appeared. Good work.",
			victory = "You focused on my visible creatures and missed the ones I was preparing.",
			complete = "Depthcon has tested you. We do not need another duel.",
			talk = "I inspect the pilings beneath the canal bridges because cracks are difficult to see from above the water."
		}
	},
	{
		id = "elmira",
		name = "Elmira",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People5-2",
		max_battles = 1,
		decks = { "Water Generic 2.txt" },
		rewards = {
			{ card = "Aqua Guard", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "These locks protect half the South Canals. Show me your defense is as quick as Aqua Guard.",
			defeat = "You attacked before I could establish my defense. Good timing.",
			victory = "You gave Aqua Guard enough time to stop your attack.",
			complete = "Our test is complete. I have nothing more to teach you today.",
			talk = "I keep the canal gates balanced. Too much water floods a kitchen; too little strands every delivery boat."
		}
	},
	{ --worker on the port
		id = "remy",
		name = "Remy",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-5",
		max_battles = 1,
		decks = { "Water Generic 1.txt" },
		rewards = {
			{ card = "Marine Flower", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "I have time for one duel before my shift ends. Can your deck win quickly?",
			defeat = "You played quickly without making careless mistakes. You would do well on this crew.",
			victory = "You acted quickly without checking your position first. That mistake cost you the duel.",
			complete = "My shift is over, so I am done dueling for today.",
			talk = "We unload by color at Long Quay: blue for food, violet for cards, green for Rootmaze timber."
		}
	},
	{
		id = "captain-brock",
		name = "Captain Brock",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-4",
		max_battles = 1,
		decks = { "KingTsunami.txt" },
		rewards = {
			{ card = "Emeral", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "A captain must trust the conditions, the crew, and the available cards. Let us test your judgment.",
			defeat = "You stopped King Tsunami without weakening your position. I trust your judgment.",
			victory = "You were not prepared when I summoned King Tsunami.",
			complete = "I recorded the match. My crew will discuss it at supper.",
			talk = "My ship runs the lake route. I am always relieved to see Glasswater's lighthouse after three nights of fog."
		}
	},
	{ --a scholar
		id = "daniel",
		name = "Daniel",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-2",
		max_battles = 1,
		decks = { "AquaSniper.txt" },
		rewards = {
			{ card = "Aqua Sniper", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Aqua Sniper lets me control opposing creatures from a safe position. Shall we test it?",
			defeat = "You attacked before I could establish control. That disproves my plan.",
			victory = "The choices you made several turns ago decided the duel.",
			complete = "One result is enough for my notes. Thank you.",
			talk = "I study how duelists value hidden information. The Prediction Hall has made that research uncomfortably practical."
		}
	},
	{
		id = "ekki",
		name = "Ekki",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People3-1",
		max_battles = 1,
		decks = { "KingDepthcon.txt" },
		rewards = {
			{ card = "Crystal Memory", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "easy" },
		dialogue = {
			greeting = "Crystal Memory helps me find the card I need. Let us see whether you can win first.",
			defeat = "You found the play you needed before I found mine.",
			victory = "Crystal Memory found the right card at the right time.",
			complete = "I recorded what this match taught me.",
			talk = "I copy damaged harbor records before the ink runs. Water is excellent for trade and terrible for paperwork."
		}
	},
	{
		id = "rama",
		name = "Rama",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People2-5",
		max_battles = 1,
		decks = { "KingDepthcon.txt" },
		rewards = {
			{ card = "Crystal Memory", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "A harbor pilot chooses a route before conditions become dangerous. Choose your opening carefully.",
			defeat = "You changed your plan without losing your advantage. That was well done.",
			victory = "You committed to a weak plan and gave Depthcon control of the board.",
			complete = "I know how you play now. Another duel would teach us little.",
			talk = "I guide heavy ships past the glass shoals. The safest line changes with every tide."
		}
	},
	{ --a child
		id = "elisa",
		name = "Elisa",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People1-2",
		max_battles = 1,
		decks = { "Water Generic 3.txt" },
		rewards = {
			{ card = "King Mazelan", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Want to see my deck?",
			defeat = "You stopped every play I tried. That was amazing.",
			victory = "I returned your best creature to your hand. Now you have to summon it again.",
			complete = "I have to go home now. I promised to stop after one duel.",
			talk = "The fish under Glassgarden know when school ends. They all swim over for crumbs at once."
		}
	},
	{ 
		id = "champion-edison",
		name = "Champion Edison",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People4-3",
		max_battles = 1,
		decks = { "More2/WN Origins Evo.txt" },
		rewards = {
			{ card = "Fighter Dual Fang", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "I am the arena champion because I adapt during a match. Show me a strategy I cannot answer.",
			defeat = "You used a strategy I was not prepared for. Excellent.",
			victory = "You challenged the creature and forgot to plan for what it would become.",
			complete = "One exhibition is enough. I need to face a new challenger next.",
			talk = "Neris studies every official match. I study challengers whose new strategies make his notes outdated."
		}
	},
	{ 
		id = "mother-melissa",
		name = "Mother Melissa",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People6-3",
		max_battles = 1,
		decks = { "More2/2 - L Diamond Cut.txt" },
		rewards = {
			{ card = "Diamond Cutter", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Good defense depends on acting at the right time. Let us test your timing.",
			defeat = "You attacked at the right time without leaving your own position exposed.",
			victory = "You hesitated when you needed to act. My blockers were ready.",
			complete = "That settles our match. Come by the public kitchen if you need a warm meal.",
			talk = "Everyone calls me Mother Melissa because I have fed the dock crews during many storms."
		}
	},
	{ 
		id = "loki",
		name = "Loki",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor4-1",
		max_battles = 1,
		decks = { "Deathliger.txt" },
		rewards = {
			{ card = "Deathliger, Lion of Chaos", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "I duel beyond the warning ropes where officials will not interrupt. Deathliger is ready for you.",
			defeat = "You stayed organized after I disrupted your plan. Annoying.",
			victory = "Every defense has a weakness, and I found yours.",
			complete = "Fine. One official loss is enough trouble for today.",
			talk = "The east breakwater is closed after dark. That makes it quieter, not less interesting."
		}
	},
	{ --a child
		id = "tissa",
		name = "Tissa",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor4-2",
		max_battles = 1,
		decks = { "Water Generic 2.txt" },
		rewards = {
			{ card = "Aqua Guard", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "I can operate every practice lock in the schoolyard. I think Aqua Guard can beat you.",
			defeat = "You opened my whole defense! Show me how after class.",
			victory = "My defense stopped you. That means I win.",
			complete = "Teacher said one duel, and I am definitely not getting extra homework.",
			talk = "We race little boats through the school locks. Mine is blue and has three emergency blockers."
		}
	},
	{ 
		id = "emma",
		name = "Emma",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor3-8",
		max_battles = 1,
		decks = { "Water Generic 1.txt" },
		rewards = {
			{ card = "Aqua Hulcus", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Aqua Hulcus draws extra cards for me. Can your deck keep up?",
			defeat = "You made better use of every card, even when I drew ahead.",
			victory = "Drawing one extra card gave me the answer I needed.",
			complete = "Good match. I have a stack of school notices to deliver before the next bell.",
			talk = "I carry messages between the school and Port Authority. The bridges make it faster than waiting for a canal boat."
		}
	},
	{ 
		id = "professor-macron",
		name = "Professor Macron",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People2-1",
		max_battles = 1,
		decks = { "LW Generic 1.txt" },
		rewards = {
			{ card = "Spiral Gate", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Spiral Gate changes the position of creatures. Show me how you respond to it.",
			defeat = "You used the forced movement to improve your position. Full marks.",
			victory = "You tried to keep one creature in play instead of preserving your overall strategy.",
			complete = "The practical examination is over. There will be no written portion.",
			talk = "At South Canal School we teach navigation, arithmetic, and why recording a duel requires permission."
		}
	},
	{ 
		id = "lady-aqua",
		name = "Lady Aqua",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor2-1",
		max_battles = 1,
		decks = { "Quix/MonoWater.txt" },
		rewards = {
			{ card = "Crystal Lancer", gold_tier = 2 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "A skilled Water duelist adapts without losing control. Demonstrate your method.",
			defeat = "You changed your formation effectively. Crystal Lancer could not control it.",
			victory = "Careful play defeated your more dramatic attacks.",
			complete = "I have evaluated your technique. I do not need to do it again.",
			talk = "Lady Aqua is an arena title, not a claim of nobility. Glasswater audiences enjoy a little ceremony."
		}
	},
	{ 
		id = "olli",
		name = "Olli",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor4-7",
		max_battles = 1,
		decks = { "FrostSpecter.txt" },
		rewards = {
			{ card = "Frost Specter, Shadow of Age", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "Frost Specter uses cards in the graveyard. Want to duel?",
			defeat = "You disrupted my plan before I could use the graveyard.",
			victory = "I used the cards in my graveyard to defeat you.",
			complete = "I have to return to work in cold storage.",
			talk = "I maintain the Fish Market cold rooms. I think the noises after midnight come from the pipes."
		}
	},
	{ 
		id = "oak",
		name = "Oak",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor5-1",
		max_battles = 1,
		decks = { "WN Generic 1.txt" },
		rewards = {
			{ card = "Emeral", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "I use Nature and Water cards so I can change strategy during a duel. Can your deck adapt?",
			defeat = "You identified the weakness in my strategy and exploited it.",
			victory = "You continued with the same plan while I adapted to it.",
			complete = "Your deck passed my test. We do not need another duel.",
			talk = "I shape imported timber at the shipyard. Rootmaze timber is difficult to work, but it is durable."
		}
	},
	{ --a schoolgirl
		id = "polina",
		name = "Polina",
		kind = "town_npc",
		options = { duel = true },
		appearance = "Actor4-6",
		max_battles = 1,
		decks = { "Fire Generic 1.txt" },
		rewards = {
			{ card = "Gatling Skyterror", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "Everyone here plays Water, so I brought a Fire Dragon. Want to duel?",
			defeat = "You stopped every Fire card I played.",
			victory = "My Fire deck can win even against Glasswater's Water decks.",
			complete = "One match was the agreement. I still have navigation homework.",
			talk = "The school weather gauge predicts rain whenever I plan outdoor practice. It has been accurate every time."
		}
	},
	{ 
		id = "arka",
		name = "Arka, Keeper of Trinkets",
		kind = "town_npc",
		options = { trade = true, wander = false },
		shop_stock = "glasswater",
		appearance = "People8-1",
		dialogue = {
			greeting = "Welcome to Current and Quill Exchange. I offer fair prices and keep every card dry.",
			shop_early = "Today's stock includes Water cards, blockers, and undamaged imports.",
			shop_late = "The inventory records are current and the shelves are full. Buy what you need before it sells out.",
			act_complete = "Glasswater is trading openly again. Every card on these shelves has a manifest and a willing seller."
		}
	},

	--Gloam Quarry
	{
		id = "vey",
		name = "Sister Vey",
		kind = "town_npc",
		options = { duel = true, wander = false },
		appearance = "Actor3-2",
		crest = "ashen",
		max_battles = 1,
		decks = { "NPC/Sister Vey.txt" },
		rewards = {
			{ card = "Death Phoenix, Avatar of Doom", gold_tier = 2 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "The arena is open. I can offer you the Ashen Crest match.",
			talk = "Sister is a civic title here. I keep the memorial records and oversee the arena. Anyone can visit the Ashvault; you do not need to win a duel first.",
			defeat = "You understood what each sacrifice would cost. The Ashen Crest and Death Phoenix are yours.",
			victory = "You used up the creatures you needed for your next play. Decide what you can afford to lose before you attack.",
			complete = "Your Ashen Crest match is recorded. I hope you will stay to meet the people who live here."
		}
	},
	{
		id = "dema",
		name = "Dema",
		kind = "town_npc",
		options = { duel = true, wander = true },
		appearance = "People3-5",
		max_battles = 1,
		decks = { "FD Generic 1.txt" },
		rewards = {
			{ card = "Jack Viper, Shadow of Doom", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "My shift is over. Have time for a duel?",
			talk = "I check the warning flags before the crew starts work. Someone swapped mine for laundry once. We found out whose when it began to rain.",
			defeat = "You stopped my early attacks and still had creatures left. Take Jack Viper; it can help you recover yours.",
			victory = "You waited for a big creature while my smaller ones kept attacking.",
			complete = "That was my practice match for today. Now I need to return these flags before Beren counts them."
		}
	},
	{
		id = "rell",
		name = "Rell",
		kind = "town_npc",
		options = { duel = true, wander = true },
		appearance = "People2-3",
		max_battles = 1,
		decks = { "LD Late.txt" },
		rewards = {
			{ card = "Stinger Worm", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "The lamps are checked. I have time for a duel.",
			talk = "Violet lamps belong beside the memorials. I test the green ones by the southern outlook. Damp air finds every crack in the glass.",
			defeat = "You kept enough cards to get past my blockers. Stinger Worm will give you another choice to consider.",
			victory = "Your attack stopped at my blockers, and you had no cards left to try something else.",
			complete = "One match is enough before the evening rounds. There are more lamps here than people think."
		}
	},
	{
		id = "ivo",
		name = "Ivo",
		kind = "town_npc",
		options = { duel = true, wander = false },
		appearance = "People2-1",
		max_battles = 1,
		decks = { "WD Generic 1.txt" },
		rewards = {
			{ card = "Terror Pit", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "The copying is finished. We can use a clear table for a duel.",
			talk = "Families check each inscription before the carver starts. Last week a grandson corrected a date that had been wrong in our ledger for thirty years.",
			defeat = "You made better use of your cards than I did. Terror Pit should be useful in your next match.",
			victory = "I saved my removal for the creatures that could actually defeat me.",
			complete = "I have written down the result. Unlike the family records, my defeat does not need to be carved in stone."
		}
	},
	{
		id = "gloam_nera",
		name = "Shopkeeper Nera",
		kind = "town_npc",
		options = { trade = true, wander = false },
		shop_stock = "gloam_quarry",
		appearance = "People3-8",
		dialogue = {
			greeting = "Welcome to Ash and Ember. Leave the quarry dust on the mat; the cards are clean.",
			talk = "Most of my customers play Darkness. I keep small creatures and recovery spells in stock because an expensive finisher cannot do everything.",
			shop_early = "I have Darkness creatures, removal, and Dark Reversal. Take a look.",
			shop_late = "The latest delivery includes more graveyard recovery and removal. The stock list is up to date."
		}
	},
	{
		id = "gloam_ossa",
		name = "Steward Ossa",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People4-6",
		dialogue = {
			greeting = "First visit? You are at Slatecross, in the middle of Gloam Quarry.",
			talk = "The cardhouse is west of this board and the inn is east. The lower terrace has the clinic, school, and Record House. Vey takes challengers at the arena beyond them."
		}
	},
	{
		id = "gloam_nella",
		name = "Nella",
		kind = "town_npc",
		options = { wander = true },
		appearance = "People3-1",
		dialogue = {
			greeting = "Mind the baskets. I only just got them out of the rain.",
			talk = "I bring vegetables down from the northern farms. The quarry crews buy every onion I carry, then complain that Orin puts onions in everything."
		}
	},
	{
		id = "gloam_beren",
		name = "Beren",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People4-8",
		dialogue = {
			greeting = "The hoist is idle. You can use the steps beside the yard.",
			talk = "I took over from Eska last spring. She still checks my load book after supper. Yesterday she corrected my spelling and left the figures alone, so I think I am improving."
		}
	},
	{
		id = "gloam_tovin",
		name = "Tovin",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People4-5",
		dialogue = {
			greeting = "Give that stone a little room. The carving is finished, but the paint is wet.",
			talk = "I carve names for the Record House and lintels for the homes. Mina asked for a dragon over her door. Her family approved a very small dragon."
		}
	},
	{
		id = "gloam_eska",
		name = "Eska",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People1-8",
		dialogue = {
			greeting = "There is room at the table. Move that empty cup and sit down.",
			talk = "I ran the upper hoist for twenty-seven years. Beren runs it now. I promised I would stop giving advice at supper, so I give it before supper."
		}
	},
	{
		id = "gloam_mina",
		name = "Mina",
		kind = "town_npc",
		options = { wander = true },
		appearance = "People1-4",
		dialogue = {
			greeting = "Have you seen a blue marble? Not a card. A marble.",
			talk = "Sella said we cannot race marbles down the stairs anymore. We marked a course beside the tables instead. Eska keeps score even when she says she is not watching."
		}
	},
	{
		id = "gloam_jessa",
		name = "Jessa",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People1-6",
		dialogue = {
			greeting = "Welcome to the Shale and Spoon. The crews have finished lunch, so you can hear yourself think.",
			talk = "I run the rooms and Orin runs the kitchen. We tried doing it the other way once. Nobody slept well and the soup was worse."
		}
	},
	{
		id = "gloam_orin",
		name = "Orin",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People2-5",
		dialogue = {
			greeting = "The next pot is for the night crew. They will notice if I start serving it early.",
			talk = "Nella brings the onions, the crews bring their appetites, and Jessa tells me when I have made too much. Jessa has never had to tell me that."
		}
	},
	{
		id = "gloam_iona",
		name = "Iona",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People3-3",
		dialogue = {
			greeting = "Please keep your voice down near the beds. Some of the night crew are resting.",
			talk = "I look after the clinic and Dema checks the work crews' equipment. She remembers everyone's inspection date and forgets her own lunch. Orin sends it over."
		}
	},
	{
		id = "gloam_sella",
		name = "Sella",
		kind = "town_npc",
		options = { wander = false },
		appearance = "People2-8",
		dialogue = {
			greeting = "Come in. We have finished lessons for the day.",
			talk = "The children copy short inscriptions to practice their letters. Ivo checks the names, and Tovin lends us scrap stone. Mina always asks whether the spelling matters more than the dragon she has drawn beside it."
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
			greeting = "Building mana gives you more choices, but you still need to choose well. Show me your plan.",
			defeat = "You developed your board according to what the duel required. That patience earned your victory.",
			victory = "You focused on your largest creature instead of the creature that supported your strategy.",
			complete = "I understand how you play now. More duels would teach us nothing new.",
			investigation = "These creatures still recognize their duelists, but the corresponding cards no longer record those bonds. We must reunite them before the false bonds become permanent."
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
			greeting = "I was looking for rare moonberries, but I have time for a duel.",
			defeat = "You found an opening before I found my next play.",
			victory = "You were not prepared for my strongest creature.",
			complete = "That was a good duel. I should return to looking for moonberries."
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
			greeting = "My beetles attack whenever an opponent hesitates. Ready to duel?",
			defeat = "That was a good duel. My beetles are still active, even in the graveyard.",
			victory = "Each creature you lost made my strategy stronger. You needed to stop it sooner.",
			complete = "One trial is enough. I need to tend to my beetles now."
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
			defeat = "Your strategy worked. I did not expect it.",
			victory = "A safer strategy would have worked better, but I respect the attempt.",
			complete = "You have completed my challenge. I will not stop you again."
		}
	},
	{
		id = "orko",
		name = "Orko",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People5-3",
		max_battles = 1,
		decks = { "WorldTree.txt" },
		rewards = {
			{ card = "World Tree, Root of Life", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "World Tree becomes difficult to defeat if it remains in play. Can your deck stop it in time?",
			defeat = "You removed my supporting creatures without weakening your own position. That took good judgment.",
			victory = "You attacked my minor creatures and left the cards supporting my strategy in play.",
			complete = "I understand your strategy now. One duel was enough.",
			talk = "I tend the oldest seed beds near Heartroot. Some saplings move their roots when they dislike their neighbors."
		}
	},
	{
		id = "mazz",
		name = "Mazz",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People7-3",
		max_battles = 1,
		decks = { "WN Generic 1.txt" },
		rewards = {
			{ card = "Bronze-Arm Tribe", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "Rootmaze's paths change often, and I built my deck to adapt. Can yours?",
			defeat = "You forced me to commit to one plan, then countered it.",
			victory = "You prepared for my Nature cards but ignored my Water cards.",
			complete = "One duel is enough. I will revise my deck before we play again.",
			talk = "I carry messages between Waterstep and the southern clearings. Waterproof paper is worth every coin."
		}
	},
	{
		id = "misty",
		name = "Misty",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People6-6",
		max_battles = 1,
		decks = { "Nature Generic 1.txt" },
		rewards = {
			{ card = "Bronze-Arm Tribe", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Some creatures look more dangerous than they are. Show me that you can identify the real threats.",
			defeat = "You understood the board before I did.",
			victory = "You attacked minor threats while I continued building mana.",
			complete = "The duel is settled. We do not need another match.",
			talk = "I gather dew from the broad leaves before sunrise. The nursery uses it for newly awakened card echoes."
		}
	},
	{
		id = "earthkeeper",
		name = "The Earthkeeper",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People5-7",
		max_battles = 1,
		decks = { "NocturnalGiant.txt" },
		rewards = {
			{ card = "Nocturnal Giant", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "medium" },
		dialogue = {
			greeting = "Nocturnal Giant attacks shields whenever it can. Show me that you can stop it.",
			defeat = "You stopped Nocturnal Giant despite its constant attacks. I will report your success.",
			victory = "Your attempts to avoid Nocturnal Giant did not stop its attacks.",
			complete = "Nocturnal Giant is resting. I will not wake it for an unnecessary rematch.",
			talk = "Earthkeeper is a duty, not a birth name. I watch the deep roots for tremors and settle Giants before they wander into homes."
		}
	},
	{
		id = "aziz",
		name = "Aziz",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People7-6",
		max_battles = 1,
		decks = { "BeastFolkNoEvo.txt" },
		rewards = {
			{ card = "Bronze-Arm Tribe", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "medium" },
		dialogue = {
			greeting = "My deck uses Beast Folk without evolution creatures. Want to duel?",
			defeat = "Your strategy developed before my Beast Folk could overpower you.",
			victory = "My trained Beast Folk defeated you before you could play your strongest cards.",
			complete = "My Beast Folk accept the result. I accept it too, though I am disappointed.",
			talk = "I train courier packs for the changing paths. Each creature learns at least three routes home."
		}
	},
	{
		id = "kelso",
		name = "Kelso",
		kind = "town_npc",
		options = { duel = true },
		appearance = "People7-8",
		max_battles = 1,
		decks = { "NL Generic 1.txt" },
		rewards = {
			{ card = "Mana Nexus", gold_tier = 1 },
		},
		ai = { personality = "control", difficulty = "medium" },
		dialogue = {
			greeting = "My Nature cards support my Light blockers. Show me how you defeat that defense.",
			defeat = "You created an opening without using all your resources. Well played.",
			victory = "You spent too many cards removing my support and had nothing left for my blockers.",
			complete = "I have tested you once. Repeating the test would prove nothing.",
			talk = "I maintain the lantern vines along the residential paths. They brighten when someone is lost and dim when the route is safe."
		}
	},
	{
		id = "merchant-gacko",
		name = "Merchant Gacko",
		kind = "town_npc",
		options = { trade = true, wander = false },
		shop_stock = "rootmaze",
		appearance = "People7-7",
		dialogue = {
			greeting = "Welcome to Gacko's Rootmarket Cards. Watch your step, and ask me before feeding any creature on the shelves.",
			shop_early = "The latest stock includes mana support, durable creatures, and cards that search your deck.",
			shop_late = "The roads are stable again. New caravans brought evolution cards and rarer Nature support.",
			act_complete = "Heartroot is healthy, trade has resumed, and the roots have not damaged any crates this week."
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
			greeting = "The western ford changes every week. Duel me so I can see how well you adapt.",
			defeat = "You assessed the situation before committing to a plan. That patience will help you on the northern route.",
			victory = "You chose the quickest strategy instead of the safest one.",
			complete = "You have passed my test. I have nothing more to teach you."
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
			greeting = "I came to map the southern pools, but the water level keeps changing the paths.",
			defeat = "I can map this area even if its paths continue to change.",
			victory = "You focused on one target and ignored the rest of the board.",
			complete = "One survey is enough. Take the shell; it came from this wetland."
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
			greeting = "A good duelist chooses a strategy and commits to it. Come on, let us duel!",
			defeat = "That was a strong final attack. You earned the right of way.",
			victory = "You hesitated before your Fire deck was ready to attack.",
			complete = "Our match is settled. Choose whichever road you want."
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
			greeting = "You can reach several regions from this crossroads. Before you choose one, let us duel!",
			defeat = "Great duel! You used a winning strategy I did not expect.",
			victory = "Once you choose a strategy, commit to it completely!",
			complete = "Our match is settled, but we will both find other rivals."
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
			greeting = "I use discard effects against travelers. Let us see how many cards you can keep.",
			defeat = "Hmph. You held on to exactly the cards I needed gone.",
			victory = "I used the cards in my graveyard to defeat you.",
			complete = "I know how you play now. Keep moving."
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
			greeting = "I guard these roads. Show me that you know how to protect your shields.",
			defeat = "You broke my formation without abandoning your own. Passage granted.",
			victory = "You attacked too quickly and left your defense exposed.",
			complete = "I have evaluated your defense once. You may pass."
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
			greeting = "I have crossed deserts to find rare creatures. Will you give me a challenging duel?",
			defeat = "That match is worth telling my caravan about.",
			victory = "Travelers must prepare for threats they have not encountered before.",
			complete = "One memorable duel is enough. Safe travels."
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
			greeting = "Hear that rumble? Terahorn is ready to duel.",
			defeat = "You avoided Terahorn's attacks. It could not control your creatures.",
			victory = "Terahorn made its attacks obvious, but you did not prepare for them.",
			complete = "The result is final. I will not challenge you again."
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
			defeat = "Do not celebrate. I will prepare for your strategy next time.",
			victory = "Your last useful card vanished before you understood the duel had begun.",
			complete = "I have nothing to gain from defeating the same strategy again."
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
			greeting = "Balbaro can destroy your shield line quickly. Stand aside or duel me.",
			defeat = "You attacked the weakness in my defense. I cannot argue with that result.",
			victory = "You did not prepare for Balbaro's attack.",
			complete = "The cannon needs maintenance. You may go."
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
			greeting = "Great-Horn and I patrol the eastern loop. Duel me so I can decide whether you can travel there safely.",
			defeat = "You made clear choices and managed your cards well. You are ready for the broken trail.",
			victory = "You did not build enough mana before playing your strongest cards.",
			complete = "The inspection is over. I will tell Wayfarer Camp that you passed."
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
			greeting = "My Darkness deck uses the cards you discard. Ready to duel?",
			defeat = "You did not leave any useful cards in your graveyard for me to exploit.",
			victory = "You kept important cards in your hand for too long, so I made you discard them.",
			complete = "We have dueled once. We do not need a rematch."
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
			greeting = "The rain flooded the wagon ruts. I brought a Water deck for the occasion.",
			defeat = "You maintained your strategy despite every disruption.",
			victory = "You committed to your plan before you knew which threats I had prepared.",
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
			greeting = "I use Dragon fire to light the road beacons. Duel me while I take a break.",
			defeat = "You stopped each of my Fire creatures before it could attack.",
			victory = "You let a small early threat become strong enough to win.",
			complete = "The beacon is lit and our duel is over. Continue while the road is visible."
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
			greeting = "The warning signs prohibit open flames, so we must keep this Fire duel under control.",
			defeat = "You stopped all my Fire cards without taking much damage. Impressive.",
			victory = "Volcanic Arrows removed the blockers that protected you.",
			complete = "I promised the wardens I would duel only once here. We should leave before they see us."
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
			greeting = "Aqua Sniper removes creatures that bandits rely on. Want a demonstration?",
			defeat = "You gave me no useful target for Aqua Sniper. Excellent formation.",
			victory = "Returning your strongest creatures to your hand gave me control of the duel.",
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
			greeting = "These mile markers were once grave markers. I use Zagaan in my deck. Ready to duel?",
			defeat = "You showed proper respect to the dead—and none at all to my shields.",
			victory = "My Darkness creatures are strongest here after sunset.",
			complete = "I know your name and how you duel. I will not ask for a rematch."
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
			greeting = "I collect rumors, and I can learn even more by watching how you duel.",
			defeat = "Interesting. You saved your strongest counter until the right moment.",
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
			greeting = "The roadside soil supports little growth. Let us see how well you build mana.",
			defeat = "You developed exactly the board you needed and wasted nothing.",
			victory = "You needed to prepare your mana before summoning creatures.",
			complete = "One duel is enough to evaluate your deck. It has potential."
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
			greeting = "My Psyshroom deck plans several turns ahead. Will you duel me?",
			defeat = "I did not predict your final attack.",
			victory = "I planned three turns ahead, and you did not change your strategy.",
			complete = "One result is enough. We do not need a second duel."
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
			defeat = "You beat me. Keep your coins and leave.",
			victory = "Terror Pit defeated your creature, and now I am taking the toll.",
			complete = "You already defeated my best trap. Trying it again would gain me nothing."
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
			greeting = "I attack first with Braid Claw. You will not have time to ask questions!",
			defeat = "You were supposed to lose before I ran out of creatures!",
			victory = "I attacked before you were ready, and now I am taking your valuables.",
			complete = "No rematch. My surprise attack will not work twice."
		}
	},
	{
		id = "treacherous-duelist-3",
		name = "Bandit Gras",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Evil-1",
		max_battles = 1,
		decks = { "FD Generic 1.txt" },
		rewards = {
			{ card = "Blasto, Explosive Soldier", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "My crew has surrounded you. Duel me or surrender your valuables.",
			defeat = "You attacked me directly instead of fighting the rest of my crew. I did not expect that.",
			victory = "Blasto destroyed your defense. Now I will take your valuables.",
			complete = "This ambush site is no longer useful. Leave before I prepare another one."
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
			greeting = "I will make you discard your cards before I rob you. I prefer to work in that order.",
			defeat = "You kept producing answers after I took them all. Infuriating.",
			victory = "You had no cards left to defend yourself, so the robbery was easy.",
			complete = "I know what you carry now. Trying the same theft twice would be foolish."
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
			greeting = "You embarrassed my crew. I will defeat you and take your deck myself.",
			defeat = "Impossible... I prepared for every strategy except yours.",
			victory = "I am the Banditlord because I duel better than the rest of my crew.",
			complete = "My crew has scattered and I have lost their respect. Take the pass and leave."
		}
	},

	--Blackstone Road duellists
	{
		id = "blackstone-road-duelist-1",
		name = "Kara",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People5-2",
		max_battles = 1,
		decks = { "Dark Generic 2.txt" },
		rewards = {
			{ card = "Ghost Touch", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "I patrol this section of Blackstone Road. Duel me before you continue.",
			defeat = "You kept the cards you needed and removed my creatures efficiently.",
			victory = "Ghost Touch removed a card you needed, and you did not recover.",
			complete = "You passed my test. I will not stop you again."
		}
	},
	{
		id = "blackstone-road-duelist-2",
		name = "Cody",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "People5-1",
		max_battles = 1,
		decks = { "FrostSpecter.txt" },
		rewards = {
			{ card = "Terror Pit", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Frost Specter lets me recover Darkness creatures from my graveyard. Can you stop it?",
			defeat = "You removed Frost Specter before I could reuse my creatures.",
			victory = "I reused my Darkness creatures until your defense was exhausted.",
			complete = "You have seen how my deck works. We do not need a rematch."
		}
	},
	{
		id = "blackstone-road-duelist-3",
		name = "Cindy",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-5",
		max_battles = 1,
		decks = { "HandRemoval.txt" },
		rewards = {
			{ card = "Horrid Worm", gold_tier = 1 },
		},
		ai = { personality = "rush", difficulty = "easy" },
		dialogue = {
			greeting = "My deck forces you to discard cards. Show me that you can win with fewer choices.",
			defeat = "You used your important cards before I could make you discard them.",
			victory = "I emptied your hand before you could build a defense.",
			complete = "The duel is settled. I have nothing more to test."
		}
	},
	{
		id = "blackstone-road-duelist-4",
		name = "Boro",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor3-4",
		max_battles = 1,
		decks = { "IzoristVhal.txt" },
		rewards = {
			{ card = "Bloody Squito", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Izorist Vhal gains power from Darkness creatures in my graveyard. Can you defeat it in time?",
			defeat = "You kept my graveyard small and defeated Izorist Vhal.",
			victory = "I put enough Darkness creatures in my graveyard to make Izorist Vhal too strong for you.",
			complete = "You understand my strategy now. Continue along the road."
		}
	},
	{
		id = "blackstone-road-duelist-5",
		name = "Onixia",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor4-4",
		max_battles = 1,
		decks = { "Gigazald.txt" },
		rewards = {
			{ card = "Phantasmal Horror Gigazald", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Gigazald can make you discard a card whenever I tap it. Ready to duel?",
			defeat = "You removed my Chimeras before I could evolve one into Gigazald.",
			victory = "Gigazald kept removing cards from your hand until you had no useful play.",
			complete = "One duel is enough. You may continue."
		}
	},
	{
		id = "blackstone-road-duelist-6",
		name = "Jacky",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor4-3",
		max_battles = 1,
		decks = { "Jack Viper.txt" },
		rewards = {
			{ card = "Jack Viper, Shadow of Doom", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "Jack Viper returns my destroyed Darkness creatures to my hand. Ready to duel?",
			defeat = "You removed Jack Viper before I could recover my creatures.",
			victory = "Jack Viper let me reuse my Darkness creatures until you ran out of answers.",
			complete = "You have already faced my best strategy. There will be no rematch."
		}
	},
	{
		id = "blackstone-road-duelist-7",
		name = "Kossi",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor5-2",
		max_battles = 1,
		decks = { "Official/1 - LFN Starter Deck.txt" },
		rewards = {
			{ card = "La Ura Giga, Sky Guardian", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "My deck combines Light, Nature, and Fire cards. Can you respond to all three?",
			defeat = "You adapted each time I changed civilizations. Well played.",
			victory = "You prepared for one civilization and had no answer for the other two.",
			complete = "You passed this challenge. Continue when you are ready."
		}
	},
	{
		id = "blackstone-road-duelist-8",
		name = "Abyssia",
		kind = "route_duelist",
		sight = { range = 4 },
		appearance = "Actor5-7",
		max_battles = 1,
		decks = { "Official/4 - FD Deadly Decay.txt" },
		rewards = {
			{ card = "Vashuna, Sword Dancer", gold_tier = 1 },
		},
		ai = { personality = "tempo", difficulty = "easy" },
		dialogue = {
			greeting = "My Fire and Darkness deck combines fast attacks with creature removal. Ready to duel?",
			defeat = "You survived my early attacks and protected your creatures from my removal cards.",
			victory = "My Fire creatures attacked while my Darkness cards removed your blockers.",
			complete = "Our duel is over. I will not challenge you again."
		}
	},
}
