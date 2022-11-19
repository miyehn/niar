#include "DescriptorSet.h"
#include "Utils/myn/Log.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"

//================ descriptor set layout cache =================

#define LOG_DESCRIPTORSETLAYOUT_CACHE 0

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

// when storing into set layout cache, make a deep copy of everything
class CachedDescriptorSetLayoutInfo
{
public:
	explicit CachedDescriptorSetLayoutInfo(VkDescriptorSetLayoutCreateInfo inInfo) : info(inInfo), layout(VK_NULL_HANDLE)
	{
		for (auto i = 0; i < inInfo.bindingCount; i++)
		{
			bindings.push_back(inInfo.pBindings[i]);
		}
		info.pBindings = bindings.data();

		EXPECT(vkCreateDescriptorSetLayout(Vulkan::Instance->device, &info, nullptr, &layout), VK_SUCCESS)
		Vulkan::Instance->destructionQueue.emplace_back([this](){
			vkDestroyDescriptorSetLayout(Vulkan::Instance->device, layout, nullptr);
		});
	}
	VkDescriptorSetLayoutCreateInfo info;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout;
};

// the pool is actually not properly cleaned up, but whatever...
class DescriptorSetLayoutCache
{
protected:
	static std::vector<CachedDescriptorSetLayoutInfo*> pool;

public:
	static VkDescriptorSetLayout get(VkDescriptorSetLayoutCreateInfo& createInfo);
};

std::vector<CachedDescriptorSetLayoutInfo*> DescriptorSetLayoutCache::pool;

VkDescriptorSetLayout DescriptorSetLayoutCache::get(VkDescriptorSetLayoutCreateInfo &createInfo)
{
#if LOG_DESCRIPTORSETLAYOUT_CACHE
	LOG("[cache] --------finding--------")
	for (auto i = 0; i < createInfo.bindingCount; i++)
	{
		LOG("[cache]     binding %u type %d cnt %d stage %d", createInfo.pBindings[i].binding, createInfo.pBindings[i].descriptorType, createInfo.pBindings[i].descriptorCount, createInfo.pBindings[i].stageFlags)
	}
#endif
	for (auto& cacheItem : pool)
	{
		if (cacheItem->info == createInfo)
		{
#if LOG_DESCRIPTORSETLAYOUT_CACHE
			LOG("[cache] --found!--")
#endif
			return cacheItem->layout;
		}
	}

	static int setIndex = 0;
#if LOG_DESCRIPTORSETLAYOUT_CACHE
	LOG("[cache]   (not found, creating new [%d])", setIndex)
#endif
	pool.push_back(new CachedDescriptorSetLayoutInfo(createInfo));
	NAME_OBJECT(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, pool.back()->layout,
				"DescriptorSetLayout[" + std::to_string(setIndex) + "]")
	setIndex++;
	return pool.back()->layout;
}

//================ END descriptor set layout cache =================

void DescriptorSetLayout::addBinding(uint32_t bindingIndex, VkShaderStageFlags shaderStages, VkDescriptorType type)
{
	EXPECT_M(layout, VK_NULL_HANDLE, "Should only add bindings before layout is committed")
	bindings.push_back({
		.binding = bindingIndex,
		.descriptorType = type,
		.descriptorCount = 1,
		.stageFlags = shaderStages,
		.pImmutableSamplers = nullptr,
	});
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

DescriptorSet::DescriptorSet(DescriptorSetLayout &layout, uint32_t numInstances) : layout(layout)
{
	// create the pool first if it isn't created yet
	if (descriptorPool == VK_NULL_HANDLE)
	{
		// TODO: make more reliable
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 64 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 }
		};
		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = static_cast<uint32_t>(256),
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};
		EXPECT(vkCreateDescriptorPool(Vulkan::Instance->device, &poolInfo, nullptr, &descriptorPool), VK_SUCCESS)
		Vulkan::Instance->destructionQueue.emplace_back([](){
			vkDestroyDescriptorPool(Vulkan::Instance->device, descriptorPool, nullptr);
		});
	}

	std::vector<VkDescriptorSetLayout> layouts(numInstances, layout.getLayout());
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
		.pSetLayouts = layouts.data()
	};
	descriptorSets.resize(layouts.size());
	auto allocResult = vkAllocateDescriptorSets(Vulkan::Instance->device, &allocInfo, descriptorSets.data());
	EXPECT(allocResult, VK_SUCCESS)
}

void DescriptorSet::pointToBuffer(const VmaBuffer &buffer, uint32_t binding, VkDescriptorType descriptorType)
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

void DescriptorSet::pointToImageView(
	VkImageView imageView,
	uint32_t binding,
	VkDescriptorType descriptorType,
	const VkSamplerCreateInfo* samplerInfoPtr)
{
	// sampling method; pass in something else if default (here) is not desired
	VkSamplerCreateInfo samplerInfo;
	if (samplerInfoPtr) samplerInfo = *samplerInfoPtr;
	else samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0,
		.minLod = 0,
		.maxLod = VK_LOD_CLAMP_NONE,
	};
	VkSampler sampler = SamplerCache::get(samplerInfo);

	uint32_t numInstances = descriptorSets.size();
	EXPECT(numInstances != 0, true)
	for (auto i = 0; i < numInstances; i++)
	{
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

void DescriptorSet::pointToRWImageView(VkImageView imageView, uint32_t binding)
{
	uint32_t numInstances = descriptorSets.size();
	EXPECT(numInstances != 0, true)

	for (auto i = 0; i < numInstances; i++)
	{
		VkDescriptorImageInfo imageInfo = {
			.sampler = {},
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
		};
		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			// actual write data (one of three)
			.pImageInfo = &imageInfo,
			.pBufferInfo = nullptr,
			.pTexelBufferView = nullptr,
		};
		vkUpdateDescriptorSets(Vulkan::Instance->device, 1, &descriptorWrite, 0, nullptr);
	}
}

void DescriptorSet::pointToAccelerationStructure(VkAccelerationStructureKHR accelerationStructure, uint32_t binding)
{
	uint32_t numInstances = descriptorSets.size();
	EXPECT(numInstances != 0, true)

	for (auto i = 0; i < numInstances; i++)
	{
		VkWriteDescriptorSetAccelerationStructureKHR asWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
			.accelerationStructureCount = 1,
			.pAccelerationStructures = &accelerationStructure
		};
		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = &asWrite,
			.dstSet = descriptorSets[i],
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			// actual write data (one of three)
			.pImageInfo = nullptr,
			.pBufferInfo = nullptr,
			.pTexelBufferView = nullptr,
		};
		vkUpdateDescriptorSets(Vulkan::Instance->device, 1, &descriptorWrite, 0, nullptr);
	}
}

void DescriptorSet::bind(
	VkCommandBuffer cmdbuf,
	VkPipelineBindPoint pipelineBindPoint,
	uint32_t setIndex,
	VkPipelineLayout pipelineLayout,
	uint32_t instanceId,
	uint32_t numDynamicOffsets,
	const uint32_t* pDynamicOffsets)
{
	vkCmdBindDescriptorSets(
		cmdbuf, pipelineBindPoint,
		pipelineLayout,
		setIndex, // firstSet : uint32_t
		1, // descriptorSetCount : uint32_t
		&descriptorSets[instanceId],
		numDynamicOffsets,
		pDynamicOffsets);
}
