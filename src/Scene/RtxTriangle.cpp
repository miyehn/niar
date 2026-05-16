//
// Created by raind on 1/29/2022.
//

#include "RtxTriangle.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Mesh.h"
#include "Utils/myn/Misc.h"

void RtxTriangle::create_vertex_buffer()
{
	const float vertices[9] = {
		0.25f, 0.25f, 0.0f,
		0.75f, 0.25f, 0.0f,
		0.5f, 0.75f, 0.0f,
	};

	VkDeviceSize bufferSize = sizeof(float) * 9;

	// create a staging buffer
	VmaBuffer stagingBuffer({&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU});

	// copy vertex buffer memory over to staging buffer
	stagingBuffer.writeData((void *) vertices, bufferSize);

	// now create the actual vertex buffer
	VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	vertexBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY,
		"RtxTriangle vertex buffer"});

	// and copy stuff from staging buffer to vertex buffer
	vk::copyBuffer(vertexBuffer.buffer, stagingBuffer.buffer, bufferSize);
	stagingBuffer.release();
}

void RtxTriangle::create_index_buffer()
{
	const VERTEX_INDEX_TYPE indices[3] = { 0, 1, 2 };

	VkDeviceSize bufferSize = sizeof(VERTEX_INDEX_TYPE) * 3;

	// create a staging buffer
	VmaBuffer stagingBuffer({&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU});

	// copy data to staging buffer
	stagingBuffer.writeData((void*)indices, bufferSize);

	// create the actual index buffer
	VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	indexBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY});

	// move stuff from staging buffer and destroy staging buffer
	vk::copyBuffer(indexBuffer.buffer, stagingBuffer.buffer, bufferSize);
	stagingBuffer.release();
}

static void buildBlas(
	VkAccelerationStructureGeometryKHR geom,
	VkAccelerationStructureBuildRangeInfoKHR range,
	VkBuildAccelerationStructureFlagsKHR flags,
	VkAccelerationStructureKHR* outBlas,
	VmaBuffer* outBlasBuffer)
{
	const uint32_t maxPrimitivesCount = 1;

	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = flags,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1,
		.pGeometries = &geom,
	};

	// buildScratchSize, updateScratchSize, accelerationStructureSize
	VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	Vulkan::Instance->fn_vkGetAccelerationStructureBuildSizesKHR(
		Vulkan::Instance->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, &maxPrimitivesCount, &buildSizesInfo);

	// create scratch buffer of size buildScratchSize, get its device address
	VmaBuffer scratchBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		buildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY});
	VkDeviceAddress scratchAddress = scratchBuffer.getDeviceAddress();

	// for compaction
	VkQueryPool queryPool{VK_NULL_HANDLE};
	VkQueryPoolCreateInfo qpCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
		.queryCount = 1
	};
	vkCreateQueryPool(Vulkan::Instance->device, &qpCreateInfo, nullptr, &queryPool);
	vkResetQueryPool(Vulkan::Instance->device, queryPool, 0, 1);

	//======== actual alloc of buffer and accel structure ========

	// pt 1 : alloc AS buffer and create it
	VmaBuffer stagingBlasBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		buildSizesInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY});
	// aka call to vkCreateAccelerationStructureKHR will create a blas to the staging buffer and occupy this amount of data
	// result will be the form of a handle
	// but this blas will be uninitialized with geometry data yet, still "all 0s"?
	VkAccelerationStructureCreateInfoKHR asCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = stagingBlasBuffer.buffer,
		.size = buildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	VkAccelerationStructureKHR stagingBlas;
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &asCreateInfo, nullptr, &stagingBlas);

	// pt 2 : build (using scratch memory), also query how much mem it'll take after compaction
	buildGeometryInfo.dstAccelerationStructure = stagingBlas;
	buildGeometryInfo.scratchData.deviceAddress = scratchAddress;
	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		auto rangePtr = &range;
		Vulkan::Instance->fn_vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &buildGeometryInfo, &rangePtr);

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
			1, // accl structure count
			&stagingBlas,
			VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
			queryPool,
			0);
	});
	Vulkan::Instance->waitDeviceIdle();

	// pt3 : compaction (writes to actual output)
	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		VkDeviceSize compactSize = 0; // need to zero out because vkGetQueryPoolResults doesn't actually overwrite all bits..
		EXPECT(vkGetQueryPoolResults(
			Vulkan::Instance->device,
			queryPool,
			0,
			1,
			sizeof(VkDeviceSize),
			&compactSize,
			sizeof(VkDeviceSize),
			VK_QUERY_RESULT_WAIT_BIT), VK_SUCCESS);

		*outBlasBuffer = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			compactSize,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			VMA_MEMORY_USAGE_GPU_ONLY,
			"RtxTriangle blas buffer"});

		VkAccelerationStructureCreateInfoKHR compactInfo = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
			.buffer = outBlasBuffer->buffer,
			.size = compactSize,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		};
		Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &compactInfo, nullptr, outBlas);

		VkCopyAccelerationStructureInfoKHR copyInfo = {
			.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR,
			.src = stagingBlas,
			.dst = *outBlas,
			.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR
		};
		Vulkan::Instance->fn_vkCmdCopyAccelerationStructureKHR(cmdbuf, &copyInfo);
	});
	Vulkan::Instance->waitDeviceIdle();

	vkDestroyQueryPool(Vulkan::Instance->device, queryPool, nullptr);
	scratchBuffer.release();
	stagingBlasBuffer.release();
	Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, stagingBlas, nullptr);
}

RtxTriangle::RtxTriangle() 
{
	name = "RTX Triangle";
	create_vertex_buffer();
	create_index_buffer();

	// BLAS

	VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		.vertexData = {
			.deviceAddress = vertexBuffer.getDeviceAddress(),
		},
		.vertexStride = sizeof(float) * 3,
		.maxVertex = 3,
		.indexType = VK_INDEX_TYPE,
		.indexData = {
			.deviceAddress = indexBuffer.getDeviceAddress(),
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

	VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {
		.primitiveCount = 1,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0
	};

	buildBlas(asGeom, rangeInfo,
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