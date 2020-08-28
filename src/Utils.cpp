#include "Utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

void AABB::add_point(vec3 p) {
	min.x = glm::min(min.x, p.x);
	min.y = glm::min(min.y, p.y);
	min.z = glm::min(min.z, p.z);
	max.x = glm::max(max.x, p.x);
	max.y = glm::max(max.y, p.y);
	max.z = glm::max(max.z, p.z);
}

void AABB::merge(const AABB& other) {
	min.x = glm::min(min.x, other.min.x);
	min.y = glm::min(min.y, other.min.y);
	min.z = glm::min(min.z, other.min.z);
	max.x = glm::max(max.x, other.max.x);
	max.y = glm::max(max.y, other.max.y);
	max.z = glm::max(max.z, other.max.z);
}

AABB AABB::merge(const AABB& A, const AABB& B) {
	AABB res;
	res.merge(A);
	res.merge(B);
	return res;
}

std::vector<vec3> AABB::corners() {
	std::vector<vec3> res(8);
	res[0] = min;
	res[1] = vec3(max.x, min.y, min.z);
	res[2] = vec3(min.x, max.y, min.z);
	res[3] = vec3(max.x, max.y, min.z);
	res[4] = vec3(min.x, min.y, max.z);
	res[5] = vec3(max.x, min.y, max.z);
	res[6] = vec3(min.x, max.y, max.z);
	res[7] = max;
	return res;
}

//------------------------------------------------

quat quat_from_dir(vec3 dir) {
	if (dot(dir, vec3(0, 0, -1)) > 1.0f - EPSILON) {
		return quat();
	}
	float costheta = dot(dir, vec3(0, 0, -1));
	float angle = acos(dot(dir, vec3(0, 0, -1)));
	vec3 axis = normalize(cross(vec3(0, 0, -1), dir));

	float c = cos(angle / 2);
	float s = sin(angle / 2);

	return quat(c, s*axis.x, s*axis.y, s*axis.z);
}

std::string s3(vec3 v) { 
	return ("(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", "  + std::to_string(v.z) + ")").c_str();
}

//------------------------------------------------

std::unordered_map<std::string, Texture*> Texture::texture_pool;

unsigned char* load_image(const std::string& path, int& width, int& height, int& nChannels) {
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 0);
	return data;
}

Texture* Texture::get(const std::string& path) {
	auto found_tex = texture_pool.find(path);
	if (found_tex != texture_pool.end()) {
		return found_tex->second;
	}
	// make texture instance
	Texture* tex = new Texture();
	unsigned char* data = stbi_load(path.c_str(), 
			&tex->width_value, &tex->height_value, &tex->num_channels_value, 0);
	if (!data) {
		ERRF("could not load image at path: %s", path.c_str());
		return tex;
	}

	glGenTextures(1, &tex->id_value);
	glBindTexture(GL_TEXTURE_2D, tex->id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (tex->num_channels() == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex->width(), tex->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	} else if (tex->num_channels() == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width(), tex->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	} else {
		WARN("trying to create a texture neither 3 channels nor 4 channels??");
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);

	texture_pool[path] = tex;
	GL_ERRORS();

	LOGF("created texture of size %dx%d with %d channels", tex->width(), tex->height(), tex->num_channels());
	return tex;
}

void Texture::cleanup() {
	for (auto tex : texture_pool) {
		glDeleteTextures(1, &tex.second->id_value);
		GL_ERRORS();
		delete tex.second;
	}
}
