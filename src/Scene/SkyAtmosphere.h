//
// Created by miyehn on 9/9/2022.
//
#pragma once

#include "SceneObject.hpp"
class Texture2D;

class SkyAtmosphere : public SceneObject {
public:
	// TODO: read from config file instead
	struct Parameters {
		// directional lights
		glm::vec3 sunIrradiance;
		float sunAngularRadius;
		glm::vec3 moonIrradiance;
		float moonAngularRadius;
		// rayleigh
		glm::vec3 rayleighRelScattering;
		float rayleighDensity;
		glm::vec3 rayleighExtinction;
		float pad0;
		// mie
		glm::vec3 mieRelScattering;
		float miePhaseFunctionG;
		glm::vec3 mieExtinction;
		float mieDensity;
		// ozone (absorption only; no scattering)
		glm::vec3 ozoneExtinction;
		float ozoneDensity;
		// ground
		glm::vec3 groundAlbedo;
		float pad1;
		// earth
		float bottomRadius;
		float topRadius;

		// LUT dimensions
		glm::u32vec2 transmittanceLutSize;
		glm::u32vec2 multiScatteredLutSize;
		glm::u32vec2 skyViewLutSize;

		bool operator==(const Parameters &other) {
			return false;
		}
	};
	Parameters parameters = {};
	Parameters cachedParameters = {};

	SkyAtmosphere();
	~SkyAtmosphere() override;

	void updateAndComposite(Texture2D* outSceneColor);

	//==== scene object overrides ====

	void draw_config_ui() override;

private:
	void updateAutoParameters();
	void updateLuts();

	Texture2D* transmittanceLut = nullptr;
	Texture2D* multiScatteredLut = nullptr;
	Texture2D* skyViewLut = nullptr;
	// TODO: camera volume
};

