#pragma once
#include "lib.h"
#include <unordered_map>

struct Texture {
	
	static Texture* get(const std::string& name);
	static void set_path(const std::string& name, const std::string& path);
	static void cleanup();

	uint id() { return id_value; }
	int width() { return width_value; }
	int height() { return height_value; }
	int num_channels() { return num_channels_value; }

private:
	uint id_value = 0;
	int width_value = 0;
	int height_value = 0;
	int num_channels_value = 0;

	static Texture* create_texture_8bit(unsigned char* data, int w, int h, int nc);

	static std::unordered_map<std::string, std::string> texture_paths;
	static std::unordered_map<std::string, Texture*> texture_pool;

	static Texture* white_value;
	
};
