// Created by raind on 1/29/2022.
//

#include "RayTracingRenderer.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Texture.h"
#include "Scene/MeshObject.h"

RayTracingRenderer::RayTracingRenderer()
{
	const auto& renderExtent = Vulkan::Instance->swapChainExtent;

	ImageCreator imageCreator(
		VK_FORMAT_R8G8B8A8_UNORM,
		{renderExtent.width, renderExtent.height, 1},
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"outImage(rtx)");
	outImage = new Texture2D(imageCreator);
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, outImage->resource.image, "rtx output image")
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		vk::insertImageBarrier(cmdbuf, outImage->resource.image,
							   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
							   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
							   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
							   VK_ACCESS_SHADER_WRITE_BIT,
							   VK_ACCESS_SHADER_WRITE_BIT,
							   VK_IMAGE_LAYOUT_UNDEFINED,
							   VK_IMAGE_LAYOUT_GENERAL);
	});

	// TLAS resources (pre-allocated for up to MAX_RTX_INSTANCES)

	// "what type of geometry? what data?" - instances, data supplied from instancesBuffer
	// instancesBuffer address is set per-frame in render(); address=0 is fine for the size query below
	tlasGeometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
				.data = {.deviceAddress = 0} // updated per-frame in render()
			}
		}
	};

	// "what you want to build from the given geometry?" - tlas
	tlasBuildInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.srcAccelerationStructure = VK_NULL_HANDLE, // would be not null if we are updating instead of building from scratch (?)
		.geometryCount = 1,
		.pGeometries = &tlasGeometry,
	};

	// "get build size for the build operation, for maxCount"
	VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	uint32_t maxCount = MAX_RTX_INSTANCES;
	Vulkan::Instance->fn_vkGetAccelerationStructureBuildSizesKHR(
		Vulkan::Instance->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&tlasBuildInfo,
		&maxCount,
		&tlasBuildSizeInfo);

	tlasBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		tlasBuildSizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		"RTX TLAS buffer"});

	const VkAccelerationStructureCreateInfoKHR tlasCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlasBuffer.getBufferInstance(),
		.size = tlasBuildSizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &tlasCreateInfo, nullptr, &tlas);

	scratchBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		tlasBuildSizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		"RTX TLAS scratch buffer"});

	const VkBufferDeviceAddressInfo scratchAddrInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.getBufferInstance()
	};
	tlasBuildInfo.dstAccelerationStructure = tlas;
	tlasBuildInfo.scratchData.deviceAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &scratchAddrInfo);

	// descriptor set layout: binding 0 = viewInfoUbo, binding 1 = TLAS, binding 2 = outImage
	DescriptorSetLayout layout{};
	layout.addBinding(0, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layout.addBinding(1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	layout.addBinding(2, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		auto& fd = gpuFrameData[i];
		fd.viewInfoUbo = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			sizeof(ViewInfo),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"RTX view info UBO"});
		fd.instancesBuffer = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			MAX_RTX_INSTANCES * sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"RTX instances buffer"});
		const VkBufferDeviceAddressInfo instancesAddrInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = fd.instancesBuffer.getBufferInstance()
		};
		fd.instancesBufferAddr = vkGetBufferDeviceAddress(Vulkan::Instance->device, &instancesAddrInfo);
		fd.descriptorSet = DescriptorSet(layout);
		fd.descriptorSet.pointToBuffer(fd.viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.descriptorSet.pointToAccelerationStructure(tlas, 1);
		fd.descriptorSet.pointToRWImageView(outImage->imageView, 2);
	}

	// pipeline
	RayTracingPipelineBuilder builder{};
	builder.rgenPath = "spirv/ray_gen.rgen.spv";
	builder.rchitPaths.emplace_back("spirv/ray_chit.rchit.spv");
	builder.rchitPaths.emplace_back("spirv/ray_chit2.rchit.spv");
	builder.rmissPaths.emplace_back("spirv/ray_miss.rmiss.spv");
	builder.rmissPaths.emplace_back("spirv/ray_miss2.rmiss.spv");
	builder.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{0, -1});
	builder.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{1, -1});
	builder.useDescriptorSetLayout(0, gpuFrameData[0].descriptorSet.getLayout());
	builder.build(pipeline, pipelineLayout);

	// sbt
	sbt = ShaderBindingTable(pipeline, 2, 2);
}

RayTracingRenderer::~RayTracingRenderer()
{
	if (tlas != VK_NULL_HANDLE)
		Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, tlas, nullptr);
	tlasBuffer.release();
	scratchBuffer.release();
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		gpuFrameData[i].viewInfoUbo.release();
		gpuFrameData[i].instancesBuffer.release();
	}
	delete outImage;
}

void RayTracingRenderer::render(VkCommandBuffer cmdbuf)
{
	if (!camera) return;

	auto& fd = gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()];

	// collect all MeshObjects with a valid BLAS in the active scene
	std::vector<MeshObject*> meshObjects;
	drawable->foreach_descendent_bfs([&meshObjects](SceneObject* obj) {
		if (auto* mo = dynamic_cast<MeshObject*>(obj))
			if (mo->enabled())
				meshObjects.push_back(mo);
	});
	if (meshObjects.empty()) return;

	if (meshObjects.size() > MAX_RTX_INSTANCES) {
		WARN("Scene has more MeshObjects with BLAS (%zu) than MAX_RTX_INSTANCES (%u); clamping.",
			meshObjects.size(), MAX_RTX_INSTANCES)
		meshObjects.resize(MAX_RTX_INSTANCES);
	}

	ViewInfo viewInfo = getCameraViewInfo();
	gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()].viewInfoUbo.writeData(&viewInfo, sizeof(viewInfo));

	// build instance descriptors from current world transforms
	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(meshObjects.size());
	for (uint32_t i = 0; i < meshObjects.size(); i++)
	{
		const MeshObject* mo = meshObjects[i];

		// convert GLM column-major mat4 to Vulkan row-major [3][4] transform
		glm::mat4 t = mo->object_to_world();
		VkTransformMatrixKHR transform{};
		for (int row = 0; row < 3; row++)
			for (int col = 0; col < 4; col++)
				transform.matrix[row][col] = t[col][row];

		instances.push_back({
			.transform = transform,
			.instanceCustomIndex = i,
			.mask = 0xFF,
			.instanceShaderBindingTableRecordOffset = 0, // TRYME: change this to use other hit/miss shaders in the SBT
			.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
			.accelerationStructureReference = mo->mesh.gpu_data.blasAddress,
		});
	}

	fd.instancesBuffer.writeData(instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

	{// rebuild TLAS in the command buffer
		SCOPED_DRAW_EVENT(cmdbuf, "rebuild TLAS")

		tlasGeometry.geometry.instances.data.deviceAddress = fd.instancesBufferAddr;

		VkMemoryBarrier barrierPre = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
		};
		vkCmdPipelineBarrier(cmdbuf,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0, 1, &barrierPre, 0, nullptr, 0, nullptr);

		VkAccelerationStructureBuildRangeInfoKHR range = {
			.primitiveCount = static_cast<uint32_t>(instances.size()),
			.primitiveOffset = 0,
			.firstVertex = 0,
			.transformOffset = 0,
		};
		auto rangePtr = &range;
		// according to claude, this is a command recording call and completely copies everything needed from tlasBuildInfo
		// so cpu writes after its return is always safe
		Vulkan::Instance->fn_vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &tlasBuildInfo, &rangePtr);

		VkMemoryBarrier barrierPost = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
		};
		vkCmdPipelineBarrier(cmdbuf,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			0, 1, &barrierPost, 0, nullptr, 0, nullptr);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "trace rays")
		vk::insertImageBarrier(cmdbuf, outImage->resource.image,
							   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
							   VK_PIPELINE_STAGE_TRANSFER_BIT,
							   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
							   VK_ACCESS_TRANSFER_READ_BIT,
							   VK_ACCESS_SHADER_WRITE_BIT,
							   VK_IMAGE_LAYOUT_UNDEFINED,
							   VK_IMAGE_LAYOUT_GENERAL);

		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
		fd.descriptorSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 0, pipelineLayout);
		const auto &extent = Vulkan::Instance->swapChainExtent;
		Vulkan::Instance->fn_vkCmdTraceRaysKHR(
			cmdbuf,
			&sbt.raygenRegion,
			&sbt.missRegion,
			&sbt.hitRegion,
			&sbt.callableRegion,
			extent.width,
			extent.height,
			/*depth*/1);

		vk::insertImageBarrier(cmdbuf, outImage->resource.image,
							   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
							   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
							   VK_PIPELINE_STAGE_TRANSFER_BIT,
							   VK_ACCESS_SHADER_WRITE_BIT,
							   VK_ACCESS_TRANSFER_READ_BIT,
							   VK_IMAGE_LAYOUT_GENERAL,
							   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	}

	const auto& renderExtent = Vulkan::Instance->swapChainExtent;
	vk::blitToScreen(
		cmdbuf,
		outImage->resource.image,
		{0, 0, 0},
		{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});
}

RayTracingRenderer *RayTracingRenderer::get()
{
	static RayTracingRenderer* renderer = nullptr;
	if (renderer == nullptr)
	{
		renderer = new RayTracingRenderer();
	}
	return renderer;
}
