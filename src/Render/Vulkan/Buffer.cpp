#include "Buffer.h"
#include "VulkanUtils.h"
#include "Utils/myn/Log.h"

VmaBuffer::VmaBuffer(const CreateInfo &info) :
	allocator(info.allocator),
	numInstances(info.numInstances),
	strideSize(info.strideSize),
	numStrides(info.numStrides)
{
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
	buffers.resize(numInstances);
	allocations.resize(numInstances);
	allocationInfos.resize(numInstances);
	for (auto i = 0; i < numInstances; i++)
	{
		EXPECT(vmaCreateBuffer(
			*allocator,
			&bufferCreateInfo,
			&vmaAllocCreateInfo,
			&buffers[i],
			&allocations[i],
			&allocationInfos[i]), VK_SUCCESS);
		if (info.debugName.length() > 0) {
			std::string instanceName = info.debugName + " (" + std::to_string(i+1) + "/" + std::to_string(numInstances) + ")";
			NAME_OBJECT(VK_OBJECT_TYPE_BUFFER, buffers[i], instanceName)
		}
	}
}

void VmaBuffer::release()
{
	for (auto i = 0; i < numInstances; i++)
	{
		vmaDestroyBuffer(*allocator, buffers[i], allocations[i]);
	}
	allocator = nullptr;
}

VkBuffer VmaBuffer::getBufferInstance(uint32_t index) const {
	ASSERT(allocator != nullptr)
	return buffers[index];
}

VmaAllocationInfo VmaBuffer::getAllocationInfo(uint32_t index) const {
	ASSERT(allocator != nullptr)
	return allocationInfos[index];
}

void VmaBuffer::writeData(void *inData, size_t writeSize, size_t bufferIndex, uint32_t strideIndex)
{
	ASSERT(allocator != nullptr)
	if (writeSize == 0) writeSize = (size_t)strideSize;
	auto dstAddress = (uint8_t*) getAllocationInfo(bufferIndex).pMappedData + (strideIndex * strideSize);
	ASSERT(dstAddress != nullptr)
	memcpy(dstAddress, inData, writeSize);
}
