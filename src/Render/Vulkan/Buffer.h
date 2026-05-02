#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
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
		uint32_t numStrides = 1;
	};
	VmaBuffer() = default;
	VmaBuffer(const CreateInfo &createInfo);

	void writeData(void* inData, size_t writeSize, uint32_t strideIndex = 0);

	VkBuffer getBufferInstance() const;

	void release();

	VkDeviceSize strideSize = 0;
	uint32_t numStrides = 0;

private:
	VmaAllocator* allocator = nullptr;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo = {};

	VmaAllocationInfo getAllocationInfo() const;
};
