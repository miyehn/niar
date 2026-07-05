#include "TAA.h"
#include "Assets/ConfigAsset.hpp"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Texture.h"
#include "Render/Materials/ComputeShader.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"

#include <string>

#define TAA_GROUPSIZE_X 8
#define TAA_GROUPSIZE_Y 8

namespace
{

constexpr uint32_t Slot_SceneColor = 0;
constexpr uint32_t Slot_ReadHistory = 1;
constexpr uint32_t Slot_Resolved = 2;
constexpr uint32_t Slot_WriteHistory = 3;
constexpr uint32_t Slot_MotionVectors = 4;

struct TaaPushData {
	float historyWeight;
};

class TaaResolveCS : public ComputeShader
{
public:
	const DescriptorSet* taaDescriptorSetPtr = nullptr;
	TaaPushData pushData = {0.9f};

	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override
	{
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(taaDescriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		vkCmdPushConstants(cmdbuf, pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TaaPushData), &pushData);
		taaDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(taaDescriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/taa_resolve.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_INDEPENDENT, taaDescriptorSetPtr->getLayout());
		builder.usePushConstantRange({VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TaaPushData)});
	}
};

}

void TAA::init(const InitInfo& info)
{
	ASSERT(info.sceneColor != nullptr)
	ASSERT(info.GMotion != nullptr)

	ImageCreator resolvedCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{info.sceneColor->getWidth(), info.sceneColor->getHeight(), 1},
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"taaResolved");
	resolved = new Texture2D(resolvedCreator);
	for (uint32_t i = 0; i < 2; i++) {
		ImageCreator historyCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{info.sceneColor->getWidth(), info.sceneColor->getHeight(), 1},
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"taaHistory" + std::to_string(i));
		history[i] = new Texture2D(historyCreator);
	}
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		vk::insertImageBarrier(
			cmdbuf,
			resolved->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		for (const auto& historyTexture : history) {
			vk::insertImageBarrier(
				cmdbuf,
				historyTexture->resource.image,
				colorRange,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	});
	clear();

	DescriptorSetLayout taaSetLayout{};
	taaSetLayout.addBinding(Slot_SceneColor, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	taaSetLayout.addBinding(Slot_ReadHistory, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	taaSetLayout.addBinding(Slot_Resolved, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	taaSetLayout.addBinding(Slot_WriteHistory, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	taaSetLayout.addBinding(Slot_MotionVectors, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	// exact per-pixel reads: current color is texelFetch'd, motion vectors need no interpolation
	auto pointSamplerInfo = SamplerCache::defaultInfo();
	pointSamplerInfo.magFilter = VK_FILTER_NEAREST;
	pointSamplerInfo.minFilter = VK_FILTER_NEAREST;
	pointSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	pointSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	pointSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	pointSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	// history is reprojected to a fractional UV, so it needs bilinear filtering
	auto linearSamplerInfo = SamplerCache::defaultInfo();
	linearSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	linearSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	linearSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	for (uint32_t historyWriteSlot = 0; historyWriteSlot < 2; historyWriteSlot++) {
		const uint32_t readHistoryIndex = 1 - historyWriteSlot;
		DescriptorSet& taaDescriptorSet = taaDescriptorSets[historyWriteSlot];
		taaDescriptorSet = DescriptorSet(taaSetLayout);
		taaDescriptorSet.pointToImageView(info.sceneColor->imageView, Slot_SceneColor, &pointSamplerInfo);
		taaDescriptorSet.pointToImageView(history[readHistoryIndex]->imageView, Slot_ReadHistory, &linearSamplerInfo);
		taaDescriptorSet.pointToRWImageView(resolved->imageView, Slot_Resolved);
		taaDescriptorSet.pointToRWImageView(history[historyWriteSlot]->imageView, Slot_WriteHistory);
		taaDescriptorSet.pointToImageView(info.GMotion->imageView, Slot_MotionVectors, &pointSamplerInfo);
	}
}

void TAA::release()
{
	ASSERT(resolved != nullptr)
	delete resolved;
	resolved = nullptr;
	for (auto& historyTexture : history) {
		ASSERT(historyTexture != nullptr)
		delete historyTexture;
		historyTexture = nullptr;
	}
}

void TAA::clear()
{
	ASSERT(resolved != nullptr)

	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		clear(cmdbuf);
	});
}

void TAA::clear(VkCommandBuffer cmdbuf)
{
	ASSERT(cmdbuf != VK_NULL_HANDLE)
	ASSERT(resolved != nullptr)
	ASSERT(history[0] != nullptr)
	ASSERT(history[1] != nullptr)

	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	const VkClearColorValue clearColor = {};
	auto clearShaderReadTexture = [&](Texture2D* texture, VkPipelineStageFlags shaderStage)
	{
		vk::insertImageBarrier(
			cmdbuf,
			texture->resource.image,
			colorRange,
			shaderStage,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		vkCmdClearColorImage(
			cmdbuf,
			texture->resource.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&clearColor,
			1,
			&colorRange);

		vk::insertImageBarrier(
			cmdbuf,
			texture->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			shaderStage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};
	clearShaderReadTexture(resolved, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	clearShaderReadTexture(history[0], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	clearShaderReadTexture(history[1], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void TAA::render(VkCommandBuffer cmdbuf, uint32_t globalFrameIndex)
{
	const uint32_t groupCountX = (resolved->getWidth() + TAA_GROUPSIZE_X - 1) / TAA_GROUPSIZE_X;
	const uint32_t groupCountY = (resolved->getHeight() + TAA_GROUPSIZE_Y - 1) / TAA_GROUPSIZE_Y;
	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	const uint32_t historyWriteIndex = globalFrameIndex % 2;
	auto& taaDescriptorSet = taaDescriptorSets[historyWriteIndex];
	Texture2D* writeHistory = history[historyWriteIndex];

	{
		SCOPED_DRAW_EVENT(cmdbuf, "TAA Resolve")
		vk::insertImageBarrier(
			cmdbuf,
			resolved->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL);
		vk::insertImageBarrier(
			cmdbuf,
			writeHistory->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL);

		auto* resolveCS = ComputeShader::getInstance<TaaResolveCS>();
		resolveCS->pushData.historyWeight = get_deferred_config()->lookup<float>("taa.historyWeight");
		resolveCS->taaDescriptorSetPtr = &taaDescriptorSet;
		resolveCS->dispatch(cmdbuf, groupCountX, groupCountY, 1);

		vk::insertImageBarrier(
			cmdbuf,
			resolved->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_GENERAL);
		vk::insertImageBarrier(
			cmdbuf,
			writeHistory->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	vk::insertImageBarrier(
		cmdbuf,
		resolved->resource.image,
		colorRange,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
