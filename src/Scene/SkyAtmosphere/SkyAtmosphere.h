//
// Created by miyehn on 9/9/2022.
//
#pragma once

#include "Scene/SceneObject.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;
class ConfigAsset;
class DirectionalLight;

class SkyAtmosphere : public SceneObject {

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

public:

	SkyAtmosphere();
	~SkyAtmosphere() override;

	static SkyAtmosphere* getInstance();

	void updateAndComposite();

	//==== scene object overrides ====

	void drawConfigUI() override;

	DescriptorSet& getDescriptorSet() { return this->enabled() ? descriptorSet : dummyDescriptorSet; }

	[[nodiscard]] DirectionalLight *const getSun() const { return foundSun; }

private:

	// actual resources
	DescriptorSet descriptorSet;

	// dummy descriptor set for when sky is disabled
	DescriptorSet dummyDescriptorSet;

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

	Parameters getParameters();

	Parameters cachedParameters = {};

	ConfigAsset* config = nullptr;

	DirectionalLight* foundSun = nullptr;

	// gpu resources

	VmaBuffer parametersBuffer;

	Texture2D* transmittanceLut = nullptr;
	Texture2D* skyViewLut = nullptr;
	Texture2D* multiScatteredLut = nullptr;
	// TODO: camera volume
};

