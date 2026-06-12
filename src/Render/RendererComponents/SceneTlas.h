#pragma once

#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/Buffer.h"
#include <string>
#include <vector>

class MeshObject;

constexpr uint32_t MAX_RTX_INSTANCES = 64;

struct SceneTlas
{
	SceneTlas() = default;
	SceneTlas(const SceneTlas&) = delete;
	SceneTlas& operator=(const SceneTlas&) = delete;

	void init(const std::string& debugNamePrefix);
	void release();

	VkAccelerationStructureKHR get() const;

	void build_from_meshes(
		VkCommandBuffer cmdbuf,
		uint32_t frameIndex,
		const std::vector<MeshObject*>& meshObjects,
		VkPipelineStageFlags dstStageMask);

private:
	std::string debugNamePrefix;
	VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
	VmaBuffer tlasBuffer;
	VmaBuffer scratchBuffer;
	VmaBuffer instancesBuffers[MAX_FRAMES_IN_FLIGHT];
};
