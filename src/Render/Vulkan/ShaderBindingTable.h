#pragma once
#include "Vulkan.hpp"

class ShaderBindingTable
{
public:
	ShaderBindingTable() = default;
	ShaderBindingTable(VkPipeline pipeline, uint32_t numHitShaders, uint32_t numMissShaders, uint32_t numCallableShaders=0);
	~ShaderBindingTable() { shaderBindingTable.release(); }
	VkStridedDeviceAddressRegionKHR raygenRegion{};
	VkStridedDeviceAddressRegionKHR hitRegion{};
	VkStridedDeviceAddressRegionKHR missRegion{};
	VkStridedDeviceAddressRegionKHR callableRegion{};
	VmaBuffer shaderBindingTable;
};
