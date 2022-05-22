//
// Created by raind on 5/21/2022.
//
#include <functional>
#include <string>

#pragma once

#ifdef MACOS
#define time_t double
#endif

class Asset
{
public:
	void reload();
	virtual ~Asset();

protected:
	Asset(const std::string &relative_path, const std::function<void()> &load_action);
	std::string relative_path;
	std::function<void()> load_action;
	std::function<bool()> reload_condition;

private:
	time_t last_load_time = 0;
};

void reloadAllAssets();
void releaseAllAssets();