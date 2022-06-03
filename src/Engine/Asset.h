//
// Created by raind on 5/21/2022.
//
#include <functional>
#include <string>
#include <vector>

#pragma once

#ifdef MACOS
#define time_t double
#endif

class Asset
{
public:
	uint32_t get_version() const { return _version; }
	void reload();
	virtual ~Asset();

	static Asset* find(const std::string& key);

	std::function<bool()> reload_condition = [](){ return true; };
	std::vector<std::function<void()>> begin_reload;
	std::vector<std::function<void()>> finish_reload;

protected:
	Asset(const std::string &relative_path, const std::function<void()> &load_action);
	std::string relative_path;
	std::function<void()> load_action_internal = nullptr;

	void bump_version() { _version += 1; }

private:
	bool _initialized = false;
	time_t last_load_time = 0;
	uint32_t _version = 0;
};

void reloadAllAssets();
void releaseAllAssets();