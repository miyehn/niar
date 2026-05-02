#include "Buffer.h"
#include "VulkanUtils.h"
#include "Utils/myn/Log.h"

VmaBuffer::VmaBuffer(const CreateInfo &info) :
	allocator(info.allocator),
	numStrides(info.numStrides)
{
	VkDeviceSize alignmentReq = Vulkan::Instance->minUniformBufferOffsetAlignment;
	uint32_t numBlocks = (info.strideSize + alignmentReq - 1) / alignmentReq;
	strideSize = numBlocks * alignmentReq;

	VkBufferCreateInfo bufferCreateInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = strideSize * numStrides,
		.usage = info.bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	const bool hostVisible = (info.memoryUsage & VMA_MEMORY_USAGE_CPU_ONLY) || (info.memoryUsage & VMA_MEMORY_USAGE_CPU_TO_GPU);
	VmaAllocationCreateFlags createFlags = hostVisible ?
		(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) : 0;
	VmaAllocationCreateInfo vmaAllocCreateInfo {
		.flags = createFlags,
		.usage = info.memoryUsage,
	};
	EXPECT(vmaCreateBuffer(
		*allocator,
		&bufferCreateInfo,
		&vmaAllocCreateInfo,
		&buffer,
		&allocation,
		&allocationInfo), VK_SUCCESS);
	if (info.debugName.length() > 0) {
		NAME_OBJECT(VK_OBJECT_TYPE_BUFFER, buffer, info.debugName)
	}
}

void VmaBuffer::release()
{
	if (buffer != nullptr || allocator != nullptr) {
		vmaDestroyBuffer(*allocator, buffer, allocation);
	}
	allocator = nullptr;
}

VkBuffer VmaBuffer::getBufferInstance() const {
	ASSERT(allocator != nullptr)
	return buffer;
}

VmaAllocationInfo VmaBuffer::getAllocationInfo() const {
	ASSERT(allocator != nullptr)
	return allocationInfo;
}

void VmaBuffer::writeData(void *inData, size_t writeSize, uint32_t strideIndex)
{
	ASSERT(allocator != nullptr)
	if (writeSize == 0) writeSize = (size_t)strideSize;
	auto dstAddress = (uint8_t*) getAllocationInfo().pMappedData + (strideIndex * strideSize);
	ASSERT(dstAddress != nullptr)
	memcpy(dstAddress, inData, writeSize);
}
