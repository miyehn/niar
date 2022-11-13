//
// Created by miyehn on 11/13/2022.
//

#include <stb_image/stb_image_write.h>
#include <windows.h>
#include "CpuTexture.h"

namespace myn {

using namespace glm;

CpuTexture::CpuTexture(int width, int height) : width(width), height(height), buffer(width * height) {}

void CpuTexture::storeTexel(int x, int y, const glm::vec4 &col) {
	buffer[y * width + x] = col;
}

glm::vec4 CpuTexture::loadTexel(int x, int y) const {
	return buffer[y * width + x];
}

void CpuTexture::writeFile(const std::string &filename, bool gammaCorrect, bool openFile) const {
	std::vector<u8vec4> u8buf(buffer.size());
	for (int i = 0; i < buffer.size(); i++) {
		auto p = buffer[i];
		p = clamp(p, vec4(0), vec4(1));
		if (gammaCorrect) {
			const vec4 gamma(vec3(0.455f), 1.0f);
			p = glm::pow(p, gamma);
		}
		u8buf[i] = u8vec4(p.x * 255, p.y * 255, p.z * 255, p.w * 255);
	}
	stbi_write_png(filename.c_str(), width, height, 4, u8buf.data(), width * 4);
	if (openFile) {
		ShellExecute(0, "open", filename.c_str(), 0, 0, SW_SHOW);
	}
}


} // namespace myn

