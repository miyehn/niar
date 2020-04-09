#include "BSDF.hpp"

#define SQ(x) (x * x)
#define EMISSIVE_THRESHOLD SQ(0.4f)

#define USE_COS_WEIGHED 0

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

bool BSDF::is_emissive() const {
	return dot(Le, Le) >= EMISSIVE_THRESHOLD;
}

vec3 Diffuse::f(const vec3& wi, const vec3& wo) const {
  return albedo * (1.0f / PI);
}

// uniform sampling
vec3 Diffuse::sample_f(float& pdf, vec3& wi, vec3 wo) const {
#if USE_COS_WEIGHED
  wi = sample::hemisphere_cos_weighed();
	pdf = wi.z / PI;
#else
  wi = sample::hemisphere_uniform();
	pdf = 1.0f / TWO_PI;
#endif
	return f(wi, wo);
}

vec3 Mirror::f(const vec3& wi, const vec3& wo) const {
	return vec3(0.0f);
}

vec3 Mirror::sample_f(float& pdf, vec3& wi, vec3 wo) const {
	wi = -wo;
	wi.z = wo.z;
	pdf = 1.0f;
	return albedo * (1.0f / wi.z);
}

vec3 Glass::f(const vec3& wi, const vec3& wo) const {
	return vec3(0.0f);
}

vec3 Glass::sample_f(float& pdf, vec3& wi, vec3 wo) const {
	// will treat wo as in direction and wi as out direction, since it's bidirectional

	bool trace_out = wo.z < 0; // the direction we're going to trace is into the medium

	// IOR, assume container medium is air
	float ni = trace_out ? IOR : 1.0f;
	float nt = trace_out ? 1.0f : IOR;
	float theta_i = acos(abs(wo.z)); // ok

	// find the other angle (and opt out early for total internal reflection)
	float cos_sq_theta_t = 1.0f - (ni/nt) * (ni/nt) * (1.0f - wo.z * wo.z);
	bool TIR = cos_sq_theta_t < 0;
	if (TIR) { // total internal reflection
		wi = -wo;
		wi.z = wo.z;
		pdf = 1.0f;
		return -albedo * (1.0f / wi.z); // TODO!!!
	}
	float theta_t = acos(sqrt(cos_sq_theta_t));
	
	// then use angles to find reflectance
	float cos_theta_i = cos(theta_i);
	float cos_theta_t = cos(theta_t);
	float r_para = (nt * cos_theta_i - ni * cos_theta_t) / (nt * cos_theta_i + ni * cos_theta_t);
	float r_perp = (ni * cos_theta_i - nt * cos_theta_t) / (ni * cos_theta_i + nt * cos_theta_t);
	float reflectance = 0.5f * (r_para * r_para + r_perp * r_perp);
	
	// flip a biased coin to decide whether to reflect or refract
	bool reflect = sample::rand01() <= reflectance;
	if (reflect) {
		wi = -wo;
		wi.z = wo.z;
		pdf = reflectance;
		return -albedo * (reflectance / wi.z); // TODO!!!

	} else { // refract
		// remember we treat wi as "out direction"
		wi = normalize(vec3(-wo.x, -wo.y, trace_out ? cos_theta_t : -cos_theta_t));
		pdf = 1.0f - reflectance;
		// now compute f...
		return albedo * ((nt*nt) / (ni*ni)) * (1.0f - reflectance) * (1.0f / abs(wi.z));
	}
}
