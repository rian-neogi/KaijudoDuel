#!/usr/bin/env python3

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import convert_legacy_world as converter


class LegacyWorldConversionTests(unittest.TestCase):
	@staticmethod
	def rle_value(runs, position):
		for value, count in runs:
			if position < count:
				return value
			position -= count
		raise AssertionError("RLE position is outside the layer")

	def test_every_legacy_glyph_has_valid_catalog_references(self):
		expected_glyphs = set(".=~HT#WDFCBA SMQERXGIPVKJUO1234567890"
			"abcdefghijklmnopqrstuv wxyz".replace(" ", ""))
		self.assertEqual(set(converter.GLYPH_TILES), expected_glyphs)
		repository = Path(__file__).resolve().parent.parent
		for glyph, references in converter.GLYPH_TILES.items():
			self.assertTrue(references, glyph)
			for reference in references:
				self.assertEqual(reference["tint"], [255, 255, 255], glyph)
				metadata = repository / "Resources" / "Graphics" / "Tilesets" / (
					reference["tileset"] + "_" + reference["sheet"] + ".txt")
				self.assertTrue(metadata.is_file(), str(metadata))
				tile_count = len(metadata.read_text(encoding="utf-8-sig").splitlines())
				self.assertGreaterEqual(reference["index"], 0, glyph)
				self.assertLess(reference["index"], tile_count, glyph)

	def test_converts_only_base_byte_map(self):
		lua = r'''
		return {
			maps = {
				{
					id = "test_map",
					name = "Test Map",
					indoor = false,
					tiles = { ".z", "~=" },
					tile_layers = {
						{ x = 0, y = 0, layer = "foreground",
						  tileset = "Ignored", sheet = "C", index = 999 },
					},
				},
			},
			regions = {
				{ id = "test", name = "Test", map = "test_map", kind = "town",
				  x = 0, y = 0, width = 2, height = 2 },
			},
			start = { map = "test_map", x = 0, y = 0 },
			portals = {},
			npcs = { ["guide"] = { map = "test_map", x = 1, y = 1 } },
			objects = {},
			shards = {},
		}
		'''
		with tempfile.TemporaryDirectory() as temporary_directory:
			root = Path(temporary_directory)
			input_path = root / "World.lua"
			output_path = root / "maps"
			manifest_path = root / "World.json"
			input_path.write_text(lua, encoding="utf-8")
			converter.convert_file(input_path, output_path, manifest_path)
			result = json.loads((output_path / "test_map.json").read_text(
				encoding="utf-8"))
			manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

		self.assertEqual(result["format"], converter.FORMAT_NAME)
		self.assertEqual((result["width"], result["height"]), (2, 2))
		self.assertEqual(result["source"]["ignoredLuaFields"], ["tile_layers"])
		self.assertFalse(any(entry and entry["tileset"] == "Ignored"
			for entry in result["palette"]))
		self.assertEqual(result["tags"], [
			{"x": 1, "y": 0, "value": "blackstone_gate"},
		])
		self.assertEqual(manifest["format"], "kaijudo-world")
		self.assertEqual(manifest["maps"], ["Maps/test_map.json"])
		self.assertEqual(manifest["entities"]["npcs"][0]["id"], "guide")
		for layer in converter.LAYERS:
			self.assertEqual(sum(run[1] for run in result["layers"][layer]["data"]), 4)

	def test_expands_and_offsets_overworld_around_emberglen(self):
		lua = r'''
		return {
			maps = {
				{ id = "overworld", name = "Exterior", indoor = false,
				  tiles = { ".=", "~." } },
				{ id = "inside", name = "Inside", indoor = true, tiles = { "F" } },
			},
			regions = {
				{ id = "emberglen", name = "Emberglen", map = "overworld",
				  kind = "town", x = 10, y = 20, width = 2, height = 2 },
			},
			start = { map = "overworld", x = 11, y = 21 },
			portals = {
				{ from = { map = "overworld", x = 10, y = 20 },
				  to = { map = "inside", x = 0, y = 0 } },
			},
			npcs = { ["guide"] = { map = "overworld", x = 11, y = 20 } },
			objects = {},
			shards = {},
		}
		'''
		with tempfile.TemporaryDirectory() as temporary_directory:
			root = Path(temporary_directory)
			input_path = root / "World.lua"
			output_path = root / "maps"
			manifest_path = root / "World.json"
			input_path.write_text(lua, encoding="utf-8")
			converter.convert_file(input_path, output_path, manifest_path)
			result = json.loads((output_path / "overworld.json").read_text(
				encoding="utf-8"))
			manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

		self.assertEqual((result["width"], result["height"]), (1024, 1024))
		self.assertEqual(result["source"]["offset"], {"x": 502, "y": 680})
		ground = result["layers"]["ground"]["data"]
		self.assertEqual(self.rle_value(ground, 0), 0)
		self.assertNotEqual(self.rle_value(ground, 680 * 1024 + 502), 0)
		self.assertEqual(self.rle_value(ground, 1024 * 1024 - 1), 0)
		self.assertEqual(manifest["regions"][0]["x"], 512)
		self.assertEqual(manifest["regions"][0]["y"], 700)
		self.assertEqual(manifest["start"], {"map": "overworld", "x": 513,
			"y": 701})
		self.assertEqual(manifest["portals"][0]["from"],
			{"map": "overworld", "x": 512, "y": 700})
		self.assertEqual(manifest["portals"][0]["to"],
			{"map": "inside", "x": 0, "y": 0})
		self.assertEqual(manifest["entities"]["npcs"][0]["x"], 513)
		palette_keys = [(entry["tileset"], entry["sheet"], entry["index"],
			entry["layer"]) for entry in result["palette"] if entry]
		self.assertEqual(len(palette_keys), len(set(palette_keys)))

	def test_rejects_unknown_glyph(self):
		legacy_map = {
			"id": "bad",
			"name": "Bad",
			"indoor": False,
			"tiles": ["!"],
		}
		with self.assertRaises(converter.ConversionError):
			converter.convert_map(legacy_map)


if __name__ == "__main__":
	unittest.main()
