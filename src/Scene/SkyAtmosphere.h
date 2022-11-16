//
// Created by miyehn on 9/9/2022.
//
#pragma once

#include "SceneObject.hpp"
#include "Render/Vulkan/Buffer.h"

class Texture2D;
class ConfigAsset;

class SkyAtmosphere : public SceneObject {
public:

	SkyAtmosphere();
	~SkyAtmosphere() override;

	void updateAndComposite(Texture2D* outSceneColor);

	//==== scene object overrides ====

	void draw_config_ui() override;

private:
	void updateLuts();

	struct AtmosphereProfile {

		glm::vec3 rayleighScattering;
		float bottomRadius;

		glm::vec3 mieScattering;
		float topRadius;

		glm::vec3 mieAbsorption;
		float miePhaseG;

		glm::vec3 ozoneAbsorption;
		float ozoneMeanHeight;

		glm::vec3 groundAlbedo; // not used for now
		float ozoneLayerWidth;
	};

	struct Parameters {

		AtmosphereProfile atmosphere;

		glm::vec3 cameraPosES;
		float exposure;

		glm::vec3 dir2sun;
		float sunAngularRadius;

		glm::vec2 skyViewNumSamplesMinMax;

		glm::uvec2 transmittanceLutTextureDimensions;
		glm::uvec2 skyViewLutTextureDimensions;

		bool equals(const Parameters& other) const { return false; }
	};

	//Parameters parameters = {};

	Parameters getParameters();

	Parameters cachedParameters = {};
	VmaBuffer parametersBuffer;

	Texture2D* transmittanceLut = nullptr;
	Texture2D* multiScatteredLut = nullptr;
	Texture2D* skyViewLut = nullptr;
	// TODO: camera volume

	ConfigAsset* config = nullptr;
};

