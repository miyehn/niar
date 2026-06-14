//
// Created by miyehn on 6/11/2026.
//

#include "GI.h"
#include "Assets/ConfigAsset.hpp"
#include "Render/Texture.h"
#include "Render/Materials/ComputeShader.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"

#define RTGI_GROUPSIZE_X 8
#define RTGI_GROUPSIZE_Y 8

namespace
{

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

	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override
	{
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(giDescriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		giDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_FRAMEGLOBAL, pipeline.layout);
		skyDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(giDescriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/rtgi_generate.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, giDescriptorSetPtr->getLayout());
		builder.useDescriptorSetLayout(DSET_INDEPENDENT, skyDescriptorSetPtr->getLayout());
	}
};

class RtgiCompositeCS : public ComputeShader
{
public:
	const DescriptorSet* descriptorSetPtr = nullptr;

	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override
	{
		ASSERT(descriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		descriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_FRAMEGLOBAL, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(descriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/rtgi_composite.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, descriptorSetPtr->getLayout());
	}
};
}

void GI::init(const InitInfo& info)
{
	ASSERT(info.GPosition != nullptr)
	ASSERT(info.GNormal != nullptr)
	ASSERT(info.sceneColor != nullptr)
	ASSERT(info.tlas != VK_NULL_HANDLE)

	sceneColor = info.sceneColor;

	ImageCreator indirectLightingCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{sceneColor->getWidth(), sceneColor->getHeight(), 1},
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"indirectLighting");
	indirectLighting = new Texture2D(indirectLightingCreator);
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		vk::insertImageBarrier(
			cmdbuf,
			indirectLighting->resource.image,
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	DescriptorSetLayout giSetLayout{};
	giSetLayout.addBinding(0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	giSetLayout.addBinding(1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	giSetLayout.addBinding(2, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	giSetLayout.addBinding(3, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	giSetLayout.addBinding(4, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	giSetLayout.addBinding(5, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	auto samplerInfo = SamplerCache::defaultInfo();
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		ASSERT(info.viewInfoUbos[i] != nullptr)
		giDescriptorSets[i] = DescriptorSet(giSetLayout);
		giDescriptorSets[i].pointToBuffer(*info.viewInfoUbos[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		giDescriptorSets[i].pointToImageView(
			info.GPosition->imageView,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&samplerInfo);
		giDescriptorSets[i].pointToImageView(
			info.GNormal->imageView,
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&samplerInfo);
		giDescriptorSets[i].pointToAccelerationStructure(info.tlas, 3);
		giDescriptorSets[i].pointToRWImageView(indirectLighting->imageView, 4);
		giDescriptorSets[i].pointToRWImageView(info.sceneColor->imageView, 5);
	}
}

void GI::release()
{
	ASSERT(indirectLighting != nullptr)
	delete indirectLighting;
	indirectLighting = nullptr;
	sceneColor = nullptr;
}

void GI::render(VkCommandBuffer cmdbuf, uint32_t frameIndex, const DescriptorSet& skyDescriptorSet)
{
	// enabled?
	if (get_gi_config()->lookup<int>("enabled") == 0) return;

	ASSERT(frameIndex < MAX_FRAMES_IN_FLIGHT)
	auto& giDescriptorSet = giDescriptorSets[frameIndex];
	const uint32_t groupCountX = (indirectLighting->getWidth() + RTGI_GROUPSIZE_X - 1) / RTGI_GROUPSIZE_X;
	const uint32_t groupCountY = (indirectLighting->getHeight() + RTGI_GROUPSIZE_Y - 1) / RTGI_GROUPSIZE_Y;
	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	// in this case, sceneColor already has its barrier between translucency and this GI render pass (external),
	// so no need to insert another one here

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

		auto* generateCS = ComputeShader::getInstance<RtgiGenerateCS>();
		generateCS->giDescriptorSetPtr = &giDescriptorSet;
		generateCS->skyDescriptorSetPtr = &skyDescriptorSet;
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
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "RTGI Composite")
		auto* compositeCS = ComputeShader::getInstance<RtgiCompositeCS>();
		compositeCS->descriptorSetPtr = &giDescriptorSet;

		vk::insertImageBarrier(
			cmdbuf,
			sceneColor->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL);

		compositeCS->dispatch(cmdbuf, groupCountX, groupCountY, 1);

		vk::insertImageBarrier(
			cmdbuf,
			sceneColor->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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