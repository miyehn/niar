//
// Created by miyehn on 9/9/2022.
//

#include <imgui.h>
#include "SkyAtmosphere.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Texture.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/VulkanUtils.h"

// checklist: https://community.khronos.org/t/drawing-to-image-from-compute-shader-example/7116/2
class TransmittanceLutCS {
public:
	static void dispatch(Texture2D* targetImage) {
		// singleton instance
		static TransmittanceLutCS* instance = nullptr;
		if (!instance) {
			instance = new TransmittanceLutCS(targetImage);
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

				instance->dynamicSet.pointToRWImageView(targetImage->imageView, 0);

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
	explicit TransmittanceLutCS(Texture2D* targetImage) {
		ComputePipelineBuilder pipelineBuilder{};
		// shader
		pipelineBuilder.shaderPath = "spirv/sky_transmittance_lut.comp.spv";
		// descriptor sets
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		dynamicSet = DescriptorSet(dynamicSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	DescriptorSet dynamicSet;
};

SkyAtmosphere::SkyAtmosphere() {
	memset(&parameters, 0, sizeof(Parameters));
	memset(&cachedParameters, 0, sizeof(Parameters));

	parameters.transmittanceLutSize = {256, 64};
	parameters.multiScatteredLutSize = {32, 32};
	parameters.skyViewLutSize = {192, 108};
	// TODO: init other parameters

	ImageCreator transmittanceLutCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{parameters.transmittanceLutSize.x, parameters.transmittanceLutSize.y, 1},
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"Transmittance LUT");
	transmittanceLut = new Texture2D(transmittanceLutCreator);

	//======== other properties ========

	ui_show_transform = false;
	ui_default_open = true;
}

// called by the renderer
void SkyAtmosphere::updateAndComposite(Texture2D *outSceneColor) {
	updateAutoParameters();
	if (!(parameters == cachedParameters)) {
		updateLuts();
		cachedParameters = parameters;
	}
	// TODO: composite onto outSceneColor
}

void SkyAtmosphere::updateAutoParameters() {
	// TODO
}

void SkyAtmosphere::updateLuts() {
	// TODO
	//TransmittanceLutCS::dispatch(transmittanceLut);
}

SkyAtmosphere::~SkyAtmosphere() {
	delete transmittanceLut;
}

void SkyAtmosphere::draw_config_ui() {
	ImGui::SliderFloat("Sun angular radius", &parameters.sunAngularRadius, 0, 1);
	// TODO: the rest
}
