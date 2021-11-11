#include "DeferredBasepassGlTF.h"
#include "Engine/SceneObject.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Texture.h"

#include <tinygltf/tiny_gltf.h>

VkPipelineLayout MatDeferredBasepassGlTF::pipelineLayout = VK_NULL_HANDLE;
VkPipeline MatDeferredBasepassGlTF::pipeline = VK_NULL_HANDLE;

void MatDeferredBasepassGlTF::setParameters(SceneObject *drawable)
{
	uniforms = {
		.ModelMatrix = drawable->object_to_world(),
	};
}

void MatDeferredBasepassGlTF::usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets)
{
	uniformBuffer.writeData(&uniforms);
	materialParamsBuffer.writeData(&materialParams);

	for (auto &dsetSlot : sharedDescriptorSets)
	{
		dsetSlot.descriptorSet.bind(cmdbuf, dsetSlot.bindingSlot, pipelineLayout);
	}

	dynamicSet.bind(cmdbuf, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

MatDeferredBasepassGlTF::~MatDeferredBasepassGlTF()
{
	uniformBuffer.release();
	materialParamsBuffer.release();
}

void MatDeferredBasepassGlTF::cleanup()
{
	if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
}

MatDeferredBasepassGlTF::MatDeferredBasepassGlTF(
	const tinygltf::Material& in_material,
	const std::vector<std::string>& texture_names)
{
	this->name = in_material.name;
	LOG("loading material '%s'..", name.c_str())
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
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
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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
		auto metallic_roughness = dynamic_cast<Texture2D*>(Texture::get(mr_idx >= 0 ? texture_names[mr_idx] : "_defaultGltfMetallicRoughness"));

		int ao_idx = in_material.occlusionTexture.index;
		auto ao = dynamic_cast<Texture2D*>(Texture::get(ao_idx >= 0 ? texture_names[ao_idx] : "_white"));

		auto bc = in_material.pbrMetallicRoughness.baseColorFactor;
		materialParams.BaseColorFactor = glm::vec4(bc[0], bc[1], bc[2], bc[3]);
		materialParams.MetallicRoughnessAONormalStrengths.r = (float)in_material.pbrMetallicRoughness.metallicFactor;
		materialParams.MetallicRoughnessAONormalStrengths.g = (float)in_material.pbrMetallicRoughness.roughnessFactor;
		materialParams.MetallicRoughnessAONormalStrengths.b = (float)in_material.occlusionTexture.strength;
		materialParams.MetallicRoughnessAONormalStrengths.a = (float)in_material.normalTexture.scale;

		dynamicSet.pointToUniformBuffer(uniformBuffer, 0);
		dynamicSet.pointToUniformBuffer(materialParamsBuffer, 1);
		dynamicSet.pointToImageView(albedo->imageView, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(normal->imageView, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(metallic_roughness->imageView, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(ao->imageView, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		if (pipeline == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE)
		{
			// now build the pipeline
			PipelineBuilder pipelineBuilder{};
			pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
			pipelineBuilder.fragPath = "spirv/geometry_gltf.frag.spv";
			pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->getRenderPass();

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
