//
// Created by miyehn on 9/9/2022.
//
#pragma once

#include "Scene/SceneObject.hpp"

class ConfigAsset;
class DirectionalLight;

class SkyAtmosphere : public SceneObject {
public:

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

		float viewHeightOffset;
	};

	~SkyAtmosphere() override;

	static SkyAtmosphere* getInstance();

#if GRAPHICS_DISPLAY
	enum BindingSlot {
		// 0: uniform buffer
		Slot_Parameters = 0,
		// 1-7: rw images
		Slot_TransmittanceLutRW = 1,
		Slot_SkyViewLutRW = 2,
		// ...
		// 8- : sampled textures
		Slot_TransmittanceLutR = 8,
		Slot_SkyViewLutR = 9,
	};

	glm::uvec2 getTransmittanceLutDimensions() const { return parameters.transmittanceLutTextureDimensions; };
	glm::uvec2 getSkyViewLutDimensions() const { return parameters.skyViewLutTextureDimensions; };

	//==== scene object overrides ====

	void update(float elapsed) override;

	void drawConfigUI() override;
#else
	void find_sun(); // called by asz to hook up the sun
#endif

	[[nodiscard]] const Parameters& getParameters() const { return parameters; }
	[[nodiscard]] DirectionalLight *const getSun() const { return foundSun; }

private:

	SkyAtmosphere();

	Parameters parameters = {};

	DirectionalLight* foundSun = nullptr;

};
