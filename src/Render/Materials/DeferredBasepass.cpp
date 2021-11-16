#include "DeferredBasepass.h"
#include "Engine/SceneObject.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Texture.h"

VkPipelineLayout DeferredBasepass::pipelineLayout = VK_NULL_HANDLE;
VkPipeline DeferredBasepass::pipeline = VK_NULL_HANDLE;

DeferredBasepass::DeferredBasepass(
	const std::string &name,
	const std::string &albedo_tex,
	const std::string &normal_tex,
	const std::string &metallic_tex,
	const std::string &roughness_tex,
	const std::string &ao_path
){
	this->name = name;
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	Material::add(this);

	auto vk = Vulkan::Instance;

	{// pipeline and layouts

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet = DescriptorSet(dynamicSetLayout); // this commits the bindings

		// assign actual values to them
		auto albedo = dynamic_cast<Texture2D*>(Texture::get(albedo_tex));
		auto normal = dynamic_cast<Texture2D*>(Texture::get(normal_tex));
		auto metallic = dynamic_cast<Texture2D*>(Texture::get(metallic_tex));
		auto roughness = dynamic_cast<Texture2D*>(Texture::get(roughness_tex));
		auto ao = dynamic_cast<Texture2D*>(Texture::get(ao_path.length() == 0 ? "_white" : ao_path));
		dynamicSet.pointToBuffer(uniformBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet.pointToImageView(albedo->imageView, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(normal->imageView, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(metallic->imageView, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(roughness->imageView, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(ao->imageView, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        if (pipeline == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE)
        {
            // now build the pipeline
            GraphicsPipelineBuilder pipelineBuilder{};
            pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
            pipelineBuilder.fragPath = "spirv/geometry.frag.spv";
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

void DeferredBasepass::usePipeline(VkCommandBuffer cmdbuf)
{
	uniformBuffer.writeData(&uniforms);

	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void DeferredBasepass::setParameters(VkCommandBuffer cmdbuf, SceneObject *drawable)
{
	uniforms = {
		.ModelMatrix = drawable->object_to_world(),
	};
}

DeferredBasepass::~DeferredBasepass()
{
	uniformBuffer.release();
}

void DeferredBasepass::cleanup()
{
	if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
}
