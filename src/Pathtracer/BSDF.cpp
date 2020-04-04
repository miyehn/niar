#include "BSDF.hpp"

#define SQ(x) x * x
#define EMISSIVE_THRESHOLD SQ(0.4f)

float sample::rand01() {
  return float(rand()) / float(RAND_MAX);
}

vec2 sample::unit_square_uniform() {
  return vec2(rand01(), rand01());
}

vec2 sample::unit_square_jittered() {
  return vec2(0);
}

vec3 sample::hemisphere_uniform() {
#if 0
  float u = rand01();
  float v = rand01();

  float z = v;
  float theta = 2 * M_PI * u;
  float r = sqrt(1.0f - z * z);
  float x = r * cos(theta);
  float y = r * sin(theta);

  return vec3(x, y, z);
#else
  float x = rand01() - 0.5f; 
  float y = rand01() - 0.5f; 
  float z = rand01() - 0.5f;
  while (length(vec3(x, y, z)) > 1) {
    x = rand01() - 0.5f;
		y = rand01() - 0.5f;
		z = rand01() - 0.5f;
  }
  return normalize(vec3(x, y, abs(z)));
#endif
}

vec3 sample::hemisphere_cos_weighed() {
  return vec3(0);
}

bool BSDF::is_emissive() {
	return dot(Le, Le) >= EMISSIVE_THRESHOLD;
}

vec3 Diffuse::f(vec3 wi, vec3 wo) const {
  return albedo * (1.0f / TWO_PI);
}

// uniform sampling
float Diffuse::pdf(vec3& wi, vec3 wo) const {
  wi = sample::hemisphere_uniform();
	return 1.0f / TWO_PI;
}
