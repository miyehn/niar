#pragma once

class ShaderBindingTable
{
public:
	ShaderBindingTable() = default;
	ShaderBindingTable(VkPipeline pipeline, uint32_t numHitShaders, uint32_t numMissShaders, uint32_t numCallableShaders=0);
	~ShaderBindingTable() { shaderBindingTable.release(); }

	ShaderBindingTable(const ShaderBindingTable&) = delete;
	ShaderBindingTable& operator=(const ShaderBindingTable&) = delete;

	ShaderBindingTable(ShaderBindingTable&& other) noexcept;
	ShaderBindingTable& operator=(ShaderBindingTable&& other) noexcept;

	VkStridedDeviceAddressRegionKHR raygenRegion{};
	VkStridedDeviceAddressRegionKHR hitRegion{};
	VkStridedDeviceAddressRegionKHR missRegion{};
	VkStridedDeviceAddressRegionKHR callableRegion{};
	VmaBuffer shaderBindingTable;
};
