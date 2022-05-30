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

	explicit DescriptorSet(DescriptorSetLayout &layout, uint32_t numInstances = 1);

	VkDescriptorSet getInstance(uint32_t index = 0) const { return descriptorSets[index]; }

	void pointToBuffer(const VmaBuffer &buffer, uint32_t binding, VkDescriptorType descriptorType);

	void pointToImageView(VkImageView imageView, uint32_t binding, VkDescriptorType descriptorType);

	void pointToRWImageView(VkImageView imageView, uint32_t binding);

	void pointToAccelerationStructure(VkAccelerationStructureKHR accelerationStructure, uint32_t binding);

	void bind(
		VkCommandBuffer cmdbuf,
		VkPipelineBindPoint pipelineBindPoint,
		uint32_t setIndex,
		VkPipelineLayout pipelineLayout,
		uint32_t instanceId = 0,
		uint32_t numDynamicOffsets = 0,
		const uint32_t* pDynamicOffsets = nullptr);

	DescriptorSetLayout getLayout() { return layout; }

private:

	DescriptorSetLayout layout;

	std::vector<VkDescriptorSet> descriptorSets;

	static VkDescriptorPool descriptorPool;
};