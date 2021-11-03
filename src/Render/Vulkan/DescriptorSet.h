#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "Buffer.h"

#define DSET_FRAMEGLOBAL 0
#define DSET_DYNAMIC 3

class DescriptorSetLayout
{
public:
	void addBinding(uint32_t bindingIndex, VkShaderStageFlags shaderStages, VkDescriptorType type);
	VkDescriptorSetLayout getLayout();

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
};

class DescriptorSet
{
public:
	DescriptorSet() = default;

	DescriptorSet(DescriptorSetLayout &layout, uint32_t numInstances = 1);

	VkDescriptorSet getInstance(uint32_t index = 0) const { return descriptorSets[index]; }

	void pointToUniformBuffer(VmaBuffer &uniformBuffer, uint32_t binding);

	void pointToImageView(VkImageView imageView, uint32_t binding, VkDescriptorType descriptorType);

	void bind(VkCommandBuffer cmdbuf, uint32_t setIndex, VkPipelineLayout pipelineLayout, uint32_t instanceId = 0);

	DescriptorSetLayout getLayout() { return layout; }

	static void releasePool();

private:

	DescriptorSetLayout layout;

	std::vector<VkDescriptorSet> descriptorSets;

	static VkDescriptorPool descriptorPool;
};

class DescriptorSetLayoutCache
{
protected:
	static std::unordered_map<VkDescriptorSetLayoutCreateInfo, VkDescriptorSetLayout> pool;

public:
	static VkDescriptorSetLayout get(VkDescriptorSetLayoutCreateInfo& createInfo);
	static void cleanup();
};