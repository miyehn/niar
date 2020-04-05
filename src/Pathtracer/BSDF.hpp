#pragma once
#include "lib.h"

namespace sample {

float rand01();

vec2 unit_square_uniform();

vec2 unit_square_jittered();

vec3 hemisphere_uniform();

vec3 hemisphere_cos_weighed();

};

struct BSDF {

	bool is_delta;

  // emission
  vec3 Le;

  // albedo
  vec3 albedo;

	virtual ~BSDF(){
		WARN("deleting BSDF!");
	}

  /* output: proportion of light going to direction wo (for each wavelength)
   * wi: negative of light incoming dir (output, sampled)
   * wo: light outgoing dir (input)
   * n: normal of the hit surface (input)
   */
  virtual vec3 f(const vec3& wi, const vec3& wo) const = 0;
	virtual vec3 sample_f(float& pdf, vec3& wi, vec3 wo) const = 0;

	bool is_emissive();
};

struct Diffuse : public BSDF {
  Diffuse(vec3 _albedo = vec3(1)) {
		is_delta = false;
    albedo = _albedo;
    Le = vec3(0.0f);
  }
  vec3 f(const vec3& wi, const vec3& wo) const;
	vec3 sample_f(float& pdf, vec3& wi, vec3 wo) const;
};

struct Mirror : public BSDF {
  Mirror() {
		is_delta = true;
    albedo = vec3(1);
    Le = vec3(0.0f);
  }
  vec3 f(const vec3& wi, const vec3& wo) const;
	vec3 sample_f(float& pdf, vec3& wi, vec3 wo) const;
};

struct Glass : public BSDF {
  // TODO
};
