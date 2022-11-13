//
// Created by miyehn on 11/10/2022.
//

#include <stb_image/stb_image_write.h>
#include <windows.h>
#include "ShaderSimulator.h"
#include "Log.h"

namespace myn {

	using namespace glm;

	ShaderSimulator::ShaderSimulator(int width, int height, const std::string &filename)
		: width(width), height(height), filename(filename), buffer(width * height) {}

	void ShaderSimulator::runSim() {
		auto texdim = uvec2(width, height);
		dispatchShader([&](uint32_t x, uint32_t y){
			return vec4( (float)x / texdim.x, (float)y / texdim.y, 0, 1 );
		});
		writeFile();
		openFile();
	}

	void ShaderSimulator::writeFile() {
		std::vector<u8vec4> u8buf(buffer.size());
		for (int i = 0; i < buffer.size(); i++) {
			auto& p = buffer[i];
			p = clamp(p, vec4(0), vec4(1));
#if 0
			// gamma correct it
			const vec4 gamma(vec3(0.455f), 1.0f);
			p = glm::pow(p, gamma);
#endif
			u8buf[i] = u8vec4(p.x * 255, p.y * 255, p.z * 255, p.w * 255);
		}
		stbi_write_png(filename.c_str(), width, height, 4, u8buf.data(), width * 4);
	}

	void ShaderSimulator::openFile() {
		ShellExecute(0, "open", filename.c_str(), 0, 0, SW_SHOW);
	}

	void ShaderSimulator::storeTexel(int x, int y, const vec4 &col) {
		buffer[y * width + x] = col;
	}

	void ShaderSimulator::dispatchShader(const std::function<vec4(uint32_t, uint32_t)> &kernel) {
		for (auto w = 0; w < width; w++) {
			for (auto h = 0; h < height; h++) {
				storeTexel(w, h, kernel(w, h));
			}
			LOG("%u/%u", w, width)
		}
	}

}

