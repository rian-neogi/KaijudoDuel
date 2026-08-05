#include "WorldStorage.h"

#include "CatalogMapStorage.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

namespace
{
	using boost::property_tree::ptree;

	std::string parentDirectory(const std::string& path)
	{
		size_t separator = path.find_last_of("/\\");
		return separator == std::string::npos ? "." : path.substr(0, separator);
	}

	bool safeRelativePath(const std::string& path)
	{
		return !path.empty() && path[0] != '/' && path[0] != '\\' &&
			path.find("..") == std::string::npos && path.find('\\') == std::string::npos;
	}

	bool safeMapId(const std::string& id)
	{
		if (id.empty()) return false;
		for (size_t index = 0; index < id.size(); ++index)
		{
			unsigned char character = (unsigned char)id[index];
			if (!((character >= 'a' && character <= 'z') ||
				(character >= 'A' && character <= 'Z') ||
				(character >= '0' && character <= '9') || character == '_' ||
				character == '-')) return false;
		}
		return true;
	}

	WorldPosition readPosition(const ptree& tree)
	{
		WorldPosition result;
		result.mapId = tree.get<std::string>("map", "");
		result.x = tree.get<int>("x", -1);
		result.y = tree.get<int>("y", -1);
		return result;
	}

	bool readPositions(const ptree& root, const std::string& path,
		std::map<std::string, WorldPosition>& positions, std::string& error)
	{
		positions.clear();
		try
		{
			const ptree& entries = root.get_child(path);
			for (ptree::const_iterator entry = entries.begin(); entry != entries.end(); ++entry)
			{
				std::string id = entry->second.get<std::string>("id", "");
				if (id.empty() || !positions.insert(std::make_pair(id,
					readPosition(entry->second))).second)
				{
					error = "world manifest has a duplicate or empty entity ID in '" + path + "'";
					return false;
				}
			}
		}
		catch (const std::exception& exception)
		{
			error = "world manifest is missing '" + path + "': " + exception.what();
			return false;
		}
		return true;
	}

	std::string jsonEscape(const std::string& value)
	{
		std::ostringstream escaped;
		for (size_t index = 0; index < value.size(); ++index)
		{
			unsigned char character = (unsigned char)value[index];
			if (character == '"') escaped << "\\\"";
			else if (character == '\\') escaped << "\\\\";
			else if (character == '\n') escaped << "\\n";
			else if (character == '\r') escaped << "\\r";
			else if (character == '\t') escaped << "\\t";
			else if (character < 0x20)
			{
				const char* digits = "0123456789abcdef";
				escaped << "\\u00" << digits[character >> 4] << digits[character & 15];
			}
			else escaped << value[index];
		}
		return escaped.str();
	}

	void writePosition(std::ostringstream& output, const WorldPosition& position)
	{
		output << "{ \"map\": \"" << jsonEscape(position.mapId) << "\", \"x\": "
			<< position.x << ", \"y\": " << position.y << " }";
	}

	void writePositions(std::ostringstream& output,
		const std::map<std::string, WorldPosition>& positions, int indentation)
	{
		size_t index = 0;
		for (std::map<std::string, WorldPosition>::const_iterator position =
			positions.begin(); position != positions.end(); ++position, ++index)
		{
			output << std::string(indentation, ' ') << "{ \"id\": \""
				<< jsonEscape(position->first) << "\", \"map\": \""
				<< jsonEscape(position->second.mapId) << "\", \"x\": "
				<< position->second.x << ", \"y\": " << position->second.y << " }"
				<< (index + 1 == positions.size() ? "\n" : ",\n");
		}
	}

	bool atomicWrite(const std::string& path, const std::string& contents,
		std::string& error)
	{
		const std::string temporary = path + ".tmp";
		std::ofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
		if (!output)
		{
			error = "could not create " + temporary;
			return false;
		}
		output << contents;
		output.close();
		if (!output)
		{
			error = "could not write " + temporary;
			std::remove(temporary.c_str());
			return false;
		}
		if (std::rename(temporary.c_str(), path.c_str()) != 0)
		{
			error = "could not replace " + path + ": " + std::strerror(errno);
			std::remove(temporary.c_str());
			return false;
		}
		return true;
	}
}

