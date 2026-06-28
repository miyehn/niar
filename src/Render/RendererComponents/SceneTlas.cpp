#include "SceneTlas.h"
#include "Scene/MeshObject.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Utils/myn/Log.h"
#include <algorithm>

void SceneTlas::init(const std::string& inDebugNamePrefix)
{
	debugNamePrefix = inDebugNamePrefix;

	VkAccelerationStructureGeometryKHR tlasGeometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
				.data = {.deviceAddress = 0}
			}
		}
	};
	VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.srcAccelerationStructure = VK_NULL_HANDLE,
		.geometryCount = 1,
		.pGeometries = &tlasGeometry,
	};

	VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	Vulkan::Instance->fn_vkGetAccelerationStructureBuildSizesKHR(
		Vulkan::Instance->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&tlasBuildInfo,
		&MAX_RTX_INSTANCES,
		&tlasBuildSizeInfo);

	tlasBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		tlasBuildSizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		debugNamePrefix + " TLAS buffer"});

	const VkAccelerationStructureCreateInfoKHR tlasCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlasBuffer.buffer,
		.size = tlasBuildSizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &tlasCreateInfo, nullptr, &tlas);
	NAME_OBJECT(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, tlas, (debugNamePrefix + " TLAS").c_str())

	scratchBuffer = VmaBuffer({
		.allocator = &Vulkan::Instance->memoryAllocator,
		.strideSize = tlasBuildSizeInfo.buildScratchSize,
		.bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
		.debugName = debugNamePrefix + " TLAS scratch buffer",
		.minAllocationAlignment = Vulkan::Instance->minAccelerationStructureScratchOffsetAlignment,
	});

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		instancesBuffers[i] = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			MAX_RTX_INSTANCES * sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			debugNamePrefix + " TLAS instances buffer (" + std::to_string(i) + ")"});
	}
}

void SceneTlas::release()
{
	ASSERT(tlas != VK_NULL_HANDLE)
	Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, tlas, nullptr);
	tlas = VK_NULL_HANDLE;

	tlasBuffer.release();
	scratchBuffer.release();
	for (auto& instancesBuffer : instancesBuffers) {
		instancesBuffer.release();
	}

	debugNamePrefix.clear();
}

VkAccelerationStructureKHR SceneTlas::get() const
{
	return tlas;
}

void SceneTlas::build_from_meshes(
	VkCommandBuffer cmdbuf,
	uint32_t frameIndex,
	const std::vector<MeshObject*>& meshObjects,
	VkPipelineStageFlags dstStageMask)
{
	ASSERT(frameIndex < MAX_FRAMES_IN_FLIGHT)
	ASSERT(tlas != VK_NULL_HANDLE)

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(std::min(meshObjects.size(), static_cast<size_t>(MAX_RTX_INSTANCES)));

	for (const MeshObject* mo : meshObjects)
	{
		if (mo == nullptr || mo->mesh.gpu_data.blasAddress == 0) continue;
		if (instances.size() >= MAX_RTX_INSTANCES) break;

		glm::mat4 t = mo->object_to_world();
		VkTransformMatrixKHR transform{};
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 4; col++) {
				transform.matrix[row][col] = t[col][row];
			}
		}

		instances.push_back({
			.transform = transform,
			// can use .instanceCustomIndex to know in shader what index it's hitting
			.instanceCustomIndex = static_cast<uint32_t>(instances.size()),
			.mask = 0xFF,
			.instanceShaderBindingTableRecordOffset = 0,
			.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
			.accelerationStructureReference = mo->mesh.gpu_data.blasAddress,
		});
	}

	if (!instances.empty()) {
		instancesBuffers[frameIndex].writeData(
			instances.data(),
			instances.size() * sizeof(VkAccelerationStructureInstanceKHR));
	}

	vk::buildTlas(
		cmdbuf,
		instancesBuffers[frameIndex],
		static_cast<uint32_t>(instances.size()),
		scratchBuffer,
		dstStageMask,
		tlas);
}
