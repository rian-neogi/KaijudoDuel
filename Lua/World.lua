-- World Builder map data. Each character is one 48x48 overworld tile.
-- This file is entirely maintained by the World Builder.
-- . grass, = path, ~ water, H house, T tree, # dense forest
return {
	map = {
		"####################",
		"#......~~~.........#",
		"#..HH..~~~..TTTT...#",
		"#..HH.......T..T...#",
		"#......====........#",
		"#..####=..=..~~~...#",
		"#......=..=..~~~...#",
		"#......====........#",
		"#...TT......HH.....#",
		"#...TT......HH.....#",
		"#..................#",
		"####################"
	},
	npcs = {
		["mira"] = { x = 10, y = 7 },
		["marin"] = { x = 16, y = 4 },
		["rook"] = { x = 7, y = 4 },
		["aurelia"] = { x = 12, y = 1 },
		["flint"] = { x = 5, y = 4 },
		["nyx"] = { x = 17, y = 7 },
		["tidal"] = { x = 10, y = 10 },
		["briar"] = { x = 2, y = 6 },
		["mercer"] = { x = 18, y = 10 },
		["veiled_one"] = { x = 10, y = 4 },
	},
	shards = {
		["living_dead"] = { x = 1, y = 1 },
		["ghost"] = { x = 5, y = 1 },
		["chimera"] = { x = 10, y = 1 },
		["parasite_worm"] = { x = 15, y = 1 },
		["human"] = { x = 1, y = 3 },
		["armoroid"] = { x = 6, y = 3 },
		["rock_beast"] = { x = 11, y = 3 },
		["machine_eater"] = { x = 17, y = 3 },
		["dragonoid"] = { x = 3, y = 4 },
		["fire"] = { x = 13, y = 4 },
		["giant_insect"] = { x = 5, y = 6 },
		["colony_beetle"] = { x = 9, y = 6 },
		["horned_beast"] = { x = 17, y = 6 },
		["beast_folk"] = { x = 3, y = 7 },
		["tree_folk"] = { x = 12, y = 7 },
		["fish"] = { x = 6, y = 8 },
		["cyber_lord"] = { x = 10, y = 8 },
		["cyber_virus"] = { x = 16, y = 8 },
		["liquid_people"] = { x = 7, y = 10 },
	}
}
