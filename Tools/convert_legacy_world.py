#!/usr/bin/env python3
"""Migrate Lua/World.lua into the native world manifest and catalog maps.

Visual conversion reads only each map's ``tiles`` string array. Existing
``tile_layers`` are intentionally ignored, while regions, start, portals, and
entity positions are copied into the native manifest.
"""

import argparse
import json
import os
import re
import tempfile
from pathlib import Path


FORMAT_NAME = "kaijudo-catalog-map"
FORMAT_VERSION = 1
LAYERS = ("ground", "decoration", "foreground")
OVERWORLD_MAP_ID = "overworld"
OVERWORLD_WIDTH = 1024
OVERWORLD_HEIGHT = 1024
OVERWORLD_ANCHOR_REGION = "emberglen"
OVERWORLD_ANCHOR_X = 512
OVERWORLD_ANCHOR_Y = 700


def tile(tileset, sheet, index, layer="ground", tint=(255, 255, 255)):
	return {
		"tileset": tileset,
		"sheet": sheet,
		"index": index,
		"layer": layer,
		"tint": list(tint),
	}


def ground(tileset, sheet, index, tint=(255, 255, 255)):
	return tile(tileset, sheet, index, "ground", tint)


def decoration(tileset, sheet, index, tint=(255, 255, 255)):
	return tile(tileset, sheet, index, "decoration", tint)


# These mappings mirror the catalog references used by the compatibility
# renderer. Natural decorations use the corresponding named catalog tiles in
# place of the compatibility renderer's hand-cut, multi-cell atlas regions.
# Legacy regional color substitutions are intentionally normalized; authored
# native maps can still use the tint field when a color variation is desired.
MEADOW = ground("Outside", "A2", 0)
WOOD_FLOOR = ground("Inside", "A2", 0)
INDUSTRIAL_FLOOR = ground("Dungeon", "A5", 51)

GLYPH_TILES = {
	".": [MEADOW],
	"=": [ground("Outside", "A2", 1)],
	"~": [ground("Outside", "A1", 0)],
	"H": [ground("Outside", "A4", 0), decoration("World", "B", 63)],
	"T": [MEADOW, decoration("Outside", "B", 93)],
	"#": [ground("Outside", "A2", 0), decoration("Outside", "B", 101)],
	"W": [ground("Inside", "A4", 16)],
	"D": [WOOD_FLOOR, decoration("Inside", "B", 195)],
	"F": [WOOD_FLOOR],
	"C": [WOOD_FLOOR, decoration("Inside", "B", 114)],
	"B": [MEADOW, decoration("Outside", "B", 83)],
	"A": [MEADOW, decoration("Inside", "B", 96)],
	"S": [ground("Outside", "A2", 16)],
	"M": [ground("Outside", "A4", 13)],
	"Q": [ground("Outside", "A3", 4)],
	"E": [WOOD_FLOOR, decoration("Inside", "C", 148)],
	"R": [INDUSTRIAL_FLOOR, decoration("Dungeon", "C", 104)],
	"X": [INDUSTRIAL_FLOOR],
	"G": [ground("Dungeon", "A5", 7)],
	"I": [ground("Dungeon", "A4", 43)],
	"P": [INDUSTRIAL_FLOOR, decoration("Inside", "C", 151)],
	"V": [INDUSTRIAL_FLOOR, decoration("Inside", "B", 112)],
	"K": [ground("Outside", "A3", 16)],
	"J": [ground("Outside", "A3", 7)],
	"U": [ground("Outside", "A5", 5)],
	"O": [ground("Outside", "A4", 40)],
	"1": [ground("Outside", "A2", 3)],
	"2": [ground("Outside", "A2", 3),
		decoration("Dungeon", "B", 145)],
	"3": [ground("Outside", "A2", 8)],
	"4": [ground("Outside", "A2", 10)],
	"5": [ground("Outside", "A2", 8),
		decoration("Dungeon", "B", 104)],
	"6": [ground("Outside", "A2", 16)],
	"7": [INDUSTRIAL_FLOOR, decoration("Dungeon", "B", 129)],
	"8": [ground("Outside", "A2", 0)],
	"9": [ground("Outside", "A2", 1)],
	"0": [ground("Outside", "A2", 0),
		decoration("Outside", "B", 73)],
	"a": [ground("Outside", "A2", 3)],
	"b": [ground("Outside", "A2", 19)],
	"c": [ground("Outside", "A3", 2)],
	"d": [ground("Outside", "A5", 5)],
	"e": [ground("Outside", "A4", 13)],
	"f": [ground("Outside", "A2", 3),
		decoration("Outside", "B", 67)],
	"g": [ground("Outside", "A2", 19)],
	"h": [ground("Outside", "A2", 3),
		decoration("Outside", "B", 73)],
	"i": [ground("Outside", "A2", 0)],
	"j": [ground("Outside", "A2", 1)],
	"k": [ground("Outside", "A4", 37)],
	"l": [ground("Outside", "A5", 5)],
	"m": [ground("Outside", "A3", 19)],
	"n": [ground("Outside", "A4", 25)],
	"o": [ground("Outside", "A2", 0),
		decoration("Outside", "B", 67)],
	"p": [ground("Outside", "A2", 1)],
	"q": [ground("Outside", "A2", 0),
		decoration("Outside", "B", 73)],
	"r": [MEADOW, decoration("Outside", "B", 107)],
	"s": [MEADOW, decoration("Outside", "B", 102)],
	"t": [ground("Outside", "A2", 4)],
	"u": [ground("Outside", "A2", 0),
		decoration("Outside", "C", 118)],
	"v": [MEADOW, decoration("Outside", "B", 92)],
	"w": [ground("Outside", "A2", 8)],
	"x": [ground("Outside", "A2", 10)],
	"y": [ground("Dungeon", "A4", 25)],
	"z": [ground("Dungeon", "A5", 51), decoration("Dungeon", "C", 56)],
}


