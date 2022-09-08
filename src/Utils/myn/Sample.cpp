#include "Sample.h"
#include "Misc.h"

namespace myn {

using namespace glm;

float sample::rand01() {
	return float(rand()) / float(RAND_MAX);
}

vec2 sample::unit_square_uniform() {
	return vec2(rand01(), rand01());
}

vec2 sample::unit_disc_uniform() {
	float x = rand01() - 0.5f;
	float y = rand01() - 0.5f;
	while(length(vec2(x, y)) > 0.5f) {
		x = rand01() - 0.5f;
		y = rand01() - 0.5f;
	}
	return vec2(x, y) * 2.0f;
}

vec3 sample::hemisphere_uniform() {
	float x = rand01() - 0.5f;
	float y = rand01() - 0.5f;
	float z = rand01() - 0.5f;
	while (length(vec3(x, y, z)) > 0.5f) {
		x = rand01() - 0.5f;
		y = rand01() - 0.5f;
		z = rand01() - 0.5f;
	}
	return normalize(vec3(x, y, abs(z)));
}

// ehh.... significantly slower than uniform sampling..
// see: https://bobobobo.wordpress.com/2012/06/11/cosine-weighted-hemisphere-sampling/
vec3 sample::hemisphere_cos_weighed() {
#if 1
	float x = rand01() - 0.5f;
	float y = rand01() - 0.5f;
	while(length(vec2(x, y)) > 0.5f) {
		x = rand01() - 0.5f;
		y = rand01() - 0.5f;
	}
	x *= 2.0f; y *= 2.0f;
	float l = length(vec2(x, y));
	float z = sqrt(1.0f - l * l);

	return vec3(x, y, z);
#else // too slow....
	float r1 = rand01();
	float r2 = rand01();
	float phi = r1 * TWO_PI;
	float cos_theta = sqrt(r2);
	float sin_theta = sqrt(1.0f - r2);
	float cos_phi = cos(phi);
	float sin_phi = sqrt(1.0f - pow(cos_phi, 2));
	return vec3(cos_phi * sin_theta, sin_phi * sin_theta, cos_theta);
#endif
}
vec3 sample::tex::tex2D_float3_point(const float* texels_raw, uint32_t width, uint32_t height, glm::ivec2 coord) {
	uint32_t i = (width * coord.y + coord.x) * 3;
	return {
		texels_raw[i],
		texels_raw[i + 1],
		texels_raw[i + 2]
	};
}

vec3 sample::tex::tex2D_float3_bilinear(const float* texels_raw, uint32_t w, uint32_t h, glm::vec2 uv) {
	vec2 coords = vec2(w * uv.x, h * uv.y);
	vec2 deci = glm::fract(coords);
	ivec2 c00(glm::floor(coords.x), glm::floor(coords.y));
	ivec2 c10(c00.x + 1, c00.y);
	ivec2 c01(c00.x, c00.y + 1);
	ivec2 c11(c00.x + 1, c00.y + 1);
	vec3 v00 = tex2D_float3_point(texels_raw, w, h, c00);
	vec3 v10 = tex2D_float3_point(texels_raw, w, h, c10);
	vec3 v01 = tex2D_float3_point(texels_raw, w, h, c01);
	vec3 v11 = tex2D_float3_point(texels_raw, w, h, c11);
	vec3 y0 = v00 * (1.0f - deci.x) + v10 * deci.x;
	vec3 y1 = v01 * (1.0f - deci.x) + v11 * deci.x;
	return y0 * (1.0f - deci.y) + y1 * deci.y;
}

vec3 sample::tex::longlatmap_float3(const float* texels_raw, uint32_t width, uint32_t height, const glm::vec3 &dir) {
	float phi = atan2(dir.y, dir.x);
	float theta = asin(dir.z);

	// need to invert them both
	vec2 uv = vec2(-phi * ONE_OVER_TWO_PI + 0.5f, -theta * ONE_OVER_PI + 0.5f);

	return sample::tex::tex2D_float3_bilinear(texels_raw, width, height, uv);
}

}
