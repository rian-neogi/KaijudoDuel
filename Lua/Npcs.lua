-- Overworld NPC metadata
-- NPC positions are owned by Lua/World.lua and maintained by the World Builder.
--
-- Add one table to the returned array for each NPC. Required fields are:
--   id, name, kind, appearance
-- Duelists and bosses also require max_battles, deck1, and one reward table
-- for every enabled battle. deck2 through deck4 are optional: a missing deck
-- reuses the most recently defined deck.
--
-- Supported kinds: duelist, shopkeeper, boss
-- Supported appearances: mira, marin, rook, aurelia, flint, nyx, tidal,
--                        briar, mercer, veiled_one
--
-- Dialogue is a flat, extensible string table. The current application uses:
--   greeting, defeat, victory, complete, clue, investigation,
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
--     kind = "duelist",
--     appearance = "mira",
--     max_battles = 4,
--     deck1 = "Example.txt",
--     deck2 = "ExampleAdvanced.txt", -- optional; reused for battles 3-4
--     reward1 = { card = "Card Name", gold = 100 },
--     reward2 = { card = "Card Name", gold = 100 },
--     reward3 = { card = "Card Name", gold = 100 },
--     reward4 = { card = "Card Name", gold = 100 },
--     ai = { personality = "balanced" },
--     dialogue = {
--         greeting = "Ready to duel?",
--         defeat = "You won this time.",
--         victory = "Try again when you are ready.",
--         complete = "We have nothing left to prove."
--     }
-- },

return {
    {
        id = "mira",
        name = "Mira",
        kind = "duelist",
        appearance = "mira",
        max_battles = 4,
        deck1 = "Zagaan.txt",
        deck2 = "Deathliger.txt",
        reward1 = { card = "Zagaan, Knight of Darkness", gold = 100 },
        reward2 = { card = "Zagaan, Knight of Darkness", gold = 100 },
        reward3 = { card = "Zagaan, Knight of Darkness", gold = 100 },
        reward4 = { card = "Zagaan, Knight of Darkness", gold = 100 },
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
        kind = "duelist",
        appearance = "marin",
        max_battles = 4,
        deck1 = "AquaSniper.txt",
        deck2 = "KingDepthcon.txt",
        reward1 = { card = "Aqua Sniper", gold = 100 },
        reward2 = { card = "Aqua Sniper", gold = 100 },
        reward3 = { card = "Aqua Sniper", gold = 100 },
        reward4 = { card = "Aqua Sniper", gold = 100 },
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
        kind = "duelist",
        appearance = "rook",
        max_battles = 4,
        deck1 = "RoaringGreathorn.txt",
        deck2 = "DeathbladeBeetle.txt",
        reward1 = { card = "Roaring Great-Horn", gold = 100 },
        reward2 = { card = "Roaring Great-Horn", gold = 100 },
        reward3 = { card = "Roaring Great-Horn", gold = 100 },
        reward4 = { card = "Roaring Great-Horn", gold = 100 },
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
        kind = "duelist",
        appearance = "aurelia",
        max_battles = 4,
        deck1 = "Hanusa.txt",
        deck2 = "Urth.txt",
        reward1 = { card = "Hanusa, Radiance Elemental", gold = 100 },
        reward2 = { card = "Hanusa, Radiance Elemental", gold = 100 },
        reward3 = { card = "Hanusa, Radiance Elemental", gold = 100 },
        reward4 = { card = "Hanusa, Radiance Elemental", gold = 100 },
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
        kind = "duelist",
        appearance = "flint",
        max_battles = 4,
        deck1 = "AstrocometDragon.txt",
        deck2 = "ScarletSkyterror.txt",
        reward1 = { card = "Astrocomet Dragon", gold = 100 },
        reward2 = { card = "Astrocomet Dragon", gold = 100 },
        reward3 = { card = "Astrocomet Dragon", gold = 100 },
        reward4 = { card = "Astrocomet Dragon", gold = 100 },
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
        kind = "duelist",
        appearance = "nyx",
        max_battles = 4,
        deck1 = "Deathliger.txt",
        deck2 = "Zagaan.txt",
        reward1 = { card = "Deathliger, Lion of Chaos", gold = 100 },
        reward2 = { card = "Deathliger, Lion of Chaos", gold = 100 },
        reward3 = { card = "Deathliger, Lion of Chaos", gold = 100 },
        reward4 = { card = "Deathliger, Lion of Chaos", gold = 100 },
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
        kind = "duelist",
        appearance = "tidal",
        max_battles = 4,
        deck1 = "KingDepthcon.txt",
        deck2 = "AquaSniper.txt",
        reward1 = { card = "King Depthcon", gold = 100 },
        reward2 = { card = "King Depthcon", gold = 100 },
        reward3 = { card = "King Depthcon", gold = 100 },
        reward4 = { card = "King Depthcon", gold = 100 },
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
        kind = "duelist",
        appearance = "briar",
        max_battles = 4,
        deck1 = "DeathbladeBeetle.txt",
        deck2 = "RoaringGreathorn.txt",
        reward1 = { card = "Deathblade Beetle", gold = 100 },
        reward2 = { card = "Deathblade Beetle", gold = 100 },
        reward3 = { card = "Deathblade Beetle", gold = 100 },
        reward4 = { card = "Deathblade Beetle", gold = 100 },
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
        kind = "shopkeeper",
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
        deck1 = "NPC/VeiledOne.txt",
        reward1 = { card = "Urth, Purifying Elemental", gold = 250 },
        ai = { personality = "adaptive" },
        dialogue = {
            greeting = "Every echo you restored belongs to the Curator. Hand them over.",
            defeat = "The veil is only a shell. The Curator has already measured what you restored.",
            victory = "Your echoes will be quieter in the Hollow Deck.",
            complete = "The Curator has already crossed the old road. Rowan still lives—but not for long."
        }
    }
}
