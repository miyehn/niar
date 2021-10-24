#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>

class SamplerCache
{
protected:
	static std::unordered_map<VkSamplerCreateInfo, VkSampler> pool;

public:
	static VkSampler get(VkSamplerCreateInfo& createInfo);
	static void cleanup();
};
