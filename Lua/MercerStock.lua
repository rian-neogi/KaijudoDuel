-- Shard positions are owned by Lua/World.lua and maintained by the World Builder.
return {
	prices = { 100, 500, 1500, 5000, 25000 },

	initial_stock = {
		"Artisan Picora", "Brawler Zyler", "Super Explosive Volcanodon",
		"Armored Walker Urherion", "Draglide", "Tornado Flame",
		"Wandering Braineater", "Masked Horror, Shadow of Scorn", "Swamp Worm",
		"Dark Clown", "Gigargon", "Creeping Plague",
		"Hunter Fish", "King Coral", "Aqua Vehicle", "Tropico", "Marine Flower",
		"Crystal Memory", "Miele, Vizier of Lightning", "Chilias, the Oracle",
		"Dia Nork, Moonlight Guardian", "Lah, Purification Enforcer",
		"Senatine Jade Tree", "Logic Sphere", "Poisonous Mushroom", "Fear Fang",
		"Forest Hornet", "Storm Shell", "Stampeding Longhorn", "Aura Blast"
	},

	shards = {
		{
			id = "living_dead", name = "Living Dead Shard",
			stock = { "Wandering Braineater", "Writhing Bone Ghoul",
				"Skeleton Soldier, the Defiled", "Bone Assassin, the Ripper", "Dark Reversal" }
		},
		{
			id = "ghost", name = "Ghost Shard",
			stock = { "Night Master, Shadow of Decay", "Dark Raven, Shadow of Grief",
				"Black Feather, Shadow of Rage", "Gray Balloon, Shadow of Greed", "Ghost Touch" }
		},
		{
			id = "chimera", name = "Chimera Shard",
			stock = { "Gigagiele", "Gigaberos", "Gigastand", "Terror Pit" }
		},
		{
			id = "parasite_worm", name = "Parasite Worm Shard",
			stock = { "Stinger Worm", "Poison Worm", "Horrid Worm", "Death Smoke" }
		},
		{
			id = "human", name = "Human Shard",
			stock = { "Immortal Baron, Vorg", "Mini Titan Gett", "Fatal Attacker Horvath" }
		},
		{
			id = "armoroid", name = "Armoroid Shard",
			stock = { "Rothus, the Traveler" }
		},
		{
			id = "rock_beast", name = "Rock Beast Shard",
			stock = { "Meteosaur", "Cratersaur", "Galsaur" }
		},
		{
			id = "machine_eater", name = "Machine Eater Shard",
			stock = { "Nomad Hero Gigio", "Magma Gazer", "Crimson Hammer" }
		},
		{
			id = "dragonoid", name = "Dragonoid Shard",
			stock = { "Onslaughter Triceps", "Deadly Fighter Braid Claw",
				"Fire Sweeper Burning Hellion", "Explosive Fighter Ucarn", "Burning Power" }
		},
		{
			id = "fire", name = "Fire Shard", stock = {}
		},
		{
			id = "giant_insect", name = "Giant Insect Shard",
			stock = { "Red-Eye Scorpion" }
		},
		{
			id = "colony_beetle", name = "Colony Beetle Shard",
			stock = { "Tower Shell" }
		},
		{
			id = "horned_beast", name = "Horned Beast Shard",
			stock = { "Rumbling Terahorn" }
		},
		{
			id = "beast_folk", name = "Beast Folk Shard",
			stock = { "Golden Wing Striker", "Mighty Shouter", "Burning Mane", "Ultimate Force" }
		},
		{
			id = "tree_folk", name = "Tree Folk Shard",
			stock = { "Coiling Vines", "Thorny Mandra", "Poisonous Dahlia", "Essence Elf" }
		},
		{
			id = "fish", name = "Fish Shard",
			stock = { "Saucer-Head Shark", "Seamine", "Illusionary Merfolk", "Phantom Fish", "Scissor Eye" }
		},
		{
			id = "cyber_lord", name = "Cyber Lord Shard",
			stock = { "Corile", "Hypersquid Walter", "Teleportation" }
		},
		{
			id = "cyber_virus", name = "Cyber Virus Shard",
			stock = { "Candy Drop", "Faerie Child", "Spiral Gate" }
		},
		{
			id = "liquid_people", name = "Liquid People Shard",
			stock = { "Aqua Shooter", "Aqua Bouncer", "Aqua Surfer", "Recon Operation" }
		}
	}
}
