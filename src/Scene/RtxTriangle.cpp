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
	auto vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
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
	auto vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
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

void buildBlas(VkAccelerationStructureGeometryKHR geom, VkAccelerationStructureBuildRangeInfoKHR range, VkBuildAccelerationStructureFlagsKHR flags)
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
			  VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
			  );
}

RtxTriangle::~RtxTriangle()
{
	vertexBuffer.release();
	indexBuffer.release();
}
