#pragma once

#include "WorldData.h"

#include <string>
#include <vector>

namespace CatalogMapStorage
{
	bool loadMap(const std::string& path, WorldMap& map, std::string& error);
	bool saveMap(const std::string& path, const WorldMap& map, std::string& error);
	bool loadMaps(const std::string& directory, std::vector<WorldMap>& maps,
		std::string& error);
	bool saveMaps(const std::string& directory, const std::vector<WorldMap>& maps,
		std::string& error);
}
