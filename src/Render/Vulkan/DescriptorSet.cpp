#include "DescriptorSet.h"
#include "Utils/myn/Log.h"

VkDescriptorPool DescriptorSet::descriptorPool = VK_NULL_HANDLE;

void DescriptorSet::releasePool(VkDevice &device)
{
	if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

DescriptorSet::DescriptorSet(VkDevice &device, VkDescriptorSetLayout &layout, uint32_t numInstances)
: device(device), numInstances(numInstances)
{
	// create the pool first if it isn't created yet
	if (descriptorPool == VK_NULL_HANDLE)
	{
		VkDescriptorPoolSize poolSize = {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = numInstances,
		};
		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = static_cast<uint32_t>(numInstances),
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		EXPECT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), VK_SUCCESS)
	}

	std::vector<VkDescriptorSetLayout> layouts(numInstances, layout);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
		.pSetLayouts = layouts.data()
	};
	descriptorSets.resize(layouts.size());
	EXPECT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()), VK_SUCCESS)
}

void DescriptorSet::pointToUniformBuffer(VmaBuffer &uniformBuffer, uint32_t binding)
{
	EXPECT(numInstances != 0, true)
	EXPECT(uniformBuffer.getNumInstances(), numInstances)
	for (auto i = 0; i < numInstances; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = uniformBuffer.getBufferInstance(i),
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		};
		// "Structure specifying the parameters of a descriptor set write operation"
		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			// actual write data (one of three)
			.pImageInfo = nullptr,
			.pBufferInfo = &bufferInfo, // where in which buffer
			.pTexelBufferView = nullptr,
		};
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}
