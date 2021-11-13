#include "DescriptorSet.h"
#include "Utils/myn/Log.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/Vulkan.hpp"

void DescriptorSetLayout::addBinding(uint32_t bindingIndex, VkShaderStageFlags shaderStages, VkDescriptorType type)
{
	EXPECT_M(layout, VK_NULL_HANDLE, "Should only add bindings before layout is committed")
	if (bindings.size() <= bindingIndex) bindings.resize(bindingIndex + 1);
	bindings[bindingIndex] = {
		.binding = bindingIndex,
		.descriptorType = type,
		.descriptorCount = 1,
		.stageFlags = shaderStages,
		.pImmutableSamplers = nullptr,
	};
}

VkDescriptorSetLayout DescriptorSetLayout::getLayout()
{
	if (layout == VK_NULL_HANDLE)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data(),
		};
		layout = DescriptorSetLayoutCache::get(layoutInfo);
	}
	return layout;
}

VkDescriptorPool DescriptorSet::descriptorPool = VK_NULL_HANDLE;

void DescriptorSet::releasePool()
{
	if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(Vulkan::Instance->device, descriptorPool, nullptr);
}

DescriptorSet::DescriptorSet(DescriptorSetLayout &layout, uint32_t numInstances) : layout(layout)
{
	// create the pool first if it isn't created yet
	if (descriptorPool == VK_NULL_HANDLE)
	{
		// TODO: make more reliable
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 20 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 }
		};
		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = static_cast<uint32_t>(20),
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};
		EXPECT(vkCreateDescriptorPool(Vulkan::Instance->device, &poolInfo, nullptr, &descriptorPool), VK_SUCCESS)
	}

	std::vector<VkDescriptorSetLayout> layouts(numInstances, layout.getLayout());
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
		.pSetLayouts = layouts.data()
	};
	descriptorSets.resize(layouts.size());
	EXPECT(vkAllocateDescriptorSets(Vulkan::Instance->device, &allocInfo, descriptorSets.data()), VK_SUCCESS)
}

void DescriptorSet::pointToBuffer(VmaBuffer &buffer, uint32_t binding, VkDescriptorType descriptorType)
{
	uint32_t numInstances = descriptorSets.size();
	EXPECT(numInstances != 0, true)
	EXPECT(buffer.numInstances, numInstances)

	for (auto i = 0; i < numInstances; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = buffer.getBufferInstance(i),
			.offset = 0,
			.range = buffer.strideSize
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
			.pImageInfo = nullptr,
			.pBufferInfo = &bufferInfo, // where in which buffer
			.pTexelBufferView = nullptr,
		};
		vkUpdateDescriptorSets(Vulkan::Instance->device, 1, &descriptorWrite, 0, nullptr);
	}
}

void DescriptorSet::pointToImageView(VkImageView imageView, uint32_t binding, VkDescriptorType descriptorType)
{
	uint32_t numInstances = descriptorSets.size();
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
		VkSampler sampler = SamplerCache::get(samplerInfo);

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
		vkUpdateDescriptorSets(Vulkan::Instance->device, 1, &descriptorWrite, 0, nullptr);
	}
}

void DescriptorSet::bind(
	VkCommandBuffer cmdbuf,
	uint32_t setIndex,
	VkPipelineLayout pipelineLayout,
	uint32_t instanceId,
	uint32_t numDynamicOffsets,
	const uint32_t* pDynamicOffsets)
{
	vkCmdBindDescriptorSets(
		cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		setIndex, // firstSet : uint32_t
		1, // descriptorSetCount : uint32_t
		&descriptorSets[instanceId],
		numDynamicOffsets,
		pDynamicOffsets);
}

//================ descriptor set layout cache =================

bool operator==(const VkDescriptorSetLayoutCreateInfo &info1, const VkDescriptorSetLayoutCreateInfo &info2)
{
	if (info1.bindingCount != info2.bindingCount) return false;
	for (auto i = 0; i < info1.bindingCount; i++)
	{
		auto& binding1 = info1.pBindings[i];
		auto& binding2 = info2.pBindings[i];

		if (binding1.binding != binding2.binding) return false;
		if (binding1.descriptorType != binding2.descriptorType) return false;
		if (binding1.descriptorCount != binding2.descriptorCount) return false;
		if (binding1.stageFlags != binding2.stageFlags) return false;
		// not considering pImmutableSamplers yet
	}
	return true;
}

namespace std
{
	template<> struct hash<VkDescriptorSetLayoutBinding>
	{
		std::size_t operator()(const VkDescriptorSetLayoutBinding &binding) const noexcept
		{
			size_t hashValue = hash<int>{}(binding.binding << 0)	 ^
				hash<int>{}(binding.descriptorType << 8) ^
				hash<int>{}(binding.descriptorCount << 16) ^
				hash<int>{}(binding.stageFlags << 24);
			return hashValue;
		}
	};

	template<> struct hash<VkDescriptorSetLayoutCreateInfo>
	{
		std::size_t operator()(const VkDescriptorSetLayoutCreateInfo &info) const noexcept
		{
			size_t hashValue = 0;
			for (auto i = 0; i < info.bindingCount; i++)
			{
				hashValue ^= hash<VkDescriptorSetLayoutBinding>{}(info.pBindings[i]);
			}
			return hashValue;
		}
	};
}

std::unordered_map<VkDescriptorSetLayoutCreateInfo, VkDescriptorSetLayout> DescriptorSetLayoutCache::pool;

VkDescriptorSetLayout DescriptorSetLayoutCache::get(VkDescriptorSetLayoutCreateInfo &createInfo)
{
	auto it = pool.find(createInfo);
	if (it != pool.end()) {
		return (*it).second;
	}

	VkDescriptorSetLayout layout;
	EXPECT(vkCreateDescriptorSetLayout(Vulkan::Instance->device, &createInfo, nullptr, &layout), VK_SUCCESS)
	pool[createInfo] = layout;
	return layout;
}

void DescriptorSetLayoutCache::cleanup()
{
	for (auto & it : DescriptorSetLayoutCache::pool)
	{
		vkDestroyDescriptorSetLayout(Vulkan::Instance->device, it.second, nullptr);
	}
}
