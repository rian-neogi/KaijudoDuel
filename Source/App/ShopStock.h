#pragma once

#include <string>
#include <vector>

struct ShopStock
{
	std::string id;
	std::string name;
	std::vector<std::string> initialStock;
	std::vector<std::string> actThreeBonus;
};

struct ShopStockData
{
	std::vector<ShopStock> shops;

	const ShopStock* find(const std::string& id) const;
};

bool loadShopStockFromLua(const std::string& path, ShopStockData& stock,
	std::string& error);
