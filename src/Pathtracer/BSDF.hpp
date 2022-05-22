#pragma once
#include <glm/glm.hpp>

namespace sample {

float rand01();

glm::vec2 unit_square_uniform();

glm::vec2 unit_disc_uniform();

glm::vec3 hemisphere_uniform();

glm::vec3 hemisphere_cos_weighed();

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
	glm::vec3 get_emission() const { return Le; }
	void set_emission(const glm::vec3& _Le) {
		Le = _Le;
		is_emissive = compute_is_emissive();
	}

	// albedo
	glm::vec3 albedo;

	virtual ~BSDF(){
	}

	/* output: proportion of light going to direction wo (for each wavelength)
	 * wi: negative of light incoming dir (output, sampled)
	 * wo: light outgoing dir (input)
	 * n: normal of the hit surface (input)
	 */
	virtual glm::vec3 f(const glm::vec3& wi, const glm::vec3& wo, bool debug = false) const = 0;
	virtual glm::vec3 sample_f(float& pdf, glm::vec3& wi, glm::vec3 wo, bool debug = false) const = 0;

	// asset management
	uint32_t asset_version = 0;

protected:
	// emission
	glm::vec3 Le;
	bool compute_is_emissive() const;

};

struct Diffuse : public BSDF {
	Diffuse(glm::vec3 _albedo = glm::vec3(1)) {
		type = BSDF::Diffuse;
		is_delta = false;
		albedo = _albedo;
		set_emission(glm::vec3(0));
	}
	glm::vec3 f(const glm::vec3& wi, const glm::vec3& wo, bool debug) const override;
	glm::vec3 sample_f(float& pdf, glm::vec3& wi, glm::vec3 wo, bool debug) const override;
};

struct Mirror : public BSDF {
	Mirror() {
		type = BSDF::Mirror;
		is_delta = true;
		albedo = glm::vec3(1);
		set_emission(glm::vec3(0));
	}
	glm::vec3 f(const glm::vec3& wi, const glm::vec3& wo, bool debug) const override;
	glm::vec3 sample_f(float& pdf, glm::vec3& wi, glm::vec3 wo, bool debug) const override;
};

struct Glass : public BSDF {
	float IOR;
	Glass(float _IOR = 1.52f) : IOR(_IOR) {
		type = BSDF::Glass;
		is_delta = true;
		albedo = glm::vec3(1);
		set_emission(glm::vec3(0));
	}
	glm::vec3 f(const glm::vec3& wi, const glm::vec3& wo, bool debug) const override;
	glm::vec3 sample_f(float& pdf, glm::vec3& wi, glm::vec3 wo, bool debug) const override;
};
