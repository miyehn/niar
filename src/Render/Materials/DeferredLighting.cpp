#include "DeferredLighting.h"
#include "Render/Texture.h"

VkPipeline MatDeferredLighting::pipeline = VK_NULL_HANDLE;
VkPipelineLayout MatDeferredLighting::pipelineLayout = VK_NULL_HANDLE;

MatDeferredLighting::MatDeferredLighting(DeferredRenderer* renderer)
{
	name = "DeferredLighting";
	pointLightsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(pointLights),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	directionalLightsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								  sizeof(directionalLights),
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU);
	Material::add(this);

	auto vk = Vulkan::Instance;

	{// create the layouts and build the pipeline

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet = DescriptorSet(dynamicSetLayout);

		// assign values
		dynamicSet.pointToUniformBuffer(pointLightsBuffer, 0);
		dynamicSet.pointToUniformBuffer(directionalLightsBuffer, 1);

        if (pipelineLayout == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE)
        {
            // build the pipeline
            PipelineBuilder pipelineBuilder{};
            pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
            pipelineBuilder.fragPath = "spirv/deferred_lighting.frag.spv";
            pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
            pipelineBuilder.pipelineState.useVertexInput = false;
            pipelineBuilder.pipelineState.useDepthStencil = false;
            pipelineBuilder.compatibleRenderPass = renderer->getRenderPass();
            pipelineBuilder.compatibleSubpass = 1;

            pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
            pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

            pipelineBuilder.build(pipeline, pipelineLayout);
        }
	}
}

void MatDeferredLighting::usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets)
{
	pointLightsBuffer.writeData(&pointLights, numPointLights * sizeof(PointLightInfo));
	directionalLightsBuffer.writeData(&directionalLights, numDirectionalLights * sizeof(DirectionalLightInfo));

	for (auto &dsetSlot : sharedDescriptorSets)
	{
		dsetSlot.descriptorSet.bind(cmdbuf, dsetSlot.bindingSlot, pipelineLayout);
	}

	dynamicSet.bind(cmdbuf, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

MatDeferredLighting::~MatDeferredLighting()
{
	pointLightsBuffer.release();
	directionalLightsBuffer.release();
}

void MatDeferredLighting::cleanup() {
	if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
}
