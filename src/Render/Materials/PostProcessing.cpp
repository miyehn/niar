#include "PostProcessing.h"
#include "Render/Texture.h"

void PostProcessing::usePipeline(VkCommandBuffer cmdbuf)
{
	MaterialPipeline materialPipeline = getPipeline();
	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, materialPipeline.layout);
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, materialPipeline.pipeline);
}

PostProcessing::PostProcessing(DeferredRenderer* renderer, Texture2D* sceneColor, Texture2D* sceneDepth)
{
	name = "Post Processing";
	postProcessPass = renderer->postProcessPass;

	// set layouts and allocation
	DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
	DescriptorSetLayout dynamicSetLayout{};
	dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	dynamicSet = DescriptorSet(dynamicSetLayout);

	// assign values
	dynamicSet.pointToImageView(sceneColor->imageView, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	dynamicSet.pointToImageView(sceneDepth->imageView, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

MaterialPipeline PostProcessing::getPipeline()
{
	static MaterialPipeline materialPipeline = {};
	if (materialPipeline.pipeline == VK_NULL_HANDLE || materialPipeline.layout == VK_NULL_HANDLE)
	{
		auto vk = Vulkan::Instance;
		// build the pipeline
		GraphicsPipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
		pipelineBuilder.fragPath = "spirv/post_processing.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.pipelineState.useVertexInput = false;
		pipelineBuilder.pipelineState.useDepthStencil = false;
		pipelineBuilder.compatibleRenderPass = postProcessPass;

		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
	}

	return materialPipeline;
}