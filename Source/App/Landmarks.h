#pragma once

#include <cstddef>
#include <string>

namespace Landmarks
{
	struct Definition
	{
		const char* id;
		const char* name;
		const char* regionId;
		int localX;
		int localY;
		int discoveryRadius;
		int goldReward;
	};

	// Positions are relative to their named native world regions, so expanding or
	// moving a region does not invalidate exploration progress.
	constexpr Definition DEFINITIONS[] = {
		{ "northwater_pools", "Northwater Pools", "watershed_crossroads", 24, 11, 3, 25 },
		{ "toll_shelter", "Old Toll Shelter", "watershed_crossroads", 61, 25, 3, 25 },
		{ "drowned_foundations", "Drowned Foundations", "watershed_crossroads", 94, 55, 5, 30 },
		{ "abandoned_cut", "Abandoned Freight Cut", "old_road", 14, 2, 3, 30 },
		{ "rook_checkpoint", "Rook's Checkpoint", "old_road", 24, 21, 3, 25 },
		{ "wayfarer_camp", "Wayfarer Camp", "old_road", 69, 42, 3, 30 }
	};

	constexpr size_t COUNT = sizeof(DEFINITIONS) / sizeof(DEFINITIONS[0]);

	inline const Definition* find(const std::string& id)
	{
		for (size_t i = 0; i < COUNT; ++i)
			if (DEFINITIONS[i].id == id) return &DEFINITIONS[i];
		return NULL;
	}
}
