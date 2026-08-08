#include "CatalogMapStorage.h"

#include "RtpTilesetRenderer.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace
{
	using boost::property_tree::ptree;

	const char* familyName(RtpTilesetFamily family)
	{
		if (family == RtpTilesetFamily::Dungeon) return "Dungeon";
		if (family == RtpTilesetFamily::Inside) return "Inside";
		if (family == RtpTilesetFamily::World) return "World";
		return "Outside";
	}

	bool parseFamily(const std::string& name, RtpTilesetFamily& family)
	{
		if (name == "Dungeon") family = RtpTilesetFamily::Dungeon;
		else if (name == "Inside") family = RtpTilesetFamily::Inside;
		else if (name == "Outside") family = RtpTilesetFamily::Outside;
		else if (name == "World") family = RtpTilesetFamily::World;
		else return false;
		return true;
	}

	const char* sheetName(RtpTileSheet sheet)
	{
		if (sheet == RtpTileSheet::A1) return "A1";
		if (sheet == RtpTileSheet::A2) return "A2";
		if (sheet == RtpTileSheet::A3) return "A3";
		if (sheet == RtpTileSheet::A4) return "A4";
		if (sheet == RtpTileSheet::A5) return "A5";
		if (sheet == RtpTileSheet::B) return "B";
		return "C";
	}

	bool parseSheet(const std::string& name, RtpTileSheet& sheet)
	{
		if (name == "A1") sheet = RtpTileSheet::A1;
		else if (name == "A2") sheet = RtpTileSheet::A2;
		else if (name == "A3") sheet = RtpTileSheet::A3;
		else if (name == "A4") sheet = RtpTileSheet::A4;
		else if (name == "A5") sheet = RtpTileSheet::A5;
		else if (name == "B") sheet = RtpTileSheet::B;
		else if (name == "C") sheet = RtpTileSheet::C;
		else return false;
		return true;
	}

	const char* layerName(RtpRenderLayer layer)
	{
		if (layer == RtpRenderLayer::Ground) return "ground";
		if (layer == RtpRenderLayer::Decoration) return "decoration";
		return "foreground";
	}

	bool parseLayer(const std::string& name, RtpRenderLayer& layer)
	{
		if (name == "ground") layer = RtpRenderLayer::Ground;
		else if (name == "decoration") layer = RtpRenderLayer::Decoration;
		else if (name == "foreground") layer = RtpRenderLayer::Foreground;
		else return false;
		return true;
	}

	void normalizeLargeTreeSpacing(WorldMap& map)
	{
		std::set<std::pair<int, int> > acceptedAnchors;
		for (std::map<std::tuple<int, int, int>, RtpTileReference>::iterator tile =
			map.tileLayers.begin(); tile != map.tileLayers.end();)
		{
			std::map<std::tuple<int, int, int>, RtpTileReference>::iterator current =
				tile++;
			if (std::get<2>(current->first) != (int)RtpRenderLayer::Decoration)
				continue;
			int footprintWidth = 0;
			int footprintHeight = 0;
			if (!RtpTilesetRenderer::treeAutotileFootprint(current->second,
				footprintWidth, footprintHeight) || footprintWidth != 2) continue;
			int y = std::get<0>(current->first);
			int x = std::get<1>(current->first);
			bool overlaps = x < footprintWidth - 1 || y < footprintHeight - 1;
			for (int horizontalSide = -1; horizontalSide <= 1;
				horizontalSide += 2)
				if (acceptedAnchors.count(std::make_pair(x + horizontalSide, y)) != 0)
				{
					overlaps = true;
					break;
				}
			if (overlaps) map.tileLayers.erase(current);
			else acceptedAnchors.insert(std::make_pair(x, y));
		}
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

	typedef std::tuple<int, int, int, int, int, int, int> PaletteKey;

	PaletteKey paletteKey(const RtpTileReference& tile)
	{
		return std::make_tuple((int)tile.family, (int)tile.sheet, tile.index,
			(int)tile.layer, (int)tile.red, (int)tile.green, (int)tile.blue);
	}

	std::vector<std::pair<int, int> > runs(const std::vector<int>& values)
	{
		std::vector<std::pair<int, int> > result;
		if (values.empty()) return result;
		int value = values[0];
		int count = 1;
		for (size_t index = 1; index < values.size(); ++index)
		{
			if (values[index] == value) ++count;
			else
			{
				result.push_back(std::make_pair(value, count));
				value = values[index];
				count = 1;
			}
		}
		result.push_back(std::make_pair(value, count));
		return result;
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

bool CatalogMapStorage::loadMap(const std::string& path, WorldMap& map,
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
		error = "could not read catalog map '" + path + "': " + exception.what();
		return false;
	}
	if (root.get<std::string>("format", "") != "kaijudo-catalog-map" ||
		root.get<int>("version", 0) != 1)
	{
		error = "catalog map '" + path + "' has an unsupported format or version";
		return false;
	}
	WorldMap loaded;
	loaded.id = root.get<std::string>("id", "");
	loaded.name = root.get<std::string>("name", "");
	loaded.indoor = root.get<bool>("indoor", false);
	loaded.columns = root.get<int>("width", 0);
	loaded.rows = root.get<int>("height", 0);
	loaded.catalogOnly = true;
	if (loaded.id.empty() || loaded.name.empty() || loaded.columns <= 0 ||
		loaded.rows <= 0 || loaded.columns > 1024 || loaded.rows > 1024)
	{
		error = "catalog map '" + path + "' has invalid identity or dimensions";
		return false;
	}
	loaded.tiles.assign(loaded.rows, std::string(loaded.columns, '.'));
	std::vector<RtpTileReference> palette;
	palette.push_back(RtpTileReference(RtpTilesetFamily::Outside,
		RtpTileSheet::A5, 0));
	try
	{
		const ptree& paletteTree = root.get_child("palette");
		int paletteIndex = 0;
		for (ptree::const_iterator entry = paletteTree.begin();
			entry != paletteTree.end(); ++entry, ++paletteIndex)
		{
			if (paletteIndex == 0) continue;
			RtpTilesetFamily family;
			RtpTileSheet sheet;
			RtpRenderLayer layer;
			const ptree& value = entry->second;
			std::string familyText = value.get<std::string>("tileset", "");
			std::string sheetText = value.get<std::string>("sheet", "");
			std::string layerText = value.get<std::string>("layer", "");
			int index = value.get<int>("index", -1);
			std::vector<int> tint;
			const ptree& tintTree = value.get_child("tint");
			for (ptree::const_iterator component = tintTree.begin();
				component != tintTree.end(); ++component)
				tint.push_back(component->second.get_value<int>());
			const RtpSheetDescriptor* descriptor = NULL;
			if (parseFamily(familyText, family) && parseSheet(sheetText, sheet))
				descriptor = RtpTilesetRenderer::descriptor(family, sheet);
			if (descriptor == NULL || !parseLayer(layerText, layer) || index < 0 ||
				index >= descriptor->tileCount || tint.size() != 3 || tint[0] < 0 ||
				tint[0] > 255 || tint[1] < 0 || tint[1] > 255 || tint[2] < 0 ||
				tint[2] > 255)
			{
				error = "catalog map '" + path + "' has an invalid palette entry";
				return false;
			}
			index = RtpTilesetRenderer::canonicalTileIndex(family, sheet, index);
			palette.push_back(RtpTileReference(family, sheet, index, layer,
				(Uint8)tint[0], (Uint8)tint[1], (Uint8)tint[2]));
		}
		const RtpRenderLayer layers[] = { RtpRenderLayer::Ground,
			RtpRenderLayer::Decoration, RtpRenderLayer::Foreground };
		const int cellCount = loaded.columns * loaded.rows;
		for (int layerIndex = 0; layerIndex < 3; ++layerIndex)
		{
			RtpRenderLayer layer = layers[layerIndex];
			const ptree& layerTree = root.get_child(
				std::string("layers.") + layerName(layer));
			if (layerTree.get<std::string>("encoding", "") != "rle-row-major")
			{
				error = "catalog map '" + path + "' has unsupported layer encoding";
				return false;
			}
			int cell = 0;
			for (ptree::const_iterator run = layerTree.get_child("data").begin();
				run != layerTree.get_child("data").end(); ++run)
			{
				std::vector<int> pair;
				for (ptree::const_iterator value = run->second.begin();
					value != run->second.end(); ++value)
					pair.push_back(value->second.get_value<int>());
				if (pair.size() != 2 || pair[0] < 0 || pair[0] >= (int)palette.size() ||
					pair[1] <= 0 || cell + pair[1] > cellCount ||
					(pair[0] != 0 && palette[pair[0]].layer != layer))
				{
					error = "catalog map '" + path + "' has invalid RLE data";
					return false;
				}
				if (pair[0] != 0)
					for (int offset = 0; offset < pair[1]; ++offset)
					{
						int position = cell + offset;
						loaded.tileLayers.insert(std::make_pair(
							std::make_tuple(position / loaded.columns,
								position % loaded.columns, (int)layer), palette[pair[0]]));
					}
				cell += pair[1];
			}
			if (cell != cellCount)
			{
				error = "catalog map '" + path + "' layer has the wrong cell count";
				return false;
			}
		}
		normalizeLargeTreeSpacing(loaded);
		boost::optional<ptree&> tags = root.get_child_optional("tags");
		if (tags)
			for (ptree::const_iterator tag = tags->begin(); tag != tags->end(); ++tag)
			{
				int x = tag->second.get<int>("x", -1);
				int y = tag->second.get<int>("y", -1);
				std::string value = tag->second.get<std::string>("value", "");
				if (!loaded.contains(x, y) || value.empty() ||
					!loaded.tags.insert(std::make_pair(std::make_pair(x, y), value)).second)
				{
					error = "catalog map '" + path + "' has an invalid gameplay tag";
					return false;
				}
			}
	}
	catch (const std::exception& exception)
	{
		error = "catalog map '" + path + "' is incomplete: " + exception.what();
		return false;
	}
	map.swap(loaded);
	return true;
}

bool CatalogMapStorage::saveMap(const std::string& path, const WorldMap& map,
	std::string& error)
{
	if (map.id.empty() || map.name.empty() || map.width() <= 0 || map.height() <= 0)
	{
		error = "cannot save an invalid catalog map";
		return false;
	}
	std::vector<RtpTileReference> palette;
	std::map<PaletteKey, int> paletteIndices;
	std::vector<int> layerValues[3];
	for (int layer = 0; layer < 3; ++layer)
		layerValues[layer].assign(map.width() * map.height(), 0);
	for (std::map<std::tuple<int, int, int>, RtpTileReference>::const_iterator tile =
		map.tileLayers.begin(); tile != map.tileLayers.end(); ++tile)
	{
		const RtpTileReference& reference = tile->second;
		PaletteKey key = paletteKey(reference);
		std::map<PaletteKey, int>::const_iterator found = paletteIndices.find(key);
		int paletteIndex = 0;
		if (found == paletteIndices.end())
		{
			paletteIndex = (int)palette.size() + 1;
			paletteIndices.insert(std::make_pair(key, paletteIndex));
			palette.push_back(reference);
		}
		else paletteIndex = found->second;
		int y = std::get<0>(tile->first);
		int x = std::get<1>(tile->first);
		int layer = std::get<2>(tile->first);
		if (!map.contains(x, y) || layer < 0 || layer >= 3 ||
			layer != (int)reference.layer)
		{
			error = "map '" + map.id + "' has an invalid tile layer";
			return false;
		}
		layerValues[layer][y * map.width() + x] = paletteIndex;
	}

	std::ostringstream output;
	output << "{\n  \"format\": \"kaijudo-catalog-map\",\n  \"version\": 1,\n"
		<< "  \"id\": \"" << jsonEscape(map.id) << "\",\n"
		<< "  \"name\": \"" << jsonEscape(map.name) << "\",\n"
		<< "  \"indoor\": " << (map.indoor ? "true" : "false") << ",\n"
		<< "  \"width\": " << map.width() << ",\n"
		<< "  \"height\": " << map.height() << ",\n  \"palette\": [\n    null";
	for (size_t index = 0; index < palette.size(); ++index)
	{
		const RtpTileReference& tile = palette[index];
		output << ",\n    { \"tileset\": \"" << familyName(tile.family)
			<< "\", \"sheet\": \"" << sheetName(tile.sheet)
			<< "\", \"index\": " << tile.index << ", \"layer\": \""
			<< layerName(tile.layer) << "\", \"tint\": [" << (int)tile.red
			<< ", " << (int)tile.green << ", " << (int)tile.blue << "] }";
	}
	output << "\n  ],\n  \"layers\": {\n";
	for (int layer = 0; layer < 3; ++layer)
	{
		RtpRenderLayer layerType = (RtpRenderLayer)layer;
		std::vector<std::pair<int, int> > encoded = runs(layerValues[layer]);
		output << "    \"" << layerName(layerType)
			<< "\": { \"encoding\": \"rle-row-major\", \"data\": [";
		for (size_t run = 0; run < encoded.size(); ++run)
			output << (run == 0 ? "" : ", ") << "[" << encoded[run].first
				<< ", " << encoded[run].second << "]";
		output << "] }" << (layer == 2 ? "\n" : ",\n");
	}
	output << "  },\n  \"source\": { \"kind\": \"native-catalog-map\" }";
	if (!map.tags.empty())
	{
		output << ",\n  \"tags\": [\n";
		size_t index = 0;
		for (std::map<std::pair<int, int>, std::string>::const_iterator tag =
			map.tags.begin(); tag != map.tags.end(); ++tag, ++index)
			output << "    { \"x\": " << tag->first.first << ", \"y\": "
				<< tag->first.second << ", \"value\": \"" << jsonEscape(tag->second)
				<< "\" }" << (index + 1 == map.tags.size() ? "\n" : ",\n");
		output << "  ]";
	}
	output << "\n}\n";
	return atomicWrite(path, output.str(), error);
}

bool CatalogMapStorage::loadMaps(const std::string& directory,
	std::vector<WorldMap>& maps, std::string& error)
{
	std::vector<WorldMap> loadedMaps;
	for (size_t index = 0; index < maps.size(); ++index)
	{
		WorldMap loaded;
		if (!loadMap(directory + "/" + maps[index].id + ".json", loaded, error))
			return false;
		if (loaded.id != maps[index].id)
		{
			error = "catalog map filename and ID do not match for '" + maps[index].id + "'";
			return false;
		}
		// The legacy rows are retained only as dimensions for old editor helpers.
		// Catalog-only rendering and collision never inspect their glyph values.
		if (loaded.width() == maps[index].width() && loaded.height() == maps[index].height())
			loaded.tiles = maps[index].tiles;
		else loaded.tiles.assign(loaded.height(), std::string(loaded.width(), '.'));
		loadedMaps.push_back(loaded);
	}
	maps.swap(loadedMaps);
	return true;
}

bool CatalogMapStorage::saveMaps(const std::string& directory,
	const std::vector<WorldMap>& maps, std::string& error)
{
	for (size_t index = 0; index < maps.size(); ++index)
		if (!saveMap(directory + "/" + maps[index].id + ".json", maps[index], error))
			return false;
	return true;
}
