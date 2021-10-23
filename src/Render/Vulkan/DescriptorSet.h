#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Buffer.h"

class DescriptorSet
{
public:
	DescriptorSet() = default;
	DescriptorSet(VkDevice &device, VkDescriptorSetLayout &layouts, uint32_t numInstances = 1);

	VkDescriptorSet getInstance(uint32_t index = 0) const { return descriptorSets[index]; }
	uint32_t getNumInstances() const { return numInstances; }

	void pointToUniformBuffer(VmaBuffer &uniformBuffer, uint32_t binding);

	void pointToImageView(VkImageView imageView, uint32_t binding, VkDescriptorType descriptorType);

	static void releasePool(VkDevice &device);

private:
	VkDevice device = VK_NULL_HANDLE;
	uint32_t numInstances = 0;
	std::vector<VkDescriptorSet> descriptorSets;

	static VkDescriptorPool descriptorPool;
};

// TODO
class DescriptorSetLayoutCache
{
public:

};