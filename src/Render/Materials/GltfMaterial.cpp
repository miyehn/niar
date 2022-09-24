#include "GltfMaterial.h"
#include "Scene/SceneObject.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Texture.h"

#include <tinygltf/tiny_gltf.h>
#include "Render/Renderers/SimpleRenderer.h"

#define MAX_MATERIAL_INSTANCES 128

void GltfMaterial::setParameters(VkCommandBuffer cmdbuf, SceneObject *drawable)
{
	// per-material-instance params (static)
	materialParamsBuffer.writeData(&materialParams);

	// per-object params (dynamic)
	uniforms = {
		.ModelMatrix = drawable->object_to_world(),
	};
	uniformBuffer.writeData(&uniforms, 0, 0, instanceCounter);

	uint32_t offset = uniformBuffer.strideSize * instanceCounter;
	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, getPipeline().layout, 0, 1, &offset);

	instanceCounter++;
}

void GltfMaterial::usePipeline(VkCommandBuffer cmdbuf)
{
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline().pipeline);
}

GltfMaterial::~GltfMaterial()
{
	uniformBuffer.release();
	materialParamsBuffer.release();
}

GltfMaterial::GltfMaterial(const GltfMaterialInfo &info)
{
	this->name = info.name;
	this->_version = info._version;
	LOG("loading material '%s'..", name.c_str())
	VkDeviceSize alignment = Vulkan::Instance->minUniformBufferOffsetAlignment;
	uint32_t numBlocks = (sizeof(uniforms) + alignment - 1) / alignment;

	// TODO: dynamically get numStrides (num instances of that material)
	std::string bufferName = "Material uniform buffer (" + info.name + ")";
	uniformBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
							  numBlocks * alignment,
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU,
							  bufferName,
							  1, MAX_MATERIAL_INSTANCES});
	bufferName = "Material params buffer (" + info.name + ")";
	materialParamsBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
									 sizeof(materialParams),
									 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
									 VMA_MEMORY_USAGE_CPU_TO_GPU,
									 bufferName});

	{// pipeline and layouts

		// set layouts and allocation
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet = DescriptorSet(dynamicSetLayout); // this commits the bindings

		// assign actual values to them
		auto albedo = Texture::get<Texture2D>(info.albedoTexName);
		auto normal = Texture::get<Texture2D>(info.normalTexName);
		auto orm = Texture::get<Texture2D>(info.mrTexName);

		materialParams.BaseColorFactor = info.BaseColorFactor;
		materialParams.OcclusionRoughnessMetallicNormalStrengths = info.OcclusionRoughnessMetallicNormalStrengths;

		dynamicSet.pointToBuffer(uniformBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		dynamicSet.pointToBuffer(materialParamsBuffer, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet.pointToImageView(albedo->imageView, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(normal->imageView, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(orm->imageView, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}
}

void GltfMaterial::resetInstanceCounter()
{
	instanceCounter = 0;
}

MaterialPipeline PbrGltfMaterial::getPipeline()
{
	static MaterialPipeline materialPipeline = {};
	if (materialPipeline.pipeline == VK_NULL_HANDLE || materialPipeline.layout == VK_NULL_HANDLE)
	{
		auto vk = Vulkan::Instance;

		// now build the pipeline
		GraphicsPipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
		pipelineBuilder.fragPath = "spirv/geometry.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->renderPass;

		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		// since it has 4 color outputs:
		auto blendInfo = pipelineBuilder.pipelineState.colorBlendAttachmentInfo;
		const VkPipelineColorBlendAttachmentState blendInfoArray[4] = {blendInfo, blendInfo, blendInfo, blendInfo};
		pipelineBuilder.pipelineState.colorBlendInfo.attachmentCount = 4;
		pipelineBuilder.pipelineState.colorBlendInfo.pAttachments = blendInfoArray;

		pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
	}

	return materialPipeline;
}

MaterialPipeline SimpleGltfMaterial::getPipeline()
{
	static MaterialPipeline materialPipeline = {};
	if (materialPipeline.pipeline == VK_NULL_HANDLE || materialPipeline.layout == VK_NULL_HANDLE)
	{
		auto vk = Vulkan::Instance;

		// now build the pipeline
		GraphicsPipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
		pipelineBuilder.fragPath = "spirv/simple_gltf.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = SimpleRenderer::get()->renderPass;

		DescriptorSetLayout frameGlobalSetLayout = SimpleRenderer::get()->descriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		// since it has 4 color outputs:
		auto blendInfo = pipelineBuilder.pipelineState.colorBlendAttachmentInfo;
		pipelineBuilder.pipelineState.colorBlendInfo.attachmentCount = 1;
		pipelineBuilder.pipelineState.colorBlendInfo.pAttachments = &blendInfo;

		pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
	}

	return materialPipeline;
}