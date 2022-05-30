#pragma once
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vector>

class VmaBuffer
{
public:
	VmaBuffer() = default;
	VmaBuffer(VmaAllocator *allocator,
			  VkDeviceSize strideSize,
			  VkBufferUsageFlags bufferUsage,
			  VmaMemoryUsage memoryUsage,
			  uint32_t numInstances=1,
			  uint32_t numStrides=1);

	void writeData(void* inData, size_t writeSize = 0, size_t bufferIndex = 0, uint32_t strideIndex = 0);

	VkBuffer getBufferInstance(uint32_t index = 0) const;
	VmaAllocation getAllocationInstance(uint32_t index = 0) const;

	void release();

	VkDeviceSize strideSize = 0;
	uint32_t numInstances = 0;
	uint32_t numStrides = 0;

private:
	VmaAllocator* allocator = nullptr;
	std::vector<VkBuffer> buffers;
	std::vector<VmaAllocation> allocations;
};
