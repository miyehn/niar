#pragma once
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vector>

class VmaBuffer
{
public:
	VmaBuffer() = default;
	VmaBuffer(VmaAllocator *allocator,
			  VkDeviceSize size,
			  VkBufferUsageFlags bufferUsage,
			  VmaMemoryUsage memoryUsage,
			  uint32_t numInstances=1);

	void writeData(void* inData, uint32_t bufferIndex = 0, size_t writeSize = 0);

	uint32_t getNumInstances() const { return numInstances; }
	VkBuffer getBufferInstance(uint32_t index = 0) const;
	VmaAllocation getAllocationInstance(uint32_t index = 0) const;

	void release();

private:
	VmaAllocator* allocator = nullptr;
	VkDeviceSize size = 0;
	uint32_t numInstances = 0;
	std::vector<VkBuffer> buffers;
	std::vector<VmaAllocation> allocations;
};