class ConversionError(RuntimeError):
	pass


def _find_matching_brace(text, opening):
	depth = 0
	quote = None
	escaped = False
	line_comment = False
	index = opening
	while index < len(text):
		character = text[index]
		if line_comment:
			if character == "\n":
				line_comment = False
			index += 1
			continue
		if quote is not None:
			if escaped:
				escaped = False
			elif character == "\\":
				escaped = True
			elif character == quote:
				quote = None
			index += 1
			continue
		if character in ('"', "'"):
			quote = character
		elif character == "-" and index + 1 < len(text) and text[index + 1] == "-":
			line_comment = True
			index += 1
		elif character == "{":
			depth += 1
		elif character == "}":
			depth -= 1
			if depth == 0:
				return index
		index += 1
	raise ConversionError("unterminated Lua table")


def _field_table(text, field):
	match = re.search(r"\b" + re.escape(field) + r"\s*=\s*\{", text)
	if match is None:
		raise ConversionError("missing '%s' table" % field)
	opening = text.find("{", match.start())
	closing = _find_matching_brace(text, opening)
	return text[opening + 1:closing]


def _top_level_tables(text):
	tables = []
	depth = 0
	quote = None
	escaped = False
	line_comment = False
	opening = -1
	index = 0
	while index < len(text):
		character = text[index]
		if line_comment:
			if character == "\n":
				line_comment = False
			index += 1
			continue
		if quote is not None:
			if escaped:
				escaped = False
			elif character == "\\":
				escaped = True
			elif character == quote:
				quote = None
			index += 1
			continue
		if character in ('"', "'"):
			quote = character
		elif character == "-" and index + 1 < len(text) and text[index + 1] == "-":
			line_comment = True
			index += 1
		elif character == "{":
			if depth == 0:
				opening = index
			depth += 1
		elif character == "}":
			depth -= 1
			if depth < 0:
				raise ConversionError("unexpected closing brace")
			if depth == 0 and opening >= 0:
				tables.append(text[opening:index + 1])
				opening = -1
		index += 1
	if depth != 0 or quote is not None:
		raise ConversionError("unterminated Lua map table")
	return tables


def _string_field(text, field, required=True):
	match = re.search(r"\b" + re.escape(field) +
		r"\s*=\s*(\"(?:\\.|[^\"\\])*\")", text)
	if match is None:
		if required:
			raise ConversionError("missing string field '%s'" % field)
		return ""
	return json.loads(match.group(1))


def _int_field(text, field):
	match = re.search(r"\b" + re.escape(field) + r"\s*=\s*(-?\d+)", text)
	if match is None:
		raise ConversionError("missing integer field '%s'" % field)
	return int(match.group(1))


def _string_array(table_text):
	values = []
	for match in re.finditer(r"\"(?:\\.|[^\"\\])*\"", table_text):
		values.append(json.loads(match.group(0)))
	return values


def parse_legacy_maps(lua_text):
	maps_table = _field_table(lua_text, "maps")
	maps = []
	for map_table in _top_level_tables(maps_table):
		map_id = _string_field(map_table, "id")
		name = _string_field(map_table, "name")
		indoor_match = re.search(r"\bindoor\s*=\s*(true|false)", map_table)
		tile_rows = _string_array(_field_table(map_table, "tiles"))
		if not tile_rows or not tile_rows[0]:
			raise ConversionError("map '%s' has an empty tile grid" % map_id)
		width = len(tile_rows[0])
		if any(len(row) != width for row in tile_rows):
			raise ConversionError("map '%s' has a non-rectangular tile grid" % map_id)
		maps.append({
			"id": map_id,
			"name": name,
			"indoor": indoor_match is not None and indoor_match.group(1) == "true",
			"tiles": tile_rows,
		})
	if not maps:
		raise ConversionError("World.lua contains no maps")
	return maps


