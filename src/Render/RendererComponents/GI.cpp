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

class RtgiGenerateCS : public ComputeShader
{
public:
	const DescriptorSet* giDescriptorSetPtr = nullptr;
	const DescriptorSet* skyDescriptorSetPtr = nullptr;
	const DescriptorSet* bindlessDescriptorSetPtr = nullptr;

	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override
	{
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(giDescriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)
		ASSERT(bindlessDescriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
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
		giDescriptorSets[i] = DescriptorSet(giSetLayout);
		const uint32_t writeHistoryIndex = i % 2;
		const uint32_t readHistoryIndex = 1 - writeHistoryIndex;
		giDescriptorSets[i].pointToBuffer(*info.viewInfoUbos[i], Slot_ViewInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		giDescriptorSets[i].pointToImageView(info.sceneDepth->imageView, Slot_SceneDepth, &samplerInfo);
		giDescriptorSets[i].pointToImageView(info.GNormal->imageView, Slot_GNormal, &samplerInfo);
		giDescriptorSets[i].pointToAccelerationStructure(info.tlas, Slot_Tlas);
		giDescriptorSets[i].pointToRWImageView(indirectLighting->imageView, Slot_IndirectLighting);
		giDescriptorSets[i].pointToImageView(info.environmentMap->imageView, Slot_EnvironmentMap);
		giDescriptorSets[i].pointToBuffer(*info.sceneInstanceRecordBuffers[i], Slot_SceneInstanceRecords, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		giDescriptorSets[i].pointToBuffer(*info.pointLightBuffers[i], Slot_PointLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		giDescriptorSets[i].pointToBuffer(*info.directionalLightBuffers[i], Slot_DirectionalLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		giDescriptorSets[i].pointToImageView(history[readHistoryIndex]->imageView, Slot_ReadHistory, &samplerInfo);
		giDescriptorSets[i].pointToRWImageView(history[writeHistoryIndex]->imageView, Slot_WriteHistory);
		giDescriptorSets[i].pointToRWImageView(sampleCount->imageView, Slot_SampleCount);
	}
}
// otherwise history pingpong breaks. TODO [myn]: more robust implementation
static_assert(MAX_FRAMES_IN_FLIGHT % 2 == 0);

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
	auto& giDescriptorSet = giDescriptorSets[frameIndex];
	const uint32_t groupCountX = (indirectLighting->getWidth() + RTGI_GROUPSIZE_X - 1) / RTGI_GROUPSIZE_X;
	const uint32_t groupCountY = (indirectLighting->getHeight() + RTGI_GROUPSIZE_Y - 1) / RTGI_GROUPSIZE_Y;
	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	const uint32_t historyWriteIndex = frameIndex % 2;
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
}
