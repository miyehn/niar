#include "SamplerCache.h"
#include "Vulkan.hpp"

bool operator==(const VkSamplerCreateInfo &info1, const VkSamplerCreateInfo &info2)
{
	return info1.magFilter == info2.magFilter &&
		   info1.minFilter == info2.minFilter &&
		   info1.addressModeU == info2.addressModeU &&
		   info1.addressModeV == info2.addressModeV &&
		   info1.addressModeW == info2.addressModeW;
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
			return ((h_magFilter << 0) ^
					(h_minFilter << 3) ^
					(h_addressModeU << 6) ^
					(h_addressModeV << 9) ^
					(h_addressModeW << 12)
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
