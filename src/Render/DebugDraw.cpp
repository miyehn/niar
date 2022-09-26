#include "DebugDraw.h"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Vulkan/VulkanUtils.h"

//................... points .......................

DebugPoints::DebugPoints(const DescriptorSetLayout& frameGlobalSetLayout, VkRenderPass compatiblePass, int compatibleSubpass)
{
	if (pipelineLayout == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE)
	{
		// build the pipeline
		auto vk = Vulkan::Instance;
		GraphicsPipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/debug_point.vert.spv";
		pipelineBuilder.fragPath = "spirv/debug_point.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = compatiblePass;
		pipelineBuilder.compatibleSubpass = compatibleSubpass;

		// input info
		pipelineBuilder.pipelineState.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		PointData::getBindingDescription(pipelineBuilder.pipelineState.bindingDescription);
		PointData::getAttributeDescriptions(pipelineBuilder.pipelineState.attributeDescriptions);
		pipelineBuilder.pipelineState.vertexInputInfo.vertexAttributeDescriptionCount = pipelineBuilder.pipelineState.attributeDescriptions.size();
		pipelineBuilder.pipelineState.rasterizationInfo.polygonMode = VK_POLYGON_MODE_POINT;

		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
}

void DebugPoints::bindAndDraw(VkCommandBuffer cmdbuf)
{
	if (points.empty()) return;

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	VkDeviceSize offsets[] = { 0 };
	auto vb = pointsBuffer.getBufferInstance();
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vb, offsets); // offset, #bindings, (content)
	vkCmdDraw(cmdbuf, points.size(), 1, 0, 0);
}

DebugPoints::~DebugPoints()
{
	pointsBuffer.release();
}

void DebugPoints::addPoint(const glm::vec3 &position, glm::u8vec4 color)
{
	points.emplace_back(position, color);
}

void DebugPoints::uploadVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(PointData) * points.size();
	if (bufferSize != pointsBuffer.numStrides * pointsBuffer.strideSize)
	{
		pointsBuffer.release();
		VmaBuffer stagingBuffer({
			&Vulkan::Instance->memoryAllocator, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU});
		stagingBuffer.writeData((void*)points.data());

		VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		pointsBuffer = VmaBuffer({
			&Vulkan::Instance->memoryAllocator, bufferSize, vkUsage, VMA_MEMORY_USAGE_GPU_ONLY, "Debug points vertex buffer"});

		vk::copyBuffer(pointsBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
		stagingBuffer.release();
	}
}

///////////////////// lines ///////////////////////

DebugLines::DebugLines(const DescriptorSetLayout& frameGlobalSetLayout, VkRenderPass compatiblePass, int compatibleSubpass)
{
	if (pipelineLayout == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE)
	{
		// build the pipeline
		auto vk = Vulkan::Instance;
		GraphicsPipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/debug_point.vert.spv";
		pipelineBuilder.fragPath = "spirv/debug_point.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = compatiblePass;
		pipelineBuilder.compatibleSubpass = compatibleSubpass;

		// input info
		pipelineBuilder.pipelineState.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		PointData::getBindingDescription(pipelineBuilder.pipelineState.bindingDescription);
		PointData::getAttributeDescriptions(pipelineBuilder.pipelineState.attributeDescriptions);
		pipelineBuilder.pipelineState.vertexInputInfo.vertexAttributeDescriptionCount = pipelineBuilder.pipelineState.attributeDescriptions.size();
		pipelineBuilder.pipelineState.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;

		pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);

		pipelineBuilder.build(pipeline, pipelineLayout);
	}
}

DebugLines::~DebugLines()
{
	pointsBuffer.release();
}

void DebugLines::bindAndDraw(VkCommandBuffer cmdbuf)
{
	if (points.empty()) return;

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	VkDeviceSize offsets[] = { 0 };
	auto vb = pointsBuffer.getBufferInstance();
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vb, offsets); // offset, #bindings, (content)
	vkCmdDraw(cmdbuf, points.size(), 1, 0, 0);
}

void DebugLines::addSegment(const PointData &endPoint1, const PointData &endPoint2)
{
	points.push_back(endPoint1);
	points.push_back(endPoint2);
}

void DebugLines::addBox(const glm::vec3 &minPos, const glm::vec3 &maxPos, glm::u8vec4 in_color)
{
	PointData aaa(minPos, in_color);
	PointData baa(glm::vec3(maxPos.x, minPos.y, minPos.z), in_color);
	PointData aba(glm::vec3(minPos.x, maxPos.y, minPos.z), in_color);
	PointData bba(glm::vec3(maxPos.x, maxPos.y, minPos.z), in_color);
	PointData aab(glm::vec3(minPos.x, minPos.y, maxPos.z), in_color);
	PointData bab(glm::vec3(maxPos.x, minPos.y, maxPos.z), in_color);
	PointData abb(glm::vec3(minPos.x, maxPos.y, maxPos.z), in_color);
	PointData bbb(maxPos, in_color);

	addSegment(aaa, baa);
	addSegment(aaa, aba);
	addSegment(bba, baa);
	addSegment(bba, aba);

	addSegment(aab, bab);
	addSegment(aab, abb);
	addSegment(bbb, bab);
	addSegment(bbb, abb);

	addSegment(aaa, aab);
	addSegment(bba, bbb);
	addSegment(baa, bab);
	addSegment(aba, abb);
}

void DebugLines::uploadVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(PointData) * points.size();
	Vulkan::Instance->waitDeviceIdle();
	pointsBuffer.release();
	VmaBuffer stagingBuffer({
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU});
	stagingBuffer.writeData((void*)points.data());

	VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	pointsBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY,
		"Debug lines vertex buffer"});

	vk::copyBuffer(pointsBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}
