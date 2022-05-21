//
// Created by raind on 5/21/2022.
//

#include "Asset.h"
#include "Utils/myn/Log.h"
#include <filesystem>
#include <unordered_map>

time_t get_file_clock_now() {
#ifdef MACOS
	auto epoch = std::chrono::file_clock::time_point();
	auto now = std::chrono::file_clock::now();
	return std::chrono::duration<time_t>(now - epoch).count();
#else
	auto tp = std::chrono::system_clock::now();
	return std::chrono::system_clock::to_time_t(tp);
#endif
}

time_t get_last_write_time(const std::string& path)
{
	auto file_time = std::filesystem::last_write_time(path);
#ifdef MACOS
	auto epoch = std::chrono::file_clock::time_point();
	return std::chrono::duration<time_t>(file_time - epoch).count();
#else
	auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
	return std::chrono::system_clock::to_time_t(system_time);
#endif
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
	if (load_action) reload();
}

void Asset::reload()
{
	time_t last_write_time = get_last_write_time(ROOT_DIR"/" + path);
	if (last_load_time < last_write_time) {
		LOG("loading %s", path.c_str())
		load_action();
		last_load_time = get_file_clock_now();
	}
}

void reloadAllAssets()
{
	for (auto& p : assets_pool) {
		p.second->reload();
	}
}