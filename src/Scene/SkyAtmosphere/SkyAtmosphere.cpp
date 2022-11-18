//
// Created by miyehn on 9/9/2022.
//

#include "imgui/imgui.h"
#include "SkyAtmosphere.h"
#include "Assets/ConfigAsset.hpp"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Texture.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Scene/Camera.hpp"
#include "Render/Materials/ComputeShader.h"
#include "SkyAtmosphereShaders.h"

SkyAtmosphere::SkyAtmosphere() {
	config = new ConfigAsset("config/skyAtmosphere.ini", true, [this](const ConfigAsset* cfg){});
	memset(&cachedParameters, 0, sizeof(Parameters));

	Parameters initialParams = getParameters();

	// create transmittance lut
	ImageCreator transmittanceLutCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{initialParams.transmittanceLutTextureDimensions.x, initialParams.transmittanceLutTextureDimensions.y, 1},
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"Transmittance LUT");
	transmittanceLut = new Texture2D(transmittanceLutCreator);

	// create sky view lut
	ImageCreator skyViewLutCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{initialParams.skyViewLutTextureDimensions.x, initialParams.skyViewLutTextureDimensions.y, 1},
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"Sky View LUT");
	skyViewLut = new Texture2D(skyViewLutCreator);

	// create uniform buffer
	parametersBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		sizeof(Parameters),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		"Sky atmosphere params buffer"
	});

	// create shared descriptor set
	DescriptorSetLayout setLayout{};
	setLayout.addBinding(Slot_Parameters, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	setLayout.addBinding(Slot_TransmittanceLutRW, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	setLayout.addBinding(Slot_SkyViewLutRW, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	setLayout.addBinding(Slot_TransmittanceLutR, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	setLayout.addBinding(Slot_SkyViewLutR, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	dynamicSet = DescriptorSet(setLayout);

	//======== other properties ========

	ui_show_transform = false;
	ui_default_open = true;
}

SkyAtmosphere::Parameters SkyAtmosphere::getParameters() {
	Parameters params{};

	float bottomRadius = config->lookup<float>("atmosphere.bottomRadius");

	glm::vec3 cameraPosWS = {0, 0, 0};
	if (Camera::Active) {
		cameraPosWS = Camera::Active->world_position();
	}
	cameraPosWS.z += config->lookup<float>("viewHeightOffset");
	params.cameraPosES = cameraPosWS * 0.001f + glm::vec3(0, 0, bottomRadius);
	params.exposure = config->lookup<float>("exposure");

	params.dir2sun = glm::normalize(glm::vec3(1, 0, 0.5f)); // todo
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

	return params;
}

// called by the renderer
void SkyAtmosphere::updateAndComposite(Texture2D *outSceneColor) {
	auto parameters = getParameters();
	if (!parameters.equals(cachedParameters)) {
		parametersBuffer.writeData(&parameters);
		updateLuts();
		cachedParameters = parameters;
	}
	// TODO: composite onto outSceneColor
}

void SkyAtmosphere::updateLuts() {

	dynamicSet.pointToBuffer(parametersBuffer, Slot_Parameters, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	dynamicSet.pointToRWImageView(transmittanceLut->imageView, Slot_TransmittanceLutRW);
	dynamicSet.pointToRWImageView(skyViewLut->imageView, Slot_SkyViewLutRW);

	auto transmittanceCS = ComputeShader::getInstance<TransmittanceLutCS>();
	transmittanceCS->descriptorSetPtr = &dynamicSet;
	transmittanceCS->targetImage = transmittanceLut->resource.image;
	transmittanceCS->dispatch(
		(transmittanceLut->getWidth() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_X,
		(transmittanceLut->getHeight() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_Y, 1);

	dynamicSet.pointToImageView(transmittanceLut->imageView, Slot_TransmittanceLutR, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	auto skyViewCS = ComputeShader::getInstance<SkyViewLutCS>();
	skyViewCS->descriptorSetPtr = &dynamicSet;
	skyViewCS->targetImage = skyViewLut->resource.image;
	skyViewCS->dispatch(
		(skyViewLut->getWidth() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_X,
		(skyViewLut->getHeight() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_Y, 1);
}

SkyAtmosphere::~SkyAtmosphere() {
	delete transmittanceLut;
	delete skyViewLut;
	parametersBuffer.release();
}

void SkyAtmosphere::draw_config_ui() {
	//ImGui::SliderFloat("Sun angular radius", &parameters.sunAngularRadius, 0, 1);
	// TODO: the rest
}
