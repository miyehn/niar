//
// Created by raind on 5/21/2022.
//

#include "Asset.h"
#include "Utils/myn/Log.h"
#include "SceneAsset.h"
#include <filesystem>
#include <unordered_map>
#include <algorithm>

#include "Utils/myn/Misc.h"

std::unordered_map<std::string, Asset*> Asset::assets_pool;
uint32_t Asset::next_callback_id = 1;
std::unordered_map<uint32_t, std::pair<Asset*, Asset::CallbackStage>> Asset::callback_registry;

Asset::Asset(const std::string &_path, bool _reloadable): reloadable(_reloadable)
{
	virtual_path = _path;
	assets_pool[virtual_path] = this;
}

bool Asset::is_outdated()
{
	if (_version == 0) return true;
	time_t last_write_time = myn::get_file_last_write_time(ROOT_DIR"/" + virtual_path);
	return last_load_time < last_write_time;
}

void Asset::initialize_or_reload_outdated() {
	if (is_outdated()) {
		if (_version == 0 || reloadable) {
			// begin reload callbacks
			for (auto& cb : reload_callbacks[BeforeReload]) cb.fn();
			// reload
			last_load_time = myn::get_file_clock_now();
			ASSET("loading asset '%s (v%d)'", virtual_path.c_str(), _version + 1)
			load_action_internal();
			bump_version();
			for (auto& cb : reload_callbacks[AfterReload]) cb.fn();
		} else {
			WARN("'%s' was edited but not reloaded because this is not a reloadable asset type", virtual_path.c_str())
		}
	}
}

Asset::~Asset() {
	assets_pool.erase(virtual_path);
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
	if (_version > 0) {
		ASSET("releasing asset %s", virtual_path.c_str())
	}
}

void Asset::initialize_or_reload_all_outdated() {
	for (auto& p : assets_pool) {
		p.second->initialize_or_reload_outdated();
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
