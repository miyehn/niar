#pragma once

#include <glm/glm.hpp>

namespace myn::sample {

	// uniform random number in [0, 1), 24 bits of it. Safe to call from any thread.
	float rand01();

	// Changes where the streams of rand01() start. Call it before the first rand01(); without it they always start
	// at the same place, so runs are reproducible.
	void seed_rand01(uint32_t seed);

	glm::vec2 unit_square_uniform();

	glm::vec2 unit_disc_uniform();

	glm::vec3 hemisphere_uniform();

	glm::vec3 hemisphere_cos_weighed();

	namespace tex {

		glm::vec3 tex2D_float3_point(const float* texels_raw, uint32_t width, uint32_t height, glm::ivec2 coord);

		glm::vec3 tex2D_float3_bilinear(const float* texels_raw, uint32_t width, uint32_t height, glm::vec2 uv);

		glm::vec3 longlatmap_float3(const float* texels_raw, uint32_t width, uint32_t height, const glm::vec3 &dir);
	}

}