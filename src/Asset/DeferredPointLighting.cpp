#include "DeferredPointLighting.h"
#include "Texture.h"

DeferredPointLighting::DeferredPointLighting(DeferredRenderer &deferredRenderer)
{
	name = "deferred lighting pass";
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	GBUF0 = deferredRenderer.GPosition;
	GBUF1 = deferredRenderer.GNormal;
	GBUF2 = deferredRenderer.GColor;
	GBUF3 = deferredRenderer.GMetallicRoughnessAO;
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

		pipelineBuilder.add_binding(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipelineBuilder.add_binding(0, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		pipelineBuilder.add_binding(0, 2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		pipelineBuilder.add_binding(0, 3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		pipelineBuilder.add_binding(0, 4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

		pipeline = pipelineBuilder.build();

		descriptorSetLayouts = pipelineBuilder.getLayouts();
		pipelineLayout = pipelineBuilder.pipelineLayout;
	}

	{// allocate the descriptor sets
		descriptorSet = DescriptorSet(vk->device, descriptorSetLayouts[0]);
		descriptorSet.pointToUniformBuffer(uniformBuffer, 0);
		descriptorSet.pointToImageView(GBUF0->imageView, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		descriptorSet.pointToImageView(GBUF1->imageView, 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		descriptorSet.pointToImageView(GBUF2->imageView, 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		descriptorSet.pointToImageView(GBUF3->imageView, 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
	}
}

void DeferredPointLighting::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms);

	auto dset = descriptorSet.getInstance();
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&dset,
							0, nullptr); // for dynamic descriptors (not reached yet)
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

DeferredPointLighting::~DeferredPointLighting()
{
	uniformBuffer.release();
}
