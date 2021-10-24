#include "DescriptorSet.h"
#include "Utils/myn/Log.h"
#include "Render/Vulkan/Sampler.h"
#include "Render/Vulkan/Vulkan.hpp"

VkDescriptorPool DescriptorSet::descriptorPool = VK_NULL_HANDLE;

void DescriptorSet::releasePool(VkDevice &device)
{
	if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

DescriptorSet::DescriptorSet(VkDevice &device, DescriptorSetLayout &layout, uint32_t numInstances)
: device(device), numInstances(numInstances), layout(layout)
{
	// create the pool first if it isn't created yet
	if (descriptorPool == VK_NULL_HANDLE)
	{
		// TODO: make more reliable
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
		};
		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = static_cast<uint32_t>(10),
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};
		EXPECT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), VK_SUCCESS)
	}

	std::vector<VkDescriptorSetLayout> layouts(numInstances, layout.layout);
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

void DescriptorSet::pointToImageView(VkImageView imageView, uint32_t binding, VkDescriptorType descriptorType)
{
	EXPECT(numInstances != 0, true)

	for (auto i = 0; i < numInstances; i++)
	{
		VkSamplerCreateInfo samplerInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		};
		VkSampler sampler = Sampler::get(samplerInfo);

		VkDescriptorImageInfo imageInfo = {
			.sampler = sampler,
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// "Structure specifying the parameters of a descriptor set write operation"
		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = descriptorType,
			// actual write data (one of three)
			.pImageInfo = &imageInfo,
			.pBufferInfo = nullptr,
			.pTexelBufferView = nullptr,
		};
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}
