//
// Created by miyehn on 11/10/2022.
//

#include <thread>
#include "ShaderSimulator.h"
#include "Log.h"

namespace myn {
using namespace glm;

ShaderSimulator::ShaderSimulator(int width, int height)
	: outputTexture(width, height) {}

void ShaderSimulator::runSim() {
	auto texdim = uvec2(outputTexture.getWidth(), outputTexture.getHeight());
	dispatchShader([&](uint32_t x, uint32_t y) {
		return vec4((float) x / texdim.x, (float) y / texdim.y, 0, 1);
	});
}

void ShaderSimulator::dispatchShader(const std::function<vec4(uint32_t, uint32_t)> &kernel) {

#define SHADERSIM_MULTITHREADED 1
#define SHADERSIM_NUM_THREADS 8

#if SHADERSIM_MULTITHREADED
	std::thread threads[SHADERSIM_NUM_THREADS];
	uint32_t rowsPerThread = (outputTexture.getHeight() + SHADERSIM_NUM_THREADS - 1) / SHADERSIM_NUM_THREADS;
	for (int tid = 0; tid < SHADERSIM_NUM_THREADS; tid++) {
		auto startRow = rowsPerThread * tid;
		auto endRow = glm::min(rowsPerThread * (tid + 1), uint32_t(outputTexture.getHeight()));
		threads[tid] = std::thread([this, startRow, endRow, kernel](int tid){
			for (auto h = startRow; h < endRow; h++) {
				for (auto w = 0; w < outputTexture.getWidth(); w++) {
					outputTexture.storeTexel(w, h, kernel(w, h));
				}
			}
		}, tid);
	}
	for (int tid = 0; tid < SHADERSIM_NUM_THREADS; tid++) {
		threads[tid].join();
	}
#else
	for (auto h = 0; h < outputTexture.getHeight(); h++) {
		for (auto w = 0; w < outputTexture.getWidth(); w++) {
			outputTexture.storeTexel(w, h, kernel(w, h));
		}
	}
#endif
}

} // namespace myn

