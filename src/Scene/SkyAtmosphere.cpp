//
// Created by miyehn on 9/9/2022.
//

#include <imgui.h>
#include "SkyAtmosphere.h"
#include "Assets/ConfigAsset.hpp"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Texture.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Camera.hpp"

// checklist: https://community.khronos.org/t/drawing-to-image-from-compute-shader-example/7116/2
class TransmittanceLutCS {
public:
	static void dispatch(const VmaBuffer& paramsBuffer, Texture2D* targetImage) {
		// singleton instance
		static TransmittanceLutCS* instance = nullptr;
		if (!instance) {
			instance = new TransmittanceLutCS();
			Vulkan::Instance->destructionQueue.emplace_back([](){ delete instance; });
		}
		Vulkan::Instance->immediateSubmit(
			[&](VkCommandBuffer cmdbuf)
			{
				SCOPED_DRAW_EVENT(cmdbuf, "Dispatch TransmittanceLutCS")
				vk::insertImageBarrier(
					cmdbuf,
					targetImage->resource.image,
					{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL
					);

				instance->dynamicSet.pointToBuffer(paramsBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
				instance->dynamicSet.pointToRWImageView(targetImage->imageView, 1);

				vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, instance->pipeline);
				instance->dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_DYNAMIC, instance->pipelineLayout);
				vkCmdDispatch(cmdbuf, 256/8, 64/8, 1);

				vk::insertImageBarrier(
					cmdbuf,
					targetImage->resource.image,
					{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);
			});
	}
private:
	explicit TransmittanceLutCS() {
		ComputePipelineBuilder pipelineBuilder{};
		// shader
		pipelineBuilder.shaderPath = "spirv/sky_transmittance_lut.comp.spv";
		// descriptor sets
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		dynamicSet = DescriptorSet(dynamicSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	DescriptorSet dynamicSet;
};

SkyAtmosphere::SkyAtmosphere() {
	config = new ConfigAsset("config/skyAtmosphere.ini", true, [this](const ConfigAsset* cfg){});
	memset(&cachedParameters, 0, sizeof(Parameters));

	Parameters initialParams = getParameters();

	ImageCreator transmittanceLutCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{initialParams.transmittanceLutTextureDimensions.x, initialParams.transmittanceLutTextureDimensions.y, 1},
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"Transmittance LUT");
	transmittanceLut = new Texture2D(transmittanceLutCreator);

	// create the buffer
	parametersBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		sizeof(Parameters),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		"Sky atmosphere params buffer"
	});

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
	// TODO
	TransmittanceLutCS::dispatch(parametersBuffer, transmittanceLut);
}

SkyAtmosphere::~SkyAtmosphere() {
	delete transmittanceLut;
	parametersBuffer.release();
}

void SkyAtmosphere::draw_config_ui() {
	//ImGui::SliderFloat("Sun angular radius", &parameters.sunAngularRadius, 0, 1);
	// TODO: the rest
}
