//
// Created by miyehn on 9/9/2022.
//

#include "SkyAtmosphere.h"
#include "Assets/ConfigAsset.hpp"
#include "Scene/Light.hpp"
#include "Scene/Camera.hpp"
#if GRAPHICS_DISPLAY
#include <imgui.h>
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Texture.h"
#include "Render/Materials/ComputeShader.h"
#include "SkyAtmosphereShaders.h"
#endif

SkyAtmosphere::SkyAtmosphere() {
	config = new ConfigAsset("config/skyAtmosphere.ini", true, [this](const ConfigAsset* cfg) {
		// todo [myn]: move the rest of config lookup to here? Test hot reload first
	});

#if GRAPHICS_DISPLAY

	// so the getters give correct values from the start (these are needed before deferred render creates the luts)
	config->lookupVector<int, 2>("transmittanceLutTextureDimensions", (int*)&parameters.transmittanceLutTextureDimensions);
	config->lookupVector<int, 2>("skyViewLutTextureDimensions", (int*)&parameters.skyViewLutTextureDimensions);

	//======== other properties ========

	ui_show_transform = false;
	ui_default_open = true;
#endif
}

// created here, but managed and destroyed by the scene tree
SkyAtmosphere *SkyAtmosphere::getInstance() {
	static SkyAtmosphere* instance = nullptr;
	if (!instance) {
		instance = new SkyAtmosphere();
	}
	return instance;
}

#if GRAPHICS_DISPLAY

void SkyAtmosphere::update(float elapsed) {
	SceneObject::update(elapsed);

	auto& params = parameters;

	float bottomRadius = config->lookup<float>("atmosphere.bottomRadius");

	glm::vec3 cameraPosWS = {0, 0, 0};
	cameraPosWS = Camera::Active->world_position();
	cameraPosWS.z += config->lookup<float>("viewHeightOffset");
	params.cameraPosES = cameraPosWS * 0.001f + glm::vec3(0, 0, bottomRadius);
	params.exposure = config->lookup<float>("exposure");

	// sun
	foundSun = DirectionalLight::getSun();
	if (foundSun) {
		params.dir2sun = -foundSun->getLightDirection();
	} else {
		params.dir2sun = glm::vec3(0, 0, -1);
	}
	params.sunAngularRadius = config->lookup<float>("sunAngularRadius");

	config->lookupVector<float, 2>("skyViewNumSamplesMinMax", (float*)&params.skyViewNumSamplesMinMax);
	config->lookupVector<int, 2>("transmittanceLutTextureDimensions", (int*)&params.transmittanceLutTextureDimensions);
	config->lookupVector<int, 2>("skyViewLutTextureDimensions", (int*)&params.skyViewLutTextureDimensions);

	AtmosphereProfile& atmosphere = params.atmosphere;
	{
		config->lookupVector<float, 3>("atmosphere.rayleighScattering", (float*)&atmosphere.rayleighScattering);
		atmosphere.bottomRadius = bottomRadius;

		atmosphere.mieScattering = glm::vec3(config->lookup<float>("atmosphere.mieScattering"));
		atmosphere.topRadius = config->lookup<float>("atmosphere.topRadius");

		atmosphere.mieAbsorption = glm::vec3(config->lookup<float>("atmosphere.mieAbsorption"));
		atmosphere.miePhaseG = config->lookup<float>("atmosphere.miePhaseG");

		config->lookupVector<float, 3>("atmosphere.ozoneAbsorption", (float*)&atmosphere.ozoneAbsorption);
		atmosphere.ozoneMeanHeight = config->lookup<float>("atmosphere.ozoneMeanHeight");

		config->lookupVector<float, 3>("atmosphere.groundAlbedo", (float*)&atmosphere.groundAlbedo);
		atmosphere.ozoneLayerWidth = config->lookup<float>("atmosphere.ozoneLayerWidth");
	}
}

// called by the renderer
void SkyAtmosphere::composite(
	VmaBuffer& parametersBuffer,
	DescriptorSet& descriptorSet,
	const Texture2D* transmittanceLut,
	const Texture2D* skyViewLut)
{
	// upload parameters
	parametersBuffer.writeData(&parameters, sizeof(parameters));

	{// update luts
		auto transmittanceCS = ComputeShader::getInstance<TransmittanceLutCS>();
		transmittanceCS->descriptorSetPtr = &descriptorSet;
		transmittanceCS->targetImage = transmittanceLut->resource.image;
		transmittanceCS->dispatch(
			(transmittanceLut->getWidth() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_X,
			(transmittanceLut->getHeight() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_Y, 1);

		auto skyViewCS = ComputeShader::getInstance<SkyViewLutCS>();
		skyViewCS->descriptorSetPtr = &descriptorSet;
		skyViewCS->targetImage = skyViewLut->resource.image;
		skyViewCS->dispatch(
			(skyViewLut->getWidth() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_X,
			(skyViewLut->getHeight() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_Y, 1);
	}
}

void SkyAtmosphere::drawConfigUI() {
	//ImGui::SliderFloat("Sun angular radius", &parameters.sunAngularRadius, 0, 1);
	// TODO: the rest
	if (enabled()) {
		auto yellow = ImVec4(1.0f, 0.7f, 0.1f, 1.0f);
		auto green = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
		if (foundSun) {
			ImGui::TextColored(green, "Hooked up to sun '%s'", foundSun->name.c_str());
		} else {
			ImGui::TextColored(yellow, "There's no sun in the scene.");
		}
	}
}
#endif

SkyAtmosphere::~SkyAtmosphere() {
}
