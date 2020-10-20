#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

Texture* Texture::create_texture_8bit(unsigned char* data, int w, int h, int nc) {
	Texture* tex = new Texture();
	tex->width_value = w;
	tex->height_value = h;
	tex->num_channels_value = nc;
	glGenTextures(1, &tex->id_value);
	glBindTexture(GL_TEXTURE_2D, tex->id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (nc == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	} else if (nc == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	} else {
		WARN("trying to create a texture neither 3 channels nor 4 channels??");
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();
	return tex;
}

std::unordered_map<std::string, Texture*> Texture::texture_pool;
std::unordered_map<std::string, std::string> Texture::texture_paths;

void Texture::set_path(const std::string& name, const std::string& path) {
	texture_paths[name] = path;
}

Texture* Texture::white_value = nullptr;
Texture* Texture::white() {
	if (white_value) return white_value;
	std::vector<u8vec3> white_data(4 * 4 * 3);
	for (int i=0; i<white_data.size(); i++) {
		white_data[i] = u8vec3(255, 255, 255);
	}
	white_value = create_texture_8bit((unsigned char*)white_data.data(), 4, 4, 3);
	return white_value;
}

Texture* Texture::default_normal_value = nullptr;
Texture* Texture::default_normal() {
	if (default_normal_value) return default_normal_value;
	std::vector<u8vec3> default_normal_data(4 * 4 * 3);
	for (int i=0; i<default_normal_data.size(); i++) {
		default_normal_data[i] = u8vec3(127, 127, 255);
	}
	default_normal_value = create_texture_8bit((unsigned char*)default_normal_data.data(), 4, 4, 3);
	return default_normal_value;
}

Texture* Texture::get(const std::string& name) {

	std::string path;
	auto path_pair = texture_paths.find(name);
	if (path_pair == texture_paths.end()) {
		ERRF("There isn't a texture called %s", name.c_str());
		return nullptr;
	} else {
		path = path_pair->second;
	}

	auto found_tex = texture_pool.find(path);
	if (found_tex != texture_pool.end()) {
		return found_tex->second;
	}
	// make texture instance
	int w, h, nc;
	unsigned char* data = stbi_load(path.c_str(), &w, &h, &nc, 0);
	if (!data) {
		ERRF("could not load image at path: %s", path.c_str());
		return nullptr;
	}

	Texture* tex = create_texture_8bit(data, w, h, nc);
	stbi_image_free(data);

	texture_pool[path] = tex;

	LOGF("created texture '%s' of size %dx%d with %d channels", name.c_str(), tex->width(), tex->height(), tex->num_channels());
	return tex;
}

void Texture::cleanup() {
	for (auto tex : texture_pool) {
		glDeleteTextures(1, &tex.second->id_value);
		GL_ERRORS();
		delete tex.second;
	}
}
