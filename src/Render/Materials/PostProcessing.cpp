#include "PostProcessing.h"
#include "Render/Texture.h"

VkPipeline PostProcessing::pipeline = VK_NULL_HANDLE;
VkPipelineLayout PostProcessing::pipelineLayout = VK_NULL_HANDLE;

void PostProcessing::usePipeline(VkCommandBuffer cmdbuf)
{
	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

PostProcessing::PostProcessing(DeferredRenderer* renderer, Texture2D* sceneColor, Texture2D* sceneDepth)
{
	name = "Post Processing";

	// set layouts and allocation
	DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
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
		GraphicsPipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
		pipelineBuilder.fragPath = "spirv/post_processing.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.pipelineState.useVertexInput = false;
		pipelineBuilder.pipelineState.useDepthStencil = false;
		pipelineBuilder.compatibleRenderPass = renderer->postProcessPass;

		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
}