def _position(table_text):
	return {
		"map": _string_field(table_text, "map"),
		"x": _int_field(table_text, "x"),
		"y": _int_field(table_text, "y"),
	}


def _position_entries(lua_text, field):
	table = _field_table(lua_text, field)
	entries = []
	pattern = re.compile(r"\[\s*(\"(?:\\.|[^\"\\])*\")\s*\]\s*=\s*\{")
	for match in pattern.finditer(table):
		opening = table.find("{", match.start())
		closing = _find_matching_brace(table, opening)
		entry = {"id": json.loads(match.group(1))}
		entry.update(_position(table[opening:closing + 1]))
		entries.append(entry)
	return entries


def parse_legacy_metadata(lua_text, legacy_maps):
	regions = []
	for table in _top_level_tables(_field_table(lua_text, "regions")):
		regions.append({
			"id": _string_field(table, "id"),
			"name": _string_field(table, "name"),
			"map": _string_field(table, "map"),
			"kind": _string_field(table, "kind"),
			"x": _int_field(table, "x"),
			"y": _int_field(table, "y"),
			"width": _int_field(table, "width"),
			"height": _int_field(table, "height"),
		})
	portals = []
	for table in _top_level_tables(_field_table(lua_text, "portals")):
		portals.append({
			"from": _position(_field_table(table, "from")),
			"to": _position(_field_table(table, "to")),
		})
	return {
		"format": "kaijudo-world",
		"version": 1,
		"maps": ["Maps/" + legacy_map["id"] + ".json"
			for legacy_map in legacy_maps],
		"regions": regions,
		"start": _position(_field_table(lua_text, "start")),
		"portals": portals,
		"entities": {
			"npcs": _position_entries(lua_text, "npcs"),
			"objects": _position_entries(lua_text, "objects"),
			"shards": _position_entries(lua_text, "shards"),
		},
	}


def expanded_overworld_layout(metadata, legacy_maps):
	overworld = next((legacy_map for legacy_map in legacy_maps
		if legacy_map["id"] == OVERWORLD_MAP_ID), None)
	if overworld is None:
		return {}
	anchor = next((region for region in metadata["regions"]
		if region["id"] == OVERWORLD_ANCHOR_REGION and
		region["map"] == OVERWORLD_MAP_ID), None)
	if anchor is None:
		raise ConversionError("overworld expansion requires the '%s' region" %
			OVERWORLD_ANCHOR_REGION)
	offset_x = OVERWORLD_ANCHOR_X - anchor["x"]
	offset_y = OVERWORLD_ANCHOR_Y - anchor["y"]
	source_width = len(overworld["tiles"][0])
	source_height = len(overworld["tiles"])
	if offset_x < 0 or offset_y < 0 or offset_x + source_width > OVERWORLD_WIDTH or \
		offset_y + source_height > OVERWORLD_HEIGHT:
		raise ConversionError("expanded overworld does not contain the legacy map")
	return {
		OVERWORLD_MAP_ID: {
			"width": OVERWORLD_WIDTH,
			"height": OVERWORLD_HEIGHT,
			"offset_x": offset_x,
			"offset_y": offset_y,
		},
	}


def apply_layout_to_metadata(metadata, layouts):
	def shift(position):
		layout = layouts.get(position["map"])
		if layout is not None:
			position["x"] += layout["offset_x"]
			position["y"] += layout["offset_y"]

	for region in metadata["regions"]:
		shift(region)
	shift(metadata["start"])
	for portal in metadata["portals"]:
		shift(portal["from"])
		shift(portal["to"])
	for group in ("npcs", "objects", "shards"):
		for position in metadata["entities"][group]:
			shift(position)
	return metadata


def _tile_key(reference):
	return (reference["tileset"], reference["sheet"], reference["index"],
		reference["layer"])


def _rle(values):
	if not values:
		return []
	runs = []
	current = values[0]
	count = 1
	for value in values[1:]:
		if value == current:
			count += 1
		else:
			runs.append([current, count])
			current = value
			count = 1
	runs.append([current, count])
	return runs


