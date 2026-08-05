#pragma once

#include "WorldData.h"

#include <string>

namespace WorldStorage
{
	bool load(const std::string& path, WorldData& world, std::string& error);
	bool save(const std::string& path, const WorldData& world, std::string& error);
}
