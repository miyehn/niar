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
		VkDeviceSize minAllocationAlignment = 0;
	};
	VmaBuffer() = default;
	VmaBuffer(const CreateInfo &createInfo);

	void writeData(void* inData, size_t writeSize, uint32_t strideIndex = 0);
	void readData(void* outData, size_t readSize, uint32_t strideIndex = 0) const;

	void release();

	VkDeviceAddress getDeviceAddress() const;

	VkDeviceSize strideSize = 0;
	uint32_t numStrides = 0;
	VkBuffer buffer = VK_NULL_HANDLE;

private:
	VmaAllocator* allocator = nullptr;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo = {};
	mutable VkDeviceAddress deviceAddress = 0;

	VmaAllocationInfo getAllocationInfo() const;
};
