//
// Created by raind on 5/21/2022.
//

#include "Asset.h"
#include "Utils/myn/Log.h"
#include "SceneAsset.h"
#include <filesystem>
#include <unordered_map>
#include <algorithm>

time_t get_file_clock_now() {
	auto tp = std::chrono::system_clock::now();
	return std::chrono::system_clock::to_time_t(tp);
}

time_t get_last_write_time(const std::string& path)
{
	auto file_time = std::filesystem::last_write_time(path);
	auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
	return std::chrono::system_clock::to_time_t(system_time);
}

std::unordered_map<std::string, Asset*> Asset::assets_pool;
uint32_t Asset::next_callback_id = 1;
std::unordered_map<uint32_t, std::pair<Asset*, Asset::CallbackStage>> Asset::callback_registry;

Asset::Asset(const std::string &_path)
{
	relative_path = _path;
	reload_condition = [](){ return true; };
	assets_pool[relative_path] = this;
}

void Asset::reload() {
	time_t last_write_time = get_last_write_time(ROOT_DIR"/" + relative_path);
	if (last_load_time < last_write_time) {
		if (!_initialized || reload_condition()) {
			// begin reload callbacks
				for (auto& cb : reload_callbacks[BeforeReload]) cb.fn();
			// reload
			last_load_time = get_file_clock_now();
			if (_initialized) bump_version();
			ASSET("loading asset '%s (now at v%d)'", relative_path.c_str(), _version)
			load_action_internal();
			_initialized = true;
				for (auto& cb : reload_callbacks[AfterReload]) cb.fn();
		} else {
			WARN("'%s' was edited but not reloaded: condition not met", relative_path.c_str())
		}
	}
}

Asset::~Asset() {
	ASSERT(!_initialized)
	assets_pool.erase(relative_path);
}

uint32_t Asset::register_callback(Asset* asset, CallbackStage stage, const std::function<void()>& callback)
{
	const uint32_t id = next_callback_id++;
	asset->reload_callbacks[stage].push_back({id, callback});
	callback_registry[id] = {asset, stage};
	return id;
}

bool Asset::unregister_callback(uint32_t callbackId)
{
	if (callback_registry.empty()) {
		// asset callback registry cleared before this callback is unregistered
		// fine because without the asset(s), the callbacks not unregistered won't be called anyway
		return false;
	}

	const auto it = callback_registry.find(callbackId);
	ASSERT(it != callback_registry.end())

	auto [asset, stage] = it->second;
	auto& vec = asset->reload_callbacks[stage];
	std::erase_if(vec, [callbackId](const ReloadCallback& cb){ return cb.id == callbackId; });

	callback_registry.erase(it);
	return true;
}

void Asset::release_resources() {
	if (_initialized) {
		ASSET("releasing asset %s", relative_path.c_str())
	}
	_initialized = false;
}

void Asset::reload_all() {
	for (auto& p : assets_pool) {
		p.second->reload();
	}
}

void Asset::release_all() {
	for (const auto& pair : assets_pool) {
		auto asset = pair.second;
		if (asset) {
			asset->release_resources();
		}
	}
}

void Asset::delete_all() {
	std::vector<Asset*> assets;
	for (const auto& pair : assets_pool) {
		assets.push_back(pair.second);
	}
	for (const auto& asset : assets) {
		delete asset;
	}
	assets_pool.clear();
	callback_registry = {};
}
