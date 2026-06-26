#pragma once
#include <vulkan/vulkan.h>
#include <span>
#include <vector>
#include "Buffer.h"

class DescriptorSetLayout
{
public:
	void addBinding(
		uint32_t bindingIndex,
		VkShaderStageFlags shaderStages,
		VkDescriptorType type,
		uint32_t descriptorCount = 1);
	VkDescriptorSetLayout getLayout();

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
};

class DescriptorSet
{
public:
	DescriptorSet() = default;

	explicit DescriptorSet(
		DescriptorSetLayout& layout,
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE);

	VkDescriptorSet get() const { return descriptorSet; }

	void pointToBuffer(const VmaBuffer &buffer, uint32_t binding, VkDescriptorType descriptorType);

	void pointToImageView(
		VkImageView imageView,
		uint32_t binding,
		const VkSamplerCreateInfo* samplerInfoPtr = nullptr);

	void pointToImageViews(
		uint32_t binding,
		uint32_t firstArrayElement,
		VkDescriptorType descriptorType,
		std::span<const VkDescriptorImageInfo> imageInfos);

	void pointToRWImageView(VkImageView imageView, uint32_t binding);

	void pointToAccelerationStructure(VkAccelerationStructureKHR accelerationStructure, uint32_t binding);

	void bind(
		VkCommandBuffer cmdbuf,
		VkPipelineBindPoint pipelineBindPoint,
		uint32_t setIndex,
		VkPipelineLayout pipelineLayout,
		uint32_t numDynamicOffsets = 0,
		const uint32_t* pDynamicOffsets = nullptr) const;

	DescriptorSetLayout getLayout() const { return layout; }

private:

	DescriptorSetLayout layout;

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	static VkDescriptorPool descriptorPool;
};
