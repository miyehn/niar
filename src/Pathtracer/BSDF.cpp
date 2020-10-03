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

bool BSDF::compute_is_emissive() const {
	return dot(Le, Le) >= EMISSIVE_THRESHOLD;
}

vec3 Diffuse::f(const vec3& wi, const vec3& wo, bool debug) const {
	return albedo * ONE_OVER_PI;
}

vec3 Diffuse::sample_f(float& pdf, vec3& wi, vec3 wo, bool debug) const {
#if USE_COS_WEIGHED
	wi = sample::hemisphere_cos_weighed();
	pdf = wi.z * ONE_OVER_PI;
#else
	wi = sample::hemisphere_uniform();
	pdf = ONE_OVER_TWO_PI;
#endif
	return f(wi, wo, debug);
}

vec3 Mirror::f(const vec3& wi, const vec3& wo, bool debug) const {
	return vec3(0.0f);
}

vec3 Mirror::sample_f(float& pdf, vec3& wi, vec3 wo, bool debug) const {
	wi = -wo;
	wi.z = wo.z;
	pdf = 1.0f;
	return albedo * (1.0f / wi.z);
}

vec3 Glass::f(const vec3& wi, const vec3& wo, bool debug) const {
	return vec3(0.0f);
}

vec3 Glass::sample_f(float& pdf, vec3& wi, vec3 wo, bool debug) const {
	// will treat wo as in direction and wi as out direction, since it's bidirectional

	bool trace_out = wo.z < 0; // the direction we're going to trace is into the medium

	// IOR, assume container medium is air
	float ni = trace_out ? IOR : 1.0f;
	float nt = trace_out ? 1.0f : IOR;
	if (debug) LOGF("ni: %f, nt: %F", ni, nt);

	float cos_theta_i = abs(wo.z);
	float sin_theta_i = sqrt(1.0f - pow(cos_theta_i, 2));

	// opt out early for total internal reflection
	float cos_sq_theta_t = 1.0f - pow(ni/nt, 2) * (1.0f - wo.z * wo.z);
	bool TIR = cos_sq_theta_t < 0;
	if (TIR) { // total internal reflection
		if (debug) LOG("TIR");
		wi = -wo;
		wi.z = wo.z;
		pdf = 1.0f;
		return albedo * (1.0f / abs(wi.z));
	}
	
	// then use angles to find reflectance
	float r0 = pow((ni-nt) / (ni+nt), 2);
	float reflectance = r0 + (1.0f - r0) * pow(1.0f - cos_theta_i, 5);
	
	// flip a biased coin to decide whether to reflect or refract
	bool reflect = sample::rand01() <= reflectance;
	if (reflect) {
		if (debug) LOG("reflect");
		wi = -wo;
		wi.z = wo.z;
		pdf = reflectance;
		return albedo * (reflectance / cos_theta_i);

	} else { // refract
		if (debug) LOG("refract")
		// remember we treat wi as "out direction"
		float cos_theta_t = sqrt(cos_sq_theta_t);
		float sin_theta_t = sqrt(1.0f - cos_sq_theta_t);

		float xy_norm_factor = sin_theta_t / sin_theta_i;
		wi = vec3(-wo.x * xy_norm_factor,
							-wo.y * xy_norm_factor,
							trace_out ? cos_theta_t : -cos_theta_t);

		pdf = 1.0f - reflectance;
		// now compute f...
		return albedo * ((nt*nt) / (ni*ni)) * (1.0f - reflectance) * (1.0f / cos_theta_i);
	}
}
