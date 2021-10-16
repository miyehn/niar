#include "GlTexture.h"

#include "stb_image/stb_image.h"

// might be relevant: https://github.com/nothings/stb/issues/103
GlTexture* GlTexture::create_texture_8bit(unsigned char* data, int w, int h, int nc, bool SRGB) {
	GlTexture* tex = new GlTexture();
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
		glTexImage2D(GL_TEXTURE_2D, 0, SRGB ? GL_SRGB : GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	} else if (nc == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, SRGB ? GL_SRGB_ALPHA : GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	} else {
		WARN("trying to create a texture neither 3 channels nor 4 channels??");
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();
	return tex;
}

std::unordered_map<std::string, GlTexture*> GlTexture::texture_pool;
std::unordered_map<std::string, GlTexture::ResourceInfo> GlTexture::texture_resource_infos;

void GlTexture::set_resource_info(const std::string& name, const std::string& path_in, bool SRGB_in) {
	ResourceInfo info;
	info.path = path_in;
	info.SRGB = SRGB_in;
	texture_resource_infos[name] = info;
}

#define IMPLEMENT_CONST_COLOR_TEX(NAME, R, G, B) \
	GlTexture* GlTexture::NAME##_value = nullptr; \
	GlTexture* GlTexture::NAME() { \
		if (NAME##_value) return NAME##_value; \
		std::vector<u8vec3> NAME##_data(4 * 4 * 3); \
		for (int i=0; i<NAME##_data.size(); i++) { \
			NAME##_data[i] = u8vec3(R, G, B); \
		} \
		NAME##_value = create_texture_8bit((unsigned char*)NAME##_data.data(), 4, 4, 3, false); \
		return NAME##_value; \
	}

IMPLEMENT_CONST_COLOR_TEX(white, 255, 255, 255);
IMPLEMENT_CONST_COLOR_TEX(black, 0, 0, 0);
IMPLEMENT_CONST_COLOR_TEX(default_normal, 127, 127, 255);

GlTexture* GlTexture::get(const std::string& name) {

	std::string path;
	bool SRGB;
	auto info_pair = texture_resource_infos.find(name);
	if (info_pair == texture_resource_infos.end()) {
		if (name=="white") return white();
		else if (name=="black") return black();
		else if (name=="defaultNormal") return default_normal();
		ERR("There isn't a texture called %s", name.c_str());
		return nullptr;
	} else {
		path = info_pair->second.path;
		SRGB = info_pair->second.SRGB;
	}

	auto found_tex = texture_pool.find(path);
	if (found_tex != texture_pool.end()) {
		return found_tex->second;
	}
	// make texture instance
	int w, h, nc;
	unsigned char* data = stbi_load(path.c_str(), &w, &h, &nc, 0);
	if (!data) {
		ERR("could not load image at path: %s", path.c_str());
		return nullptr;
	}

	GlTexture* tex = create_texture_8bit(data, w, h, nc, SRGB);
	stbi_image_free(data);

	texture_pool[path] = tex;

	LOG("(created texture '%s' of size %dx%d with %d channels)", name.c_str(), tex->width(), tex->height(), tex->num_channels());
	return tex;
}

void GlTexture::cleanup() {
	for (auto tex : texture_pool) {
		glDeleteTextures(1, &tex.second->id_value);
		GL_ERRORS();
		delete tex.second;
	}
}
