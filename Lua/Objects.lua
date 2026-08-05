-- Overworld object metadata.
-- Positions are owned by Lua/World.lua and maintained by the World Builder.
-- Supported kinds: signpost, deck_chest
-- Deck chests additionally require deck, deck_name, and opened_text. Deck paths
-- are searched beneath Decks/ automatically.

return {
	{
		id = "glasswater_tideglass_sign",
		name = "Tideglass Square Signpost",
		kind = "signpost",
		text = "Tideglass Square. Long Quay and the ferry lie west; the Ledger Ward and Tidal Arena lie east."
	},
	{
		id = "glasswater_quay_sign",
		name = "Long Quay Signpost",
		kind = "signpost",
		text = "Long Quay and the ferry are ahead. Tideglass Square is east along the main dock."
	},
	{
		id = "glasswater_ledger_sign",
		name = "Ledger Ward Signpost",
		kind = "signpost",
		text = "Ledger Ward. Continue east for the Tidal Arena, or west for Tideglass Square and Long Quay."
	},
	{
		id = "glasswater_watershed_sign",
		name = "Watershed Gate Signpost",
		kind = "signpost",
		text = "Watershed Gate. The road beyond leads to Watershed Crossroads, Rootmaze Commons, and Emberglen."
	},
	{
		id = "rootmaze_common_sign",
		name = "Greatroot Common Signpost",
		kind = "signpost",
		text = "Greatroot Common. Waterstep Market is northeast; Heartroot is west; Southroot Green is south."
	},
	{
		id = "rootmaze_market_sign",
		name = "Waterstep Market Signpost",
		kind = "signpost",
		text = "Waterstep Market. Greatroot Common lies southwest, and Northwater Gate lies east."
	},
	{
		id = "rootmaze_heartroot_sign",
		name = "Heartroot Signpost",
		kind = "signpost",
		text = "Heartroot. The Verdant Arena is south; Greatroot Common and Waterstep Market are east."
	},
	{
		id = "rootmaze_arena_sign",
		name = "Verdant Arena Signpost",
		kind = "signpost",
		text = "Verdant Arena. Heartroot is north, and Southroot Green is east along the meadow path."
	},
	{
		id = "rootmaze_southroot_sign",
		name = "Southroot Green Signpost",
		kind = "signpost",
		text = "Southroot Green. The Verdant Arena lies west; Greatroot Common lies north."
	},
	{
		id = "rootmaze_northwater_sign",
		name = "Northwater Gate Signpost",
		kind = "signpost",
		text = "Northwater Gate. Follow the road west into Rootmaze Commons or east toward Watershed Crossroads."
	},
	{
		id = "watershed_crossroads_sign",
		name = "Watershed Crossroads Signpost",
		kind = "signpost",
		text = "Watershed Crossroads. Glasswater lies northwest, Rootmaze southwest, and Emberglen east."
	},
	{
		id = "watershed_glasswater_sign",
		name = "Glasswater Route Signpost",
		kind = "signpost",
		text = "West to Glasswater Port. Return east for the toll shelter, Emberglen, and the Old Road."
	},
	{
		id = "watershed_rootmaze_sign",
		name = "Rootmaze Route Signpost",
		kind = "signpost",
		text = "West to Rootmaze Commons. Return east for the toll shelter, Emberglen, and the Old Road."
	},
	{
		id = "watershed_shelter_sign",
		name = "Toll Shelter Signpost",
		kind = "signpost",
		text = "Toll Shelter. Glasswater and Rootmaze branch west; Emberglen lies east."
	},
	{
		id = "watershed_emberglen_sign",
		name = "Emberglen Route Signpost",
		kind = "signpost",
		text = "East to Emberglen. West returns to Watershed Crossroads, Glasswater, and Rootmaze."
	},
	{
		id = "old_road_junction_sign",
		name = "Old Road Signpost",
		kind = "signpost",
		text = "The Old Road. Emberglen lies west; Cinderrail Foundry lies east."
	},
	{
		id = "old_road_camp_sign",
		name = "Wayfarer Camp Signpost",
		kind = "signpost",
		text = "Wayfarer Camp lies just off the road. Cinderrail is east and Emberglen is west."
	},
	{
		id = "old_road_cut_sign",
		name = "Abandoned Cut Signpost",
		kind = "signpost",
		text = "Abandoned Cut. This neglected trail loops back toward the Old Road; travelers proceed at their own risk."
	},
	{
		id = "old_road_wayfarer_chest",
		name = "Weathered Freight Chest",
		kind = "deck_chest",
		deck = "Treasure/Wayfarers Cache.txt",
		deck_name = "Wayfarer's Cache",
		text = "Beneath a ruined freight manifest is a complete Fire and Nature deck. The Wayfarer's Cache was added to your collection and deck list.",
		opened_text = "The freight chest is empty. Its broken manifest still bears the mark of an old Emberglen caravan."
	},
	{
		id = "blackstone_checkpoint_sign",
		name = "Blackstone Checkpoint Signpost",
		kind = "signpost",
		text = "Blackstone Road checkpoint. The relay gate remains sealed until Dragon Keep restores the Confluence route to Gloam Quarry."
	},
	{
		id = "cinderrail_station_sign",
		name = "Central Station Signpost",
		kind = "signpost",
		text = "Central Station. Forge Square lies southeast, and the Old Road leaves town to the west."
	},
	{
		id = "cinderrail_square_sign",
		name = "Forge Square Signpost",
		kind = "signpost",
		text = "Forge Square. Central Station is northwest; Foundry Hall and the Forge Arena are east."
	},
	{
		id = "cinderrail_foundry_sign",
		name = "Foundry Hall Signpost",
		kind = "signpost",
		text = "Foundry Hall. Forge Square lies west, and the Forge Arena is south."
	},
	{
		id = "cinderrail_arena_sign",
		name = "Forge Arena Signpost",
		kind = "signpost",
		text = "Forge Arena. Foundry Hall is north; Forge Square and Central Station lie west."
	}
}
