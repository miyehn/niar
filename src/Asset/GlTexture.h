#pragma once
#include "Utils/lib.h"
#include <unordered_map>

struct GlTexture {
	
	struct ResourceInfo {
		std::string path;
		bool SRGB;
	};

	CONST_PTR(GlTexture, white);
	CONST_PTR(GlTexture, black);
	CONST_PTR(GlTexture, default_normal);

	static GlTexture* get(const std::string& name);
	static void set_resource_info(const std::string& name, const std::string& path, bool SRGB_in);
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

	static GlTexture* create_texture_8bit(unsigned char* data, int w, int h, int nc, bool SRGB);

	static std::unordered_map<std::string, ResourceInfo> texture_resource_infos;
	static std::unordered_map<std::string, GlTexture*> texture_pool;
	
};
