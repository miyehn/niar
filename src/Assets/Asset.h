//
// Created by raind on 5/21/2022.
//
#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>


class Asset
{
public:

	enum CallbackStage
	{
		BeforeReload = 0,
		AfterReload = 1,
		CallbackStageCount = 2
	};

	uint32_t get_version() const { return _version; }

	// by default, it watches the asset file at relative_path. But can be overwritten if needed.
	virtual bool is_outdated();

	void initialize_or_reload_outdated();

	virtual ~Asset();

	template<typename Asset_T>
	static Asset_T* find(const std::string& key) {
		return dynamic_cast<Asset_T*>(assets_pool[key]);
	}

	const bool reloadable;

	static uint32_t register_callback(Asset* asset, CallbackStage stage, const std::function<void()>& callback);
	static bool unregister_callback(uint32_t callbackId);

	static void initialize_or_reload_all_outdated();

	static void release_all();

	static void delete_all();

protected:
	Asset(const std::string &relative_path, bool reloadable);
	std::string relative_path;
	std::function<void()> load_action_internal = nullptr;

	void bump_version() { _version += 1; }

	virtual void release_resources();

private:
	struct ReloadCallback {
		uint32_t id;
		std::function<void()> fn;
	};
	std::vector<ReloadCallback> reload_callbacks[CallbackStageCount];

	time_t last_load_time = 0;
	uint32_t _version = 0;

	static std::unordered_map<std::string, Asset*> assets_pool;
	static uint32_t next_callback_id;
	static std::unordered_map<uint32_t, std::pair<Asset*, CallbackStage>> callback_registry;
};