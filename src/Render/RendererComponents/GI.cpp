//
// Created by miyehn on 6/11/2026.
//

#include "GI.h"
#include "Assets/ConfigAsset.hpp"
#include "Render/BindlessResources.h"
#include "Render/Texture.h"
#include "Render/Materials/ComputeShader.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"

#include <string>

#define RTGI_GROUPSIZE_X 8
#define RTGI_GROUPSIZE_Y 8

namespace
{

constexpr uint32_t Slot_ViewInfo = 0;
constexpr uint32_t Slot_SceneDepth = 1;
constexpr uint32_t Slot_GNormal = 2;
constexpr uint32_t Slot_Tlas = 3;
constexpr uint32_t Slot_IndirectLighting = 4;
constexpr uint32_t Slot_EnvironmentMap = 5;
constexpr uint32_t Slot_SceneInstanceRecords = 6;
constexpr uint32_t Slot_PointLights = 7;
constexpr uint32_t Slot_DirectionalLights = 8;
constexpr uint32_t Slot_ReadHistory = 9;
constexpr uint32_t Slot_WriteHistory = 10;
constexpr uint32_t Slot_SampleCount = 11;

// currently made local to this file
ConfigAsset* get_gi_config()
{
	static ConfigAsset* config = nullptr;
	if (!config) {
		config = new ConfigAsset("config/gi.ini", true, [](const ConfigAsset* cfg)
		{
			LOG("GI enabled: %i", cfg->lookup<int>("enabled"));
		});
	}
	return config;
}

struct RtgiPushData {
	uint32_t maxSampleCount;
	float historyDepthRejectionThreshold;
};

class RtgiGenerateCS : public ComputeShader
{
public:
	const DescriptorSet* giDescriptorSetPtr = nullptr;
	const DescriptorSet* skyDescriptorSetPtr = nullptr;
	const DescriptorSet* bindlessDescriptorSetPtr = nullptr;
	RtgiPushData pushData = {64, 0.1f};

	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override
	{
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(giDescriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)
		ASSERT(bindlessDescriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		vkCmdPushConstants(cmdbuf, pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RtgiPushData), &pushData);
		giDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_FRAMEGLOBAL, pipeline.layout);
		skyDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipeline.layout);
		bindlessDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_BINDLESS, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(giDescriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)
		ASSERT(bindlessDescriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/rtgi_generate.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, giDescriptorSetPtr->getLayout());
		builder.useDescriptorSetLayout(DSET_INDEPENDENT, skyDescriptorSetPtr->getLayout());
		builder.useDescriptorSetLayout(DSET_BINDLESS, bindlessDescriptorSetPtr->getLayout());
		builder.usePushConstantRange({VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RtgiPushData)});
	}
};

}

