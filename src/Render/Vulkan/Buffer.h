#pragma once
#include <VulkanMemoryAllocator-3.0.1/include/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VmaBuffer
{
public:
	struct CreateInfo {
		VmaAllocator *allocator = nullptr;
		VkDeviceSize strideSize = 0;
		VkBufferUsageFlags bufferUsage = 0;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
		// below: optional
		std::string debugName;
		uint32_t numInstances = 1;
		uint32_t numStrides = 1;
	};
	VmaBuffer() = default;
	VmaBuffer(const CreateInfo &createInfo);

	void writeData(void* inData, size_t writeSize = 0, size_t bufferIndex = 0, uint32_t strideIndex = 0);

	VkBuffer getBufferInstance(uint32_t index = 0) const;

	void release();

	VkDeviceSize strideSize = 0;
	uint32_t numInstances = 0;
	uint32_t numStrides = 0;

private:
	VmaAllocator* allocator = nullptr;
	std::vector<VkBuffer> buffers;
	std::vector<VmaAllocation> allocations;
	std::vector<VmaAllocationInfo> allocationInfos;

	VmaAllocationInfo getAllocationInfo(uint32_t index = 0) const;
};