def convert_map(legacy_map, layout=None):
	source_width = len(legacy_map["tiles"][0])
	source_height = len(legacy_map["tiles"])
	width = layout["width"] if layout is not None else source_width
	height = layout["height"] if layout is not None else source_height
	offset_x = layout["offset_x"] if layout is not None else 0
	offset_y = layout["offset_y"] if layout is not None else 0
	if offset_x < 0 or offset_y < 0 or offset_x + source_width > width or \
		offset_y + source_height > height:
		raise ConversionError("map '%s' does not fit its output canvas" %
			legacy_map["id"])
	palette = [None]
	palette_indices = {}
	layer_values = {layer: [0] * (width * height) for layer in LAYERS}
	tags = []

	for y, row in enumerate(legacy_map["tiles"]):
		for x, glyph in enumerate(row):
			if glyph not in GLYPH_TILES:
				raise ConversionError("map '%s' has unknown glyph %r at (%d, %d)" %
					(legacy_map["id"], glyph, x, y))
			cell = {layer: 0 for layer in LAYERS}
			for reference in GLYPH_TILES[glyph]:
				key = _tile_key(reference)
				if key not in palette_indices:
					palette_indices[key] = len(palette)
					palette.append(reference)
				cell[reference["layer"]] = palette_indices[key]
			position = (y + offset_y) * width + x + offset_x
			for layer in LAYERS:
				layer_values[layer][position] = cell[layer]
			if glyph == "z":
				tags.append({"x": x + offset_x, "y": y + offset_y,
					"value": "blackstone_gate"})

	result = {
		"format": FORMAT_NAME,
		"version": FORMAT_VERSION,
		"id": legacy_map["id"],
		"name": legacy_map["name"],
		"indoor": legacy_map["indoor"],
		"width": width,
		"height": height,
		"palette": palette,
		"layers": {},
		"source": {
			"kind": "legacy-byte-map",
			"ignoredLuaFields": ["tile_layers"],
			"offset": {"x": offset_x, "y": offset_y},
			"sourceWidth": source_width,
			"sourceHeight": source_height,
		},
	}
	for layer in LAYERS:
		result["layers"][layer] = {
			"encoding": "rle-row-major",
			"data": _rle(layer_values[layer]),
		}
	if tags:
		result["tags"] = tags
	return result


def _atomic_json_write(path, value):
	path.parent.mkdir(parents=True, exist_ok=True)
	file_descriptor, temporary_name = tempfile.mkstemp(
		prefix=path.name + ".", suffix=".tmp", dir=str(path.parent))
	try:
		with os.fdopen(file_descriptor, "w", encoding="utf-8") as output:
			json.dump(value, output, indent=2, ensure_ascii=False)
			output.write("\n")
		os.replace(temporary_name, path)
	except Exception:
		try:
			os.unlink(temporary_name)
		except OSError:
			pass
		raise


def convert_file(input_path, output_directory, manifest_path=None):
	with input_path.open("r", encoding="utf-8") as source:
		lua_text = source.read()
		legacy_maps = parse_legacy_maps(lua_text)
	metadata = parse_legacy_metadata(lua_text, legacy_maps)
	layouts = expanded_overworld_layout(metadata, legacy_maps)
	apply_layout_to_metadata(metadata, layouts)
	seen = set()
	converted_maps = []
	for legacy_map in legacy_maps:
		map_id = legacy_map["id"]
		if not re.match(r"^[A-Za-z0-9_-]+$", map_id):
			raise ConversionError("map ID is not safe as a filename: %r" % map_id)
		if map_id in seen:
			raise ConversionError("duplicate map ID: %s" % map_id)
		seen.add(map_id)
		converted_maps.append((map_id, convert_map(legacy_map, layouts.get(map_id))))
	for map_id, converted_map in converted_maps:
		_atomic_json_write(output_directory / (map_id + ".json"), converted_map)
	if manifest_path is not None:
		_atomic_json_write(manifest_path, metadata)
	return legacy_maps


def main():
	parser = argparse.ArgumentParser(
		description="Migrate World.lua to the native manifest and catalog JSON maps.")
	parser.add_argument("--input", type=Path, default=Path("Lua/World.lua"),
		help="legacy World.lua path (default: Lua/World.lua)")
	parser.add_argument("--output", type=Path, default=Path("World/Maps"),
		help="output directory (default: World/Maps)")
	parser.add_argument("--manifest", type=Path, default=Path("World/World.json"),
		help="native world manifest (default: World/World.json)")
	arguments = parser.parse_args()
	try:
		maps = convert_file(arguments.input, arguments.output, arguments.manifest)
	except (ConversionError, OSError, ValueError) as error:
		parser.error(str(error))
	print("Migrated %d map(s) into %s and %s" %
		(len(maps), arguments.output, arguments.manifest))


if __name__ == "__main__":
	main()
