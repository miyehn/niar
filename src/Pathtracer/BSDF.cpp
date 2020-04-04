#include "BSDF.hpp"

#define SQ(x) (x * x)
#define EMISSIVE_THRESHOLD SQ(0.4f)

#define USE_COS_WEIGHED 1

float sample::rand01() {
  return float(rand()) / float(RAND_MAX);
}

vec2 sample::unit_square_uniform() {
  return vec2(rand01(), rand01());
}

vec2 sample::unit_square_jittered() {
	// TODO
  return vec2(0);
}

vec3 sample::hemisphere_uniform() {
  float x = 2.0f * rand01() - 1.0f; 
  float y = 2.0f * rand01() - 1.0f; 
  float z = 2.0f * rand01() - 1.0f;
  while (length(vec3(x, y, z)) > 1) {
    x = 2.0f * rand01() - 1.0f;
		y = 2.0f * rand01() - 1.0f;
		z = 2.0f * rand01() - 1.0f;
  }
  return normalize(vec3(x, y, abs(z)));
}

vec3 sample::hemisphere_cos_weighed() {
  float x = 2.0f * rand01() - 1.0f; 
  float y = 2.0f * rand01() - 1.0f; 
	while(length(vec2(x, y)) > 1) {
    x = 2.0f * rand01() - 1.0f;
		y = 2.0f * rand01() - 1.0f;
	}
	float z = sqrt(1.0f - length(vec2(x, y)) * length(vec2(x, y)));

  return vec3(x, y, z);
}

bool BSDF::is_emissive() {
	return dot(Le, Le) >= EMISSIVE_THRESHOLD;
}

vec3 Diffuse::f(vec3 wi, vec3 wo) const {
  return albedo * (1.0f / PI);
}

// uniform sampling
float Diffuse::pdf(vec3& wi, vec3 wo) const {
#if USE_COS_WEIGHED
  wi = sample::hemisphere_cos_weighed();
	return wi.z / PI;
#else
  wi = sample::hemisphere_uniform();
	return 1.0f / TWO_PI;
#endif
}