bool WorldStorage::load(const std::string& path, WorldData& world,
	std::string& error)
{
	error.clear();
	ptree root;
	try
	{
		boost::property_tree::read_json(path, root);
	}
	catch (const std::exception& exception)
	{
		error = "could not read world manifest '" + path + "': " + exception.what();
		return false;
	}
	if (root.get<std::string>("format", "") != "kaijudo-world" ||
		root.get<int>("version", 0) != 1)
	{
		error = "world manifest has an unsupported format or version";
		return false;
	}
	WorldData loaded;
	const std::string directory = parentDirectory(path);
	std::set<std::string> mapIds;
	try
	{
		const ptree& maps = root.get_child("maps");
		for (ptree::const_iterator mapPath = maps.begin(); mapPath != maps.end(); ++mapPath)
		{
			std::string relativePath = mapPath->second.get_value<std::string>();
			if (!safeRelativePath(relativePath))
			{
				error = "world manifest contains an unsafe map path";
				return false;
			}
			WorldMap map;
			if (!CatalogMapStorage::loadMap(directory + "/" + relativePath, map, error))
				return false;
			if (!mapIds.insert(map.id).second)
			{
				error = "world manifest contains duplicate map ID '" + map.id + "'";
				return false;
			}
			loaded.maps.push_back(map);
		}
		const ptree& regions = root.get_child("regions");
		for (ptree::const_iterator entry = regions.begin(); entry != regions.end(); ++entry)
		{
			WorldRegion region;
			region.id = entry->second.get<std::string>("id", "");
			region.name = entry->second.get<std::string>("name", "");
			region.mapId = entry->second.get<std::string>("map", "");
			std::string kind = entry->second.get<std::string>("kind", "");
			if (kind != "town" && kind != "connector")
			{
				error = "world manifest contains an invalid region kind";
				return false;
			}
			region.connector = kind == "connector";
			region.x = entry->second.get<int>("x", -1);
			region.y = entry->second.get<int>("y", -1);
			region.width = entry->second.get<int>("width", 0);
			region.height = entry->second.get<int>("height", 0);
			loaded.regions.push_back(region);
		}
		loaded.start = readPosition(root.get_child("start"));
		const ptree& portals = root.get_child("portals");
		for (ptree::const_iterator entry = portals.begin(); entry != portals.end(); ++entry)
		{
			WorldPosition from = readPosition(entry->second.get_child("from"));
			WorldPosition to = readPosition(entry->second.get_child("to"));
			WorldPortal portal;
			portal.fromMap = from.mapId;
			portal.fromX = from.x;
			portal.fromY = from.y;
			portal.toMap = to.mapId;
			portal.toX = to.x;
			portal.toY = to.y;
			loaded.portals.push_back(portal);
		}
	}
	catch (const std::exception& exception)
	{
		error = "world manifest is incomplete: " + std::string(exception.what());
		return false;
	}
	if (!readPositions(root, "entities.npcs", loaded.npcPositions, error) ||
		!readPositions(root, "entities.objects", loaded.objectPositions, error) ||
		!readPositions(root, "entities.shards", loaded.shardPositions, error) ||
		!loaded.validateStructure(error)) return false;
	world.swap(loaded);
	return true;
}

bool WorldStorage::save(const std::string& path, const WorldData& world,
	std::string& error)
{
	if (!world.validateStructure(error)) return false;
	for (size_t index = 0; index < world.maps.size(); ++index)
		if (!safeMapId(world.maps[index].id))
		{
			error = "map ID '" + world.maps[index].id +
				"' cannot be used as a native map filename";
			return false;
		}
	const std::string directory = parentDirectory(path);
	if (!CatalogMapStorage::saveMaps(directory + "/Maps", world.maps, error)) return false;
	std::ostringstream output;
	output << "{\n  \"format\": \"kaijudo-world\",\n  \"version\": 1,\n  \"maps\": [\n";
	for (size_t index = 0; index < world.maps.size(); ++index)
		output << "    \"Maps/" << jsonEscape(world.maps[index].id) << ".json\""
			<< (index + 1 == world.maps.size() ? "\n" : ",\n");
	output << "  ],\n  \"regions\": [\n";
	for (size_t index = 0; index < world.regions.size(); ++index)
	{
		const WorldRegion& region = world.regions[index];
		output << "    { \"id\": \"" << jsonEscape(region.id) << "\", \"name\": \""
			<< jsonEscape(region.name) << "\", \"map\": \"" << jsonEscape(region.mapId)
			<< "\", \"kind\": \"" << (region.connector ? "connector" : "town")
			<< "\", \"x\": " << region.x << ", \"y\": " << region.y
			<< ", \"width\": " << region.width << ", \"height\": " << region.height
			<< " }" << (index + 1 == world.regions.size() ? "\n" : ",\n");
	}
	output << "  ],\n  \"start\": ";
	writePosition(output, world.start);
	output << ",\n  \"portals\": [\n";
	for (size_t index = 0; index < world.portals.size(); ++index)
	{
		const WorldPortal& portal = world.portals[index];
		output << "    { \"from\": ";
		writePosition(output, { portal.fromMap, portal.fromX, portal.fromY });
		output << ", \"to\": ";
		writePosition(output, { portal.toMap, portal.toX, portal.toY });
		output << " }" << (index + 1 == world.portals.size() ? "\n" : ",\n");
	}
	output << "  ],\n  \"entities\": {\n    \"npcs\": [\n";
	writePositions(output, world.npcPositions, 6);
	output << "    ],\n    \"objects\": [\n";
	writePositions(output, world.objectPositions, 6);
	output << "    ],\n    \"shards\": [\n";
	writePositions(output, world.shardPositions, 6);
	output << "    ]\n  }\n}\n";
	return atomicWrite(path, output.str(), error);
}
