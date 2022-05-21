//
// Created by raind on 5/21/2022.
//
#include <functional>
#include <string>

#pragma once

#ifdef MACOS
#define time_t float
#endif

class Asset
{
public:
	void reload();

protected:
	Asset(const std::string &relative_path, const std::function<void()> &load_action);
	std::string relative_path;
	std::function<void()> load_action;

private:
	time_t last_load_time = 0;
};

void reloadAllAssets();