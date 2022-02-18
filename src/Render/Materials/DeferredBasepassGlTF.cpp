#include "DeferredBasepassGlTF.h"
#include "Engine/SceneObject.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Texture.h"

#include <tinygltf/tiny_gltf.h>

VkPipelineLayout DeferredBasepassGlTF::pipelineLayout = VK_NULL_HANDLE;
VkPipeline DeferredBasepassGlTF::pipeline = VK_NULL_HANDLE;

void DeferredBasepassGlTF::setParameters(VkCommandBuffer cmdbuf, SceneObject *drawable)
{
	// per-material-instance params (static)
	materialParamsBuffer.writeData(&materialParams);

	// per-object params (dynamic)
	uniforms = {
		.ModelMatrix = drawable->object_to_world(),
	};
	uniformBuffer.writeData(&uniforms, 0, 0, instanceCounter);

	uint32_t offset = uniformBuffer.strideSize * instanceCounter;
	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, pipelineLayout, 0, 1, &offset);

	instanceCounter++;
}

void DeferredBasepassGlTF::usePipeline(VkCommandBuffer cmdbuf)
{
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

DeferredBasepassGlTF::~DeferredBasepassGlTF()
{
	uniformBuffer.release();
	materialParamsBuffer.release();
}

DeferredBasepassGlTF::DeferredBasepassGlTF(
	const tinygltf::Material& in_material,
	const std::vector<std::string>& texture_names)
{
	this->name = in_material.name;
	LOG("loading material '%s'..", name.c_str())
	VkDeviceSize alignment = Vulkan::Instance->minUniformBufferOffsetAlignment;
	uint32_t numBlocks = (sizeof(uniforms) + alignment - 1) / alignment;

	// TODO: dynamically get numStrides (num instances of that material)
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  numBlocks * alignment,
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU,
							  1, 32);
	materialParamsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(materialParams),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	Material::add(this);

	auto vk = Vulkan::Instance;

	{// pipeline and layouts

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet = DescriptorSet(dynamicSetLayout); // this commits the bindings

		// assign actual values to them
		int albedo_idx = in_material.pbrMetallicRoughness.baseColorTexture.index;
		auto albedo = dynamic_cast<Texture2D*>(Texture::get(albedo_idx >= 0 ? texture_names[albedo_idx] : "_white"));

		int normal_idx = in_material.normalTexture.index;
		auto normal = dynamic_cast<Texture2D*>(Texture::get(normal_idx >= 0 ? texture_names[normal_idx] : "_defaultNormal"));

		int mr_idx = in_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
		auto metallic_roughness = dynamic_cast<Texture2D*>(Texture::get(mr_idx >= 0 ? texture_names[mr_idx] : "_white"));

		int ao_idx = in_material.occlusionTexture.index;
		auto ao = dynamic_cast<Texture2D*>(Texture::get(ao_idx >= 0 ? texture_names[ao_idx] : "_white"));

		auto bc = in_material.pbrMetallicRoughness.baseColorFactor;
		materialParams.BaseColorFactor = glm::vec4(bc[0], bc[1], bc[2], bc[3]);
		materialParams.MetallicRoughnessAONormalStrengths.r = (float)in_material.pbrMetallicRoughness.metallicFactor;
		materialParams.MetallicRoughnessAONormalStrengths.g = (float)in_material.pbrMetallicRoughness.roughnessFactor;
		materialParams.MetallicRoughnessAONormalStrengths.b = (float)in_material.occlusionTexture.strength;
		materialParams.MetallicRoughnessAONormalStrengths.a = (float)in_material.normalTexture.scale;

		dynamicSet.pointToBuffer(uniformBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		dynamicSet.pointToBuffer(materialParamsBuffer, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet.pointToImageView(albedo->imageView, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(normal->imageView, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(metallic_roughness->imageView, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(ao->imageView, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		if (pipeline == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE)
		{
			// now build the pipeline
			GraphicsPipelineBuilder pipelineBuilder{};
			pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
			pipelineBuilder.fragPath = "spirv/geometry_gltf.frag.spv";
			pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->renderPass;

			pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

			// since it has 4 color outputs:
			auto blendInfo = pipelineBuilder.pipelineState.colorBlendAttachmentInfo;
			const VkPipelineColorBlendAttachmentState blendInfoArray[4] = {blendInfo, blendInfo, blendInfo, blendInfo};
			pipelineBuilder.pipelineState.colorBlendInfo.attachmentCount = 4;
			pipelineBuilder.pipelineState.colorBlendInfo.pAttachments = blendInfoArray;

			pipelineBuilder.build(pipeline, pipelineLayout);
		}
	}
}

void DeferredBasepassGlTF::resetInstanceCounter()
{
	instanceCounter = 0;
}
