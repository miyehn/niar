#include "DeferredLighting.h"
#include "Render/Texture.h"

VkPipeline DeferredLighting::pipeline = VK_NULL_HANDLE;
VkPipelineLayout DeferredLighting::pipelineLayout = VK_NULL_HANDLE;

DeferredLighting::DeferredLighting(DeferredRenderer* renderer)
{
	name = "Deferred Lighting";
	pointLightsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(pointLights),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	directionalLightsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								  sizeof(directionalLights),
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto vk = Vulkan::Instance;

	{// create the layouts and build the pipeline

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet = DescriptorSet(dynamicSetLayout);

		// assign values
		dynamicSet.pointToBuffer(pointLightsBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet.pointToBuffer(directionalLightsBuffer, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        if (pipelineLayout == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE)
        {
            // build the pipeline
            GraphicsPipelineBuilder pipelineBuilder{};
            pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
            pipelineBuilder.fragPath = "spirv/deferred_lighting.frag.spv";
            pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
            pipelineBuilder.pipelineState.useVertexInput = false;
            pipelineBuilder.pipelineState.useDepthStencil = false;
            pipelineBuilder.compatibleRenderPass = renderer->renderPass;
            pipelineBuilder.compatibleSubpass = 1;

            pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
            pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

            pipelineBuilder.build(pipeline, pipelineLayout);
        }
	}
}

void DeferredLighting::usePipeline(VkCommandBuffer cmdbuf)
{
	pointLightsBuffer.writeData(&pointLights, numPointLights * sizeof(PointLightInfo));
	directionalLightsBuffer.writeData(&directionalLights, numDirectionalLights * sizeof(DirectionalLightInfo));

	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, pipelineLayout);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

DeferredLighting::~DeferredLighting()
{
	pointLightsBuffer.release();
	directionalLightsBuffer.release();
}
