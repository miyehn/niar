#include "DeferredPointLighting.h"
#include "Texture.h"

DeferredPointLighting::DeferredPointLighting(DeferredRenderer &deferredRenderer)
{
	name = "deferred lighting pass";
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	add_material(this);

	auto vk = Vulkan::Instance;

	{// create the layouts and build the pipeline
		PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
		pipelineBuilder.fragPath = "spirv/deferred_lighting_point.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.pipelineState.useVertexInput = false;
		pipelineBuilder.pipelineState.useDepthStencil = false;
		pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->getRenderPass();
		pipelineBuilder.compatibleSubpass = 1;

		pipelineBuilder.include_descriptor_set_layout(0, deferredRenderer.frameGlobalDescriptorSet.layout);
		pipelineBuilder.add_binding(3, 0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		pipeline = pipelineBuilder.build();

		descriptorSetLayouts = pipelineBuilder.descriptorSetLayouts;
		pipelineLayout = pipelineBuilder.pipelineLayout;
	}

	{// allocate the (per-material managed) descriptor sets
		dynamicSet = DescriptorSet(vk->device, descriptorSetLayouts[3]);
		dynamicSet.pointToUniformBuffer(uniformBuffer, 0);
	}
}

void DeferredPointLighting::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms);

	auto dset = dynamicSet.getInstance();
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							3, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&dset,
							0, nullptr); // for dynamic descriptors (not reached yet)
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

DeferredPointLighting::~DeferredPointLighting()
{
	uniformBuffer.release();
}
