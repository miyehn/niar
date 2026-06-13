//
// Created by miyehn on 9/9/2022.
//

#include "SkyAtmosphere.h"
#include "Assets/ConfigAsset.hpp"
#include "Scene/Light.hpp"
#include "Scene/Camera.hpp"
#if GRAPHICS_DISPLAY
#include <imgui.h>
#endif

SkyAtmosphere::SkyAtmosphere() {
	new ConfigAsset("config/skyAtmosphere.ini", true, [this](const ConfigAsset* cfg) {
		set_enabled(cfg->lookup<int>("enabled"));

		cfg->lookupVector<int, 2>("transmittanceLutTextureDimensions", (int*)&parameters.transmittanceLutTextureDimensions);
		cfg->lookupVector<int, 2>("skyViewLutTextureDimensions", (int*)&parameters.skyViewLutTextureDimensions);

		parameters.exposure = cfg->lookup<float>("exposure");
		parameters.sunAngularRadius = cfg->lookup<float>("sunAngularRadius");
		parameters.viewHeightOffset = cfg->lookup<float>("viewHeightOffset");

		cfg->lookupVector<float, 2>("skyViewNumSamplesMinMax", (float*)&parameters.skyViewNumSamplesMinMax);
		cfg->lookupVector<int, 2>("transmittanceLutTextureDimensions", (int*)&parameters.transmittanceLutTextureDimensions);
		cfg->lookupVector<int, 2>("skyViewLutTextureDimensions", (int*)&parameters.skyViewLutTextureDimensions);

		AtmosphereProfile& atmosphere = parameters.atmosphere;
		{
			cfg->lookupVector<float, 3>("atmosphere.rayleighScattering", (float*)&atmosphere.rayleighScattering);

			atmosphere.mieScattering = glm::vec3(cfg->lookup<float>("atmosphere.mieScattering"));
			atmosphere.topRadius = cfg->lookup<float>("atmosphere.topRadius");

			atmosphere.mieAbsorption = glm::vec3(cfg->lookup<float>("atmosphere.mieAbsorption"));
			atmosphere.miePhaseG = cfg->lookup<float>("atmosphere.miePhaseG");

			cfg->lookupVector<float, 3>("atmosphere.ozoneAbsorption", (float*)&atmosphere.ozoneAbsorption);
			atmosphere.ozoneMeanHeight = cfg->lookup<float>("atmosphere.ozoneMeanHeight");

			cfg->lookupVector<float, 3>("atmosphere.groundAlbedo", (float*)&atmosphere.groundAlbedo);
			atmosphere.ozoneLayerWidth = cfg->lookup<float>("atmosphere.ozoneLayerWidth");

			atmosphere.bottomRadius = cfg->lookup<float>("atmosphere.bottomRadius");
		}
	});

#if GRAPHICS_DISPLAY

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

	glm::vec3 cameraPosWS = Camera::Active->world_position();
	cameraPosWS.z += parameters.viewHeightOffset;
	parameters.cameraPosES = cameraPosWS * 0.001f + glm::vec3(0, 0, parameters.atmosphere.bottomRadius);

	// sun
	foundSun = DirectionalLight::getSun();
	if (foundSun) {
		parameters.dir2sun = -foundSun->getLightDirection();
	} else {
		parameters.dir2sun = glm::vec3(0, 0, -1);
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
#else
void SkyAtmosphere::find_sun()
{
	foundSun = DirectionalLight::getSun();
	if (foundSun) {
		parameters.dir2sun = -foundSun->getLightDirection();
	} else {
		parameters.dir2sun = glm::vec3(0, 0, -1);
	}
}
#endif

SkyAtmosphere::~SkyAtmosphere() {
}
