-- World Builder data. This file is entirely maintained by the World Builder.
-- Tile legend: . grass, = path, ~ water, H house, T tree, # forest,
-- W wooden wall, D door, F wooden floor, C counter, B bonfire,
-- A feast table, S dueling sand, M marble, E workshop tools.
return {
	maps = {
		{
			id = "emberglen",
			name = "Emberglen",
			indoor = false,
			tiles = {
				"####################################",
				"#TTT..........MMMMMMM..........TTTT#",
				"#TTT..........MMMMMMM..........TTTT#",
				"#.WWWWWWW.....MMMMMMM.~~~~~TT..TTTT#",
				"#.WWWWWWW..TT.MMMDMMM.~~~~~T.......#",
				"#.WWWWWWW........=....~~~~~......T.#",
				"#.WWWDWWWTT......=.....~~~.........#",
				"#..TT===T=========..........WWWWW..#",
				"#....=...........=..........WWWWW..#",
				"#.SSSSSSSSS.===========.....WWWWW..#",
				"#.SSSSSSSSS.==A=====A==.....WWDWW..#",
				"#.SSSSSSSSS.===================....#",
				"#.SSSSSSSS=======B=================#",
				"#.SSSSSSSSS.===========............#",
				"#.SSSSSSSSS.==A=====A=============.#",
				"#.SSSSSSSSS.===========............#",
				"#.SSSSSSSSS......=...............TT#",
				"#........TT......=.................#",
				"#................=....TTT..........#",
				"#................=.......WWWWWWW####",
				"#########........=.......WWWWWWW####",
				"#########........=.......WWWWWWW####",
				"#########...TT...=.......WWWDWWW####",
				"#########........============...####",
				"#########.......................####",
				"####################################"
			}
		},
		{
			id = "mercers_house",
			name = "Mercer's House",
			indoor = true,
			tiles = {
				"WWWWWWWWWW",
				"WEEFFFFFEW",
				"WFFFFFFFFW",
				"WFFCCCCFFW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WWWWWDWWWW"
			}
		},
		{
			id = "rook_mira_home",
			name = "Rook and Mira's Home",
			indoor = true,
			tiles = {
				"WWWWWWWWWW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WFFAAFFFFW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WFFFFFFFFW",
				"WWWWWDWWWW"
			}
		}
	},
	start = { map = "emberglen", x = 17, y = 15 },
	portals = {
		{ from = { map = "emberglen", x = 30, y = 10 },
			to = { map = "mercers_house", x = 5, y = 6 } },
		{ from = { map = "mercers_house", x = 5, y = 7 },
			to = { map = "emberglen", x = 30, y = 11 } },
		{ from = { map = "emberglen", x = 5, y = 6 },
			to = { map = "rook_mira_home", x = 5, y = 6 } },
		{ from = { map = "rook_mira_home", x = 5, y = 7 },
			to = { map = "emberglen", x = 5, y = 7 } }
	},
	npcs = {
		["mira"] = { map = "rook_mira_home", x = 5, y = 2 },
		["marin"] = { map = "emberglen", x = 4, y = 14 },
		["rook"] = { map = "emberglen", x = 31, y = 14 },
		["aurelia"] = { map = "emberglen", x = 17, y = 5 },
		["flint"] = { map = "emberglen", x = 4, y = 10 },
		["nyx"] = { map = "emberglen", x = 28, y = 23 },
		["tidal"] = { map = "emberglen", x = 8, y = 14 },
		["briar"] = { map = "emberglen", x = 8, y = 10 },
		["mercer"] = { map = "mercers_house", x = 5, y = 2 },
		["veiled_one"] = { map = "emberglen", x = 17, y = 10 },
	},
	shards = {
		["living_dead"] = { map = "emberglen", x = 4, y = 1 },
		["ghost"] = { map = "emberglen", x = 30, y = 23 },
		["chimera"] = { map = "emberglen", x = 11, y = 2 },
		["parasite_worm"] = { map = "emberglen", x = 25, y = 2 },
		["human"] = { map = "emberglen", x = 10, y = 5 },
		["armoroid"] = { map = "emberglen", x = 12, y = 7 },
		["rock_beast"] = { map = "emberglen", x = 23, y = 7 },
		["machine_eater"] = { map = "emberglen", x = 34, y = 6 },
		["dragonoid"] = { map = "emberglen", x = 24, y = 10 },
		["fire"] = { map = "emberglen", x = 33, y = 10 },
		["giant_insect"] = { map = "emberglen", x = 1, y = 18 },
		["colony_beetle"] = { map = "emberglen", x = 6, y = 18 },
		["horned_beast"] = { map = "emberglen", x = 12, y = 19 },
		["beast_folk"] = { map = "emberglen", x = 20, y = 18 },
		["tree_folk"] = { map = "emberglen", x = 34, y = 18 },
		["fish"] = { map = "emberglen", x = 27, y = 6 },
		["cyber_lord"] = { map = "emberglen", x = 26, y = 12 },
		["cyber_virus"] = { map = "emberglen", x = 32, y = 12 },
		["liquid_people"] = { map = "emberglen", x = 13, y = 16 },
	}
}