void GI::init(const InitInfo& info)
{
	ASSERT(info.sceneDepth != nullptr)
	ASSERT(info.GNormal != nullptr)
	ASSERT(info.tlas != VK_NULL_HANDLE)
	ASSERT(info.environmentMap != nullptr)

	ImageCreator indirectLightingCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{info.sceneDepth->getWidth(), info.sceneDepth->getHeight(), 1},
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"indirectLighting");
	indirectLighting = new Texture2D(indirectLightingCreator);
	for (uint32_t i = 0; i < 2; i++) {
		ImageCreator historyCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{info.sceneDepth->getWidth(), info.sceneDepth->getHeight(), 1},
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"rtgiHistory" + std::to_string(i));
		history[i] = new Texture2D(historyCreator);
	}
	ImageCreator sampleCountCreator(
		VK_FORMAT_R32_UINT,
		{info.sceneDepth->getWidth(), info.sceneDepth->getHeight(), 1},
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"rtgiSampleCount");
	sampleCount = new Texture2D(sampleCountCreator);
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		vk::insertImageBarrier(
			cmdbuf,
			indirectLighting->resource.image,
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
		vk::insertImageBarrier(
			cmdbuf,
			sampleCount->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL);
	});
	clear();

	DescriptorSetLayout giSetLayout{};
	giSetLayout.addBinding(Slot_ViewInfo, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	giSetLayout.addBinding(Slot_SceneDepth, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	giSetLayout.addBinding(Slot_GNormal, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	giSetLayout.addBinding(Slot_Tlas, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	giSetLayout.addBinding(Slot_IndirectLighting, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	giSetLayout.addBinding(Slot_EnvironmentMap, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	giSetLayout.addBinding(Slot_SceneInstanceRecords, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	giSetLayout.addBinding(Slot_PointLights, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	giSetLayout.addBinding(Slot_DirectionalLights, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	giSetLayout.addBinding(Slot_ReadHistory, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	giSetLayout.addBinding(Slot_WriteHistory, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	giSetLayout.addBinding(Slot_SampleCount, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	auto samplerInfo = SamplerCache::defaultInfo();
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		ASSERT(info.viewInfoUbos[i] != nullptr)
		ASSERT(info.sceneInstanceRecordBuffers[i] != nullptr)
		ASSERT(info.pointLightBuffers[i] != nullptr)
		ASSERT(info.directionalLightBuffers[i] != nullptr)
		for (uint32_t historyWriteSlot = 0; historyWriteSlot < 2; historyWriteSlot++) {
			const uint32_t readHistoryIndex = 1 - historyWriteSlot;
			DescriptorSet& giDescriptorSet = giDescriptorSets[i][historyWriteSlot];
			giDescriptorSet = DescriptorSet(giSetLayout);
			giDescriptorSet.pointToBuffer(*info.viewInfoUbos[i], Slot_ViewInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			giDescriptorSet.pointToImageView(info.sceneDepth->imageView, Slot_SceneDepth, &samplerInfo);
			giDescriptorSet.pointToImageView(info.GNormal->imageView, Slot_GNormal, &samplerInfo);
			giDescriptorSet.pointToAccelerationStructure(info.tlas, Slot_Tlas);
			giDescriptorSet.pointToRWImageView(indirectLighting->imageView, Slot_IndirectLighting);
			giDescriptorSet.pointToImageView(info.environmentMap->imageView, Slot_EnvironmentMap);
			giDescriptorSet.pointToBuffer(*info.sceneInstanceRecordBuffers[i], Slot_SceneInstanceRecords, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			giDescriptorSet.pointToBuffer(*info.pointLightBuffers[i], Slot_PointLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			giDescriptorSet.pointToBuffer(*info.directionalLightBuffers[i], Slot_DirectionalLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			giDescriptorSet.pointToImageView(history[readHistoryIndex]->imageView, Slot_ReadHistory, &samplerInfo);
			giDescriptorSet.pointToRWImageView(history[historyWriteSlot]->imageView, Slot_WriteHistory);
			giDescriptorSet.pointToRWImageView(sampleCount->imageView, Slot_SampleCount);
		}
	}
}

void GI::release()
{
	ASSERT(indirectLighting != nullptr)
	delete indirectLighting;
	indirectLighting = nullptr;
	for (auto& historyTexture : history) {
		ASSERT(historyTexture != nullptr)
		delete historyTexture;
		historyTexture = nullptr;
	}
	ASSERT(sampleCount != nullptr)
	delete sampleCount;
	sampleCount = nullptr;
}

void GI::clear()
{
	ASSERT(indirectLighting != nullptr)

	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		clear(cmdbuf);
	});
}

void GI::clear(VkCommandBuffer cmdbuf)
{
	ASSERT(cmdbuf != VK_NULL_HANDLE)
	ASSERT(indirectLighting != nullptr)
	ASSERT(history[0] != nullptr)
	ASSERT(history[1] != nullptr)
	ASSERT(sampleCount != nullptr)

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
	clearShaderReadTexture(indirectLighting, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	clearShaderReadTexture(history[0], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	clearShaderReadTexture(history[1], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	vk::insertImageBarrier(
		cmdbuf,
		sampleCount->resource.image,
		colorRange,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdClearColorImage(
		cmdbuf,
		sampleCount->resource.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&clearColor,
		1,
		&colorRange);

	vk::insertImageBarrier(
		cmdbuf,
		sampleCount->resource.image,
		colorRange,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL);
	historyWriteIndex = 0;
}

void GI::render(VkCommandBuffer cmdbuf, uint32_t frameIndex, const DescriptorSet& skyDescriptorSet)
{
	const bool enabled = get_gi_config()->lookup<int>("enabled") != 0;
	if (!enabled) {
		if (enabledLastFrame) {
			SCOPED_DRAW_EVENT(cmdbuf, "RTGI Clear")
			clear(cmdbuf);
		}
		enabledLastFrame = false;
		return;
	}
	enabledLastFrame = true;

	ASSERT(frameIndex < MAX_FRAMES_IN_FLIGHT)
	const uint32_t groupCountX = (indirectLighting->getWidth() + RTGI_GROUPSIZE_X - 1) / RTGI_GROUPSIZE_X;
	const uint32_t groupCountY = (indirectLighting->getHeight() + RTGI_GROUPSIZE_Y - 1) / RTGI_GROUPSIZE_Y;
	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	auto& giDescriptorSet = giDescriptorSets[frameIndex][historyWriteIndex];
	Texture2D* writeHistory = history[historyWriteIndex];

	{
		SCOPED_DRAW_EVENT(cmdbuf, "RTGI Generate")
		vk::insertImageBarrier(
			cmdbuf,
			indirectLighting->resource.image,
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

		auto* generateCS = ComputeShader::getInstance<RtgiGenerateCS>();
		ASSERT(BindlessResources::Instance != nullptr)
		generateCS->pushData.maxSampleCount = static_cast<uint32_t>(get_gi_config()->lookup<int>("maxSampleCount"));
		generateCS->pushData.historyDepthRejectionThreshold = get_gi_config()->lookup<float>("historyDepthRejectionThreshold");
		generateCS->giDescriptorSetPtr = &giDescriptorSet;
		generateCS->skyDescriptorSetPtr = &skyDescriptorSet;
		generateCS->bindlessDescriptorSetPtr = &BindlessResources::Instance->descriptorSet();
		generateCS->dispatch(cmdbuf, groupCountX, groupCountY, 1);

		vk::insertImageBarrier(
			cmdbuf,
			indirectLighting->resource.image,
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
		indirectLighting->resource.image,
		colorRange,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	historyWriteIndex = 1 - historyWriteIndex;
}
