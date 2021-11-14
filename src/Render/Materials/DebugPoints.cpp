#include "DebugPoints.h"
#include "Render/Vulkan/VulkanUtils.h"

VkPipeline DebugPoints::pipeline = VK_NULL_HANDLE;
VkPipelineLayout DebugPoints::pipelineLayout = VK_NULL_HANDLE;

void DebugPoints::cleanup()
{
	if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
}

DebugPoints::DebugPoints(DeferredRenderer* renderer, const std::vector<PointData>& initialPoints) : numPoints(initialPoints.size())
{
	{// initialize buffer
		VkDeviceSize bufferSize = sizeof(PointData) * numPoints;

		VmaBuffer stagingBuffer(
			&Vulkan::Instance->memoryAllocator, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		stagingBuffer.writeData((void*)initialPoints.data());

		auto vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		pointsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator, bufferSize, vkUsage, VMA_MEMORY_USAGE_GPU_ONLY);

		vk::copyBuffer(pointsBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
		stagingBuffer.release();
	}

	if (pipelineLayout == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE)
	{
		// build the pipeline
		auto vk = Vulkan::Instance;
		PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/debug_point.vert.spv";
		pipelineBuilder.fragPath = "spirv/debug_point.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = renderer->postProcessPass;
		pipelineBuilder.compatibleSubpass = 1;

		// input info
		pipelineBuilder.pipelineState.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		PointData::getBindingDescription(pipelineBuilder.pipelineState.bindingDescription);
		PointData::getAttributeDescriptions(pipelineBuilder.pipelineState.attributeDescriptions);
		pipelineBuilder.pipelineState.vertexInputInfo.vertexAttributeDescriptionCount = 1;
		pipelineBuilder.pipelineState.rasterizationInfo.polygonMode = VK_POLYGON_MODE_POINT;

		DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
}

void DebugPoints::bindAndDraw(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets)
{
	for (auto &dsetSlot : sharedDescriptorSets)
	{
		dsetSlot.descriptorSet.bind(cmdbuf, dsetSlot.bindingSlot, pipelineLayout);
	}
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	VkDeviceSize offsets[] = { 0 };
	auto vb = pointsBuffer.getBufferInstance();
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vb, offsets); // offset, #bindings, (content)
	vkCmdDraw(cmdbuf, numPoints, 1, 0, 0);
}

DebugPoints::~DebugPoints()
{
	pointsBuffer.release();
}