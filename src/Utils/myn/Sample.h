#pragma once

#include <glm/glm.hpp>

namespace myn::sample {

	float rand01();

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