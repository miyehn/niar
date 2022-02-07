//
// Created by raind on 1/29/2022.
//

#include "RtxTriangle.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Mesh.h"

void RtxTriangle::create_vertex_buffer()
{
	const float vertices[9] = {
		0.25f, 0.25f, 0.0f,
		0.75f, 0.25f, 0.0f,
		0.5f, 0.75f, 0.0f,
	};

	VkDeviceSize bufferSize = sizeof(float) * 9;

	// create a staging buffer
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// copy vertex buffer memory over to staging buffer
	stagingBuffer.writeData((void *) vertices);

	// now create the actual vertex buffer
	auto vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	vertexBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// and copy stuff from staging buffer to vertex buffer
	vk::copyBuffer(vertexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void RtxTriangle::create_index_buffer()
{
	const VERTEX_INDEX_TYPE indices[3] = { 0, 1, 2 };

	VkDeviceSize bufferSize = sizeof(VERTEX_INDEX_TYPE) * 3;

	// create a staging buffer
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// copy data to staging buffer
	stagingBuffer.writeData((void*)indices);

	// create the actual index buffer
	auto vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	indexBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// move stuff from staging buffer and destroy staging buffer
	vk::copyBuffer(indexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void RtxTriangle::draw(VkCommandBuffer cmdbuf)
{
	SceneObject::draw(cmdbuf);
}

void buildBlas(
	VkAccelerationStructureGeometryKHR geom,
	VkAccelerationStructureBuildRangeInfoKHR range,
	VkBuildAccelerationStructureFlagsKHR flags,
	VkAccelerationStructureKHR* outBlas,
	VmaBuffer* blasBuffer)
{
	VkDeviceSize structureSize{0};
	VkDeviceSize scratchSize{0};
	const uint32_t maxPrimitivesCount = 1;

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = flags,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1,
		.pGeometries = &geom,
	};

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

	Vulkan::Instance->fn_vkGetAccelerationStructureBuildSizesKHR(
		Vulkan::Instance->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &maxPrimitivesCount, &sizeInfo);

	VmaBuffer scratchBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
	//VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, )
	const VkBufferDeviceAddressInfo scratchBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.getBufferInstance()
	};
	VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &scratchBufferAddressInfo);

	// for compaction
	VkQueryPool queryPool{VK_NULL_HANDLE};
	VkQueryPoolCreateInfo qpCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
		.queryCount = 1
	};
	vkCreateQueryPool(Vulkan::Instance->device, &qpCreateInfo, nullptr, &queryPool);
	vkResetQueryPool(Vulkan::Instance->device, queryPool, 0, 1);
	uint32_t queryCnt{0};

	//======== actual alloc of buffer and accel structure ========

	// pt 1 : alloc AS buffer and create it
	*blasBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY);
	VkAccelerationStructureCreateInfoKHR asCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blasBuffer->getBufferInstance(),
		.size = sizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &asCreateInfo, nullptr, outBlas);

	// pt 2 : build (using scratch memory)
	buildInfo.dstAccelerationStructure = *outBlas;
	buildInfo.scratchData.deviceAddress = scratchAddress;
	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		auto rangePtr = &range;
		Vulkan::Instance->fn_vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &buildInfo, &rangePtr);

		VkMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
		};
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0,
			1,
			&barrier,
			0,
			nullptr,
			0,
			nullptr
			);
		Vulkan::Instance->fn_vkCmdWriteAccelerationStructuresPropertiesKHR(
			cmdbuf,
			1,
			outBlas,
			VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
			queryPool,
			queryCnt);
		queryCnt++;
	});

	Vulkan::Instance->waitDeviceIdle();

	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{

	});

	vkDestroyQueryPool(Vulkan::Instance->device, queryPool, nullptr);
	scratchBuffer.release();
}

RtxTriangle::RtxTriangle()
{
	name = "RTX Triangle";
	create_vertex_buffer();
	create_index_buffer();

	auto vBufferRaw = vertexBuffer.getBufferInstance();
	auto iBufferRaw = indexBuffer.getBufferInstance();

	const VkBufferDeviceAddressInfo vBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vBufferRaw
	};
	const VkBufferDeviceAddressInfo iBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = iBufferRaw
	};
	VkDeviceAddress vBufferAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &vBufferAddressInfo);
	VkDeviceAddress iBufferAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &iBufferAddressInfo);

	uint32_t maxPrimitivesCount = 1;

	VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		.vertexData = {
			.deviceAddress = vBufferAddress
		},
		.vertexStride = sizeof(float) * 3,
		.maxVertex = 3,
		.indexType = VK_INDEX_TYPE,
		.indexData = {
			.deviceAddress = iBufferAddress
		},
	};

	VkAccelerationStructureGeometryKHR asGeom = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry = {
			.triangles = triangles
		},
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildRangeInfoKHR offset = {
		.primitiveCount = maxPrimitivesCount,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0
	};

	buildBlas(asGeom, offset,
			  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR |
			  VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
			  &blas, &blasBuffer);
}

RtxTriangle::~RtxTriangle()
{
	Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, blas, nullptr);
	blasBuffer.release();
	vertexBuffer.release();
	indexBuffer.release();
}
