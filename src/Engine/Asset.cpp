//
// Created by raind on 5/21/2022.
//

#include "Asset.h"
#include "Utils/myn/Log.h"
#include "GltfAsset.h"
#include <filesystem>
#include <unordered_map>

#ifdef MACOS
#define to_time_t(diff) std::chrono::duration<time_t, std::ratio<86400>>(diff).count()
#endif

time_t get_file_clock_now() {
#ifdef MACOS
	auto epoch = std::chrono::file_clock::time_point();
	auto now = std::chrono::file_clock::now();
	return to_time_t(now - epoch);
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
	return to_time_t(file_time - epoch);
#else
	auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
	return std::chrono::system_clock::to_time_t(system_time);
#endif
}
#ifdef MACOS
#undef to_time_t
#endif

namespace
{
std::unordered_map<std::string, Asset*> assets_pool;
}

Asset::Asset(const std::string &_path, const std::function<void()> &_load_action)
{
	relative_path = _path;
	load_action = _load_action;
	reload_condition = [](){ return true; };
	assets_pool[relative_path] = this;
	if (load_action) reload();
}

void Asset::reload() {
	time_t last_write_time = get_last_write_time(ROOT_DIR"/" + relative_path);
	if (last_load_time < last_write_time) {
		LOG("loading asset '%s'", relative_path.c_str())
		if (!_initialized || reload_condition()) {
			load_action();
			last_load_time = get_file_clock_now();
			_initialized = true;
		} else {
			WARN("'%s' was edited but not reloaded: condition not met", relative_path.c_str())
		}
	}
}

Asset::~Asset() {
	assets_pool.erase(relative_path);
}

void reloadAllAssets()
{
	for (auto& p : assets_pool) {
		p.second->reload();
	}
}

void releaseAllAssets()
{
	for (const auto& pair : assets_pool) {
		auto asset = pair.second;
		if (asset) {
			if (auto* gltf = dynamic_cast<GltfAsset*>(pair.second)) {
				gltf->release_resources();
			}
		}
	}
}