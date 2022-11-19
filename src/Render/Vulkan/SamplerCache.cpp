#include "SamplerCache.h"
#include "Vulkan.hpp"

bool operator==(const VkSamplerCreateInfo &info1, const VkSamplerCreateInfo &info2)
{
	return info1.magFilter == info2.magFilter &&
		   info1.minFilter == info2.minFilter &&
		   info1.addressModeU == info2.addressModeU &&
		   info1.addressModeV == info2.addressModeV &&
		   info1.addressModeW == info2.addressModeW &&

		   info1.mipmapMode == info2.mipmapMode &&
		   info1.minLod == info2.minLod &&
		   info1.maxLod == info2.maxLod &&
		   info1.mipLodBias == info2.mipLodBias;
}

namespace std
{
	template<> struct hash<VkSamplerCreateInfo>
	{
		std::size_t operator()(const VkSamplerCreateInfo &info) const noexcept
		{
			size_t h_magFilter = hash<int>{}(info.magFilter);
			size_t h_minFilter = hash<int>{}(info.minFilter);
			size_t h_addressModeU = hash<int>{}(info.addressModeU);
			size_t h_addressModeV = hash<int>{}(info.addressModeV);
			size_t h_addressModeW = hash<int>{}(info.addressModeW);

			size_t h_mipmapMode = hash<int>{}(info.mipmapMode);
			size_t h_minLod = hash<int>{}(info.minLod);
			size_t h_maxLod = hash<int>{}(info.maxLod);
			size_t h_mipLodBias = hash<int>{}(info.mipLodBias);
			return (
				(h_magFilter << 0) ^
				(h_minFilter << 3) ^
				(h_addressModeU << 6) ^
				(h_addressModeV << 9) ^
				(h_addressModeW << 12) ^
				(h_mipmapMode << 15) ^
				(h_minLod << 18) ^
				(h_maxLod << 21) ^
				(h_mipLodBias << 24)
			);
		}
	};
}

std::unordered_map<VkSamplerCreateInfo, VkSampler> SamplerCache::pool;

VkSampler SamplerCache::get(VkSamplerCreateInfo &createInfo)
{
	auto it = pool.find(createInfo);
	if (it != pool.end()) {
		return (*it).second;
	}

	VkSampler sampler;
	EXPECT(vkCreateSampler(Vulkan::Instance->device, &createInfo, nullptr, &sampler), VK_SUCCESS)
	pool[createInfo] = sampler;
	Vulkan::Instance->destructionQueue.emplace_back([sampler](){
		vkDestroySampler(Vulkan::Instance->device, sampler, nullptr);
	});
	return sampler;
}

VkSamplerCreateInfo SamplerCache::defaultInfo() {
	return {
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
}
