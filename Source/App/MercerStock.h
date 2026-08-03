#pragma once

#include <string>
#include <vector>

struct MercerShard
{
	std::string id;
	std::string mapId;
	std::string name;
	int x;
	int y;
	std::vector<std::string> stock;
};

struct MercerStockData
{
	int prices[5];
	std::vector<std::string> initialStock;
	std::vector<MercerShard> shards;

	MercerStockData();
};

bool loadMercerStockFromLua(const std::string& path, MercerStockData& stock,
	std::string& error);
