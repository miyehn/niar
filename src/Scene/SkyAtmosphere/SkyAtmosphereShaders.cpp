//
// Created by miyehn on 11/17/2022.
//

#include "SkyAtmosphereShaders.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/VulkanUtils.h"

void TransmittanceLutCS::dispatch(int groupCountX, int groupCountY, int groupCountZ) {
	initializePipeline();

	// singleton instance
	Vulkan::Instance->immediateSubmit(
		[&](VkCommandBuffer cmdbuf) {
			SCOPED_DRAW_EVENT(cmdbuf, "Dispatch TransmittanceLutCS")
			vk::insertImageBarrier(
				cmdbuf,
				targetImage,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL
			);

			vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			descriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipelineLayout);
			vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);

			vk::insertImageBarrier(
				cmdbuf,
				targetImage,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		});
}

void SkyViewLutCS::dispatch(int groupCountX, int groupCountY, int groupCountZ) {
	initializePipeline();

	Vulkan::Instance->immediateSubmit(
		[&](VkCommandBuffer cmdbuf) {
			SCOPED_DRAW_EVENT(cmdbuf, "Dispatch SkyViewLutCS")
			vk::insertImageBarrier(
				cmdbuf,
				targetImage,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL
			);

			vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			descriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipelineLayout);
			vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);

			vk::insertImageBarrier(
				cmdbuf,
				targetImage,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		});
}