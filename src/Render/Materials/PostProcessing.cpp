#include "PostProcessing.h"
#include "Render/Texture.h"

VkPipeline MatPostProcessing::pipeline = VK_NULL_HANDLE;
VkPipelineLayout MatPostProcessing::pipelineLayout = VK_NULL_HANDLE;

void MatPostProcessing::usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets)
{
	for (auto &dsetSlot : sharedDescriptorSets)
	{
		dsetSlot.descriptorSet.bind(cmdbuf, dsetSlot.bindingSlot, pipelineLayout);
	}

	dynamicSet.bind(cmdbuf, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

MatPostProcessing::MatPostProcessing(Texture2D* sceneColor, Texture2D* sceneDepth)
{
	name = "Post Processing";
	Material::add(this);

	// set layouts and allocation
	DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
	DescriptorSetLayout dynamicSetLayout{};
	dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	dynamicSet = DescriptorSet(dynamicSetLayout);

	// assign values
	dynamicSet.pointToImageView(sceneColor->imageView, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	dynamicSet.pointToImageView(sceneDepth->imageView, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	if (pipelineLayout == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE)
	{
		auto vk = Vulkan::Instance;
		// build the pipeline
		PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
		pipelineBuilder.fragPath = "spirv/post_processing.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.pipelineState.useVertexInput = false;
		pipelineBuilder.pipelineState.useDepthStencil = false;
		pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->postProcessPass;

		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
}

void MatPostProcessing::cleanup()
{
	if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
}

