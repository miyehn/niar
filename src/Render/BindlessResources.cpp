#include "BindlessResources.h"

#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Utils/myn/Log.h"

#include <algorithm>
#include <array>

BindlessResources* BindlessResources::Instance = nullptr;

namespace
{

constexpr uint32_t BindlessTextureBinding = 0;
constexpr uint32_t MaterialBufferBinding = 1;

VkDescriptorImageInfo textureDescriptor(
	VkImageView imageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	return {
		.sampler = SamplerCache::get(samplerInfo),
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
}

}

void BindlessResources::init()
{
	ASSERT(Instance == nullptr)
	ASSERT(Vulkan::Instance != nullptr)

	Instance = this;

	{ // create the dedicated pool
		const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = MAX_BINDLESS_TEXTURES_2D,
			},
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
			},
		}};
		const VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data(),
		};
		EXPECT(vkCreateDescriptorPool(
			Vulkan::Instance->device,
			&poolInfo,
			nullptr,
			&descriptorPool), VK_SUCCESS)
	}

	{ // allocate the bindless set from the dedicated pool
		constexpr VkShaderStageFlags bindlessStages =
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		bindlessSetLayout.addBinding(
			BindlessTextureBinding,
			bindlessStages,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			MAX_BINDLESS_TEXTURES_2D);
		bindlessSetLayout.addBinding(
			MaterialBufferBinding,
			bindlessStages,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		bindlessDescriptorSet = DescriptorSet(bindlessSetLayout, descriptorPool);
	}

	{ // create placeholder material buffer and put to slot 1
		placeholderMaterialBuffer = VmaBuffer({
			.allocator = &Vulkan::Instance->memoryAllocator,
			.strideSize = 16,
			.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			.debugName = "Bindless placeholder material buffer",
		});
		std::array<uint8_t, 16> zeroPlaceholder{};
		placeholderMaterialBuffer.writeData(
			zeroPlaceholder.data(),
			zeroPlaceholder.size());
		bindlessDescriptorSet.pointToBuffer(
			placeholderMaterialBuffer,
			MaterialBufferBinding,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	ASSERT(freeTexture2DSlots.empty())
	freeTexture2DSlots.reserve(MAX_BINDLESS_TEXTURES_2D);
	for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES_2D; ++i)
	{
		texture2DSlots[i] = {};
		freeTexture2DSlots.push_back(MAX_BINDLESS_TEXTURES_2D - 1 - i);
	}
}

void BindlessResources::release()
{
	ASSERT(Instance == this)

	const uint32_t occupiedCount = static_cast<uint32_t>(std::count_if(
		texture2DSlots.begin(),
		texture2DSlots.end(),
		[](const Texture2DSlot& slot) { return slot.occupied; }));
	ASSERT_M(
		occupiedCount == 0,
		"BindlessResources still has %u texture registration(s)",
		occupiedCount)

	vkDestroyDescriptorPool(Vulkan::Instance->device, descriptorPool, nullptr);
	descriptorPool = VK_NULL_HANDLE;
	bindlessDescriptorSet = {};
	bindlessSetLayout = {};
	fillerTexture2DDescriptor = {};

	placeholderMaterialBuffer.release();

	for (auto& slot : texture2DSlots) slot = {};
	freeTexture2DSlots.clear();
	Instance = nullptr;
}

BindlessTexture2DHandle BindlessResources::addTexture2D(
	VkImageView imageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	ASSERT(Instance == this)
	ASSERT(imageView != VK_NULL_HANDLE)

	if (freeTexture2DSlots.empty())
	{
		ERR("Bindless 2D texture table is exhausted")
		return {};
	}

	// take a slot from free list
	const uint32_t slotIndex = freeTexture2DSlots.back();
	freeTexture2DSlots.pop_back();
	// (this is the slot it's just taken from free list must be free)
	auto& slot = texture2DSlots[slotIndex];
	ASSERT(!slot.occupied)

	// create a descriptor for input image and update bindless
	const VkDescriptorImageInfo descriptor =
		textureDescriptor(imageView, samplerInfo);
	bindlessDescriptorSet.pointToImageViews(
		BindlessTextureBinding,
		slotIndex,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		{&descriptor, 1});

	slot.occupied = true;
	return {
		.index = slotIndex,
		.generation = slot.generation,
	};
}

void BindlessResources::removeTexture2D(BindlessTexture2DHandle handle)
{
	const uint32_t slotIndex = validate(handle);
	ASSERT(slotIndex != INVALID_BINDLESS_INDEX)

	Vulkan::Instance->waitDeviceIdle();
	bindlessDescriptorSet.pointToImageViews(
		BindlessTextureBinding,
		slotIndex,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		{&fillerTexture2DDescriptor, 1});

	auto& slot = texture2DSlots[slotIndex];
	slot.occupied = false;
	++slot.generation;
	freeTexture2DSlots.push_back(slotIndex);
}

void BindlessResources::setTexture2DFiller(
	VkImageView imageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	ASSERT(Instance == this)
	ASSERT(imageView != VK_NULL_HANDLE)
	if (Instance != this || imageView == VK_NULL_HANDLE) return;

	fillerTexture2DDescriptor = textureDescriptor(imageView, samplerInfo);
	for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES_2D; ++i)
	{
		if (texture2DSlots[i].occupied) continue;
		bindlessDescriptorSet.pointToImageViews(
			BindlessTextureBinding,
			i,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			{&fillerTexture2DDescriptor, 1});
	}
}

uint32_t BindlessResources::validate(BindlessTexture2DHandle handle) const
{
	if (handle.index == INVALID_BINDLESS_INDEX)
	{
		ERR("Invalid bindless 2D texture handle")
		return INVALID_BINDLESS_INDEX;
	}
	if (handle.index >= MAX_BINDLESS_TEXTURES_2D)
	{
		ERR("Bindless 2D texture handle index %u is out of range", handle.index)
		return INVALID_BINDLESS_INDEX;
	}

	const auto& slot = texture2DSlots[handle.index];
	if (!slot.occupied)
	{
		ERR("Bindless 2D texture handle %u:%u refers to a free slot", handle.index, handle.generation)
		return INVALID_BINDLESS_INDEX;
	}
	if (slot.generation != handle.generation)
	{
		ERR(
			"Stale bindless 2D texture handle %u:%u; current generation is %u",
			handle.index,
			handle.generation,
			slot.generation)
		return INVALID_BINDLESS_INDEX;
	}
	return handle.index;
}
