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

	// todo [myn]: might want to make just one get function that returns framedata
	VkAccelerationStructureKHR get() const;
	const VmaBuffer& getSceneInstanceRecordBuffer(uint32_t frameIndex) const;
	uint32_t getSceneInstanceRecordCount(uint32_t frameIndex) const;

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

	struct FrameData {
		VmaBuffer tlasInstancesBuffer;
		VmaBuffer sceneInstanceRecordBuffer;
		uint32_t sceneInstanceRecordCount = 0;
	};
	FrameData frameData[MAX_FRAMES_IN_FLIGHT];
};
