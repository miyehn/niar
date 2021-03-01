#pragma once
#include "lib.h"

namespace sample {

float rand01();

vec2 unit_square_uniform();

vec2 unit_disc_uniform();

vec3 hemisphere_uniform();

vec3 hemisphere_cos_weighed();

};

struct BSDF {

	enum Type {
		Diffuse,
		Mirror,
		Glass
	};

	Type type;

	bool is_delta;
	bool is_emissive;

	// Le
	vec3 get_emission() const { return Le; }
	void set_emission(const vec3& _Le) {
		Le = _Le;
		is_emissive = compute_is_emissive();
	}

	// albedo
	vec3 albedo;

	virtual ~BSDF(){
	}

	/* output: proportion of light going to direction wo (for each wavelength)
	 * wi: negative of light incoming dir (output, sampled)
	 * wo: light outgoing dir (input)
	 * n: normal of the hit surface (input)
	 */
	virtual vec3 f(const vec3& wi, const vec3& wo, bool debug = false) const = 0;
	virtual vec3 sample_f(float& pdf, vec3& wi, vec3 wo, bool debug = false) const = 0;

protected:
	// emission
	vec3 Le;
	bool compute_is_emissive() const;

};

struct Diffuse : public BSDF {
	Diffuse(vec3 _albedo = vec3(1)) {
		type = BSDF::Diffuse;
		is_delta = false;
		albedo = _albedo;
		set_emission(vec3(0));
	}
	vec3 f(const vec3& wi, const vec3& wo, bool debug) const;
	vec3 sample_f(float& pdf, vec3& wi, vec3 wo, bool debug) const;
};

struct Mirror : public BSDF {
	Mirror() {
		type = BSDF::Mirror;
		is_delta = true;
		albedo = vec3(1);
		set_emission(vec3(0));
	}
	vec3 f(const vec3& wi, const vec3& wo, bool debug) const;
	vec3 sample_f(float& pdf, vec3& wi, vec3 wo, bool debug) const;
};

struct Glass : public BSDF {
	float IOR;
	Glass(float _IOR = 1.52f) : IOR(_IOR) {
		type = BSDF::Glass;
		is_delta = true;
		albedo = vec3(1);
		set_emission(vec3(0));
	}
	vec3 f(const vec3& wi, const vec3& wo, bool debug) const;
	vec3 sample_f(float& pdf, vec3& wi, vec3 wo, bool debug) const;
};
