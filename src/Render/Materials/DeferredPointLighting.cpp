#include "DeferredPointLighting.h"
#include "Texture.h"

MatDeferredPointLighting::MatDeferredPointLighting()
{
	name = "deferred lighting pass";
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	Material::add(this);

	auto vk = Vulkan::Instance;

	{// create the layouts and build the pipeline

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet = DescriptorSet(dynamicSetLayout);

		// assign values
		dynamicSet.pointToUniformBuffer(uniformBuffer, 0);

		// build the pipeline
		PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
		pipelineBuilder.fragPath = "spirv/deferred_lighting_point.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.pipelineState.useVertexInput = false;
		pipelineBuilder.pipelineState.useDepthStencil = false;
		pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->getRenderPass();
		pipelineBuilder.compatibleSubpass = 1;

		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
}

void MatDeferredPointLighting::usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets)
{
	uniformBuffer.writeData(&uniforms);

	for (auto &dsetSlot : sharedDescriptorSets)
	{
		dsetSlot.descriptorSet.bind(cmdbuf, dsetSlot.bindingSlot, pipelineLayout);
	}

	dynamicSet.bind(cmdbuf, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

MatDeferredPointLighting::~MatDeferredPointLighting()
{
	uniformBuffer.release();
	vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
}
