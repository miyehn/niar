//
// Created by raind on 5/21/2022.
//

#include "Asset.h"
#include "Utils/myn/Log.h"
#include <filesystem>

time_t get_last_write_time(const std::string& path)
{
	auto file_time = std::filesystem::last_write_time(path);
	auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
	return std::chrono::system_clock::to_time_t(system_time);
}

namespace
{
std::unordered_map<std::string, Asset*> assets_pool;
}

Asset::Asset(const std::string &_path, const std::function<void()> &_load_action)
{
	path = _path;
	load_action = _load_action;
	assets_pool[path] = this;
	reload();
}

void Asset::reload()
{
	time_t last_write_time = get_last_write_time(path);
	if (last_load_time < last_write_time) {
		LOG("loading %s", path.c_str())
		load_action();
		last_load_time = last_write_time;
	}
}

void reloadAllAssets()
{
	for (auto& p : assets_pool) {
		p.second->reload();
	}
}