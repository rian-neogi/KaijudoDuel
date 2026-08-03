-- World Builder data. This file is entirely maintained by the World Builder.
-- Tile legend: . grass, = path, ~ water, H house, T tree, # forest,
-- W wooden wall, D door, F wooden floor, C counter.
return {
	maps = {
		{
			id = "emberglen",
			name = "Emberglen",
			indoor = false,
			tiles = {
				"####################",
				"#......~~~.........#",
				"#..HH..~~~..TTTT...#",
				"#..HH.......T..T...#",
				"#......====........#",
				"#..####=..=..~~~...#",
				"#......=..=..~~~...#",
				"#......====........#",
				"#...TT......WWW....#",
				"#...TT......WDW....#",
				"#..................#",
				"####################"
			}
		},
		{
			id = "mercers_house",
			name = "Mercer's House",
			indoor = true,
			tiles = {
				"WWWWWWWWWW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WFFCCCCFFW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WWWWWDWWWW"
			}
		}
	},
	start = { map = "emberglen", x = 2, y = 10 },
	portals = {
		{ from = { map = "emberglen", x = 13, y = 9 },
			to = { map = "mercers_house", x = 5, y = 6 } },
		{ from = { map = "mercers_house", x = 5, y = 7 },
			to = { map = "emberglen", x = 13, y = 10 } }
	},
	npcs = {
		["mira"] = { map = "emberglen", x = 10, y = 7 },
		["marin"] = { map = "emberglen", x = 16, y = 4 },
		["rook"] = { map = "emberglen", x = 7, y = 4 },
		["aurelia"] = { map = "emberglen", x = 12, y = 1 },
		["flint"] = { map = "emberglen", x = 5, y = 4 },
		["nyx"] = { map = "emberglen", x = 17, y = 7 },
		["tidal"] = { map = "emberglen", x = 10, y = 10 },
		["briar"] = { map = "emberglen", x = 2, y = 6 },
		["mercer"] = { map = "mercers_house", x = 5, y = 2 },
		["veiled_one"] = { map = "emberglen", x = 10, y = 4 },
	},
	shards = {
		["living_dead"] = { map = "emberglen", x = 1, y = 1 },
		["ghost"] = { map = "emberglen", x = 5, y = 1 },
		["chimera"] = { map = "emberglen", x = 10, y = 1 },
		["parasite_worm"] = { map = "emberglen", x = 15, y = 1 },
		["human"] = { map = "emberglen", x = 1, y = 3 },
		["armoroid"] = { map = "emberglen", x = 6, y = 3 },
		["rock_beast"] = { map = "emberglen", x = 11, y = 3 },
		["machine_eater"] = { map = "emberglen", x = 17, y = 3 },
		["dragonoid"] = { map = "emberglen", x = 3, y = 4 },
		["fire"] = { map = "emberglen", x = 13, y = 4 },
		["giant_insect"] = { map = "emberglen", x = 5, y = 6 },
		["colony_beetle"] = { map = "emberglen", x = 9, y = 6 },
		["horned_beast"] = { map = "emberglen", x = 17, y = 6 },
		["beast_folk"] = { map = "emberglen", x = 3, y = 7 },
		["tree_folk"] = { map = "emberglen", x = 12, y = 7 },
		["fish"] = { map = "emberglen", x = 6, y = 8 },
		["cyber_lord"] = { map = "emberglen", x = 10, y = 8 },
		["cyber_virus"] = { map = "emberglen", x = 16, y = 8 },
		["liquid_people"] = { map = "emberglen", x = 7, y = 10 },
	}
}
