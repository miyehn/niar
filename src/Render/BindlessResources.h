#pragma once

#include "Render/Vulkan/DescriptorSet.h"

#include <array>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

constexpr uint32_t MAX_BINDLESS_TEXTURES_2D = 1024;
constexpr uint32_t INVALID_BINDLESS_INDEX = UINT32_MAX;

struct BindlessTexture2DHandle
{
	uint32_t index = INVALID_BINDLESS_INDEX;
	uint32_t generation = 0;
};

struct BindlessTexture2DInfo
{
	bool registerTexture = false;
	VkSamplerCreateInfo samplerInfo;
};

class BindlessResources
{
public:
	static BindlessResources* Instance;

	BindlessResources() = default;
	BindlessResources(const BindlessResources&) = delete;
	BindlessResources& operator=(const BindlessResources&) = delete;

	void init();
	void release();

	BindlessTexture2DHandle addTexture2D(
		VkImageView imageView,
		const VkSamplerCreateInfo& samplerInfo);
	void removeTexture2D(BindlessTexture2DHandle handle);

	// Returns the shader-visible index. Invalid or stale handles are errors.
	uint32_t validate(BindlessTexture2DHandle handle) const;

	const DescriptorSet& descriptorSet() const { return bindlessDescriptorSet; }
	const DescriptorSetLayout& layout() const { return bindlessSetLayout; }

private:
	struct Texture2DSlot
	{
		uint32_t generation = 0;
		bool occupied = false;
	};

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	DescriptorSetLayout bindlessSetLayout;
	DescriptorSet bindlessDescriptorSet;

	std::array<Texture2DSlot, MAX_BINDLESS_TEXTURES_2D> texture2DSlots{};
	std::vector<uint32_t> freeTexture2DSlots;
};
