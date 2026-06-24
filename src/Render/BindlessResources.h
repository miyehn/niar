#pragma once

#include <cstdint>
#include <optional>
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
	// nullopt uses SamplerCache::defaultInfo() when registration is implemented.
	std::optional<VkSamplerCreateInfo> samplerInfo;
};
