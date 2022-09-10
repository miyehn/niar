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
	VmaAllocationCreateInfo vmaAllocCreateInfo {
		.usage = info.memoryUsage
	};
	buffers.resize(numInstances);
	allocations.resize(numInstances);
	for (auto i = 0; i < numInstances; i++)
	{
		EXPECT(vmaCreateBuffer(
			*allocator,
			&bufferCreateInfo,
			&vmaAllocCreateInfo,
			&buffers[i],
			&allocations[i],
			nullptr), VK_SUCCESS);
		if (info.debugName.length() > 0) {
			std::string instanceName = info.debugName + "(" + std::to_string(i) + "/" + std::to_string(numInstances) + ")";
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

VkBuffer VmaBuffer::getBufferInstance(uint32_t index) const
{
	EXPECT(allocator == nullptr, false)
	return buffers[index];
}

VmaAllocation VmaBuffer::getAllocationInstance(uint32_t index) const
{
	EXPECT(allocator == nullptr, false)
	return allocations[index];
}

void VmaBuffer::writeData(void *inData, size_t writeSize, size_t bufferIndex, uint32_t strideIndex)
{
	EXPECT(allocator == nullptr, false)
	if (writeSize == 0) writeSize = (size_t)strideSize;
	void* mappedMemory;
	vmaMapMemory(*allocator, getAllocationInstance(bufferIndex), &mappedMemory);
	memcpy((uint8_t*)mappedMemory + strideIndex * strideSize, inData, writeSize);
	vmaUnmapMemory(*allocator, getAllocationInstance(bufferIndex));
}
