//
// Created by raind on 1/29/2022.
//

#include "RayTracingRenderer.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Texture.h"
#include "Render/Vulkan/RenderPassBuilder.h"

static void buildTlas(
	VkAccelerationStructureInstanceKHR inst,
	VkBuildAccelerationStructureFlagsKHR flags,
	VkAccelerationStructureKHR* outTlas,
	VmaBuffer* outTlasBuffer)
{
	VmaBuffer instancesBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU});
	instancesBuffer.writeData(&inst, sizeof(VkAccelerationStructureInstanceKHR));
	const VkBufferDeviceAddressInfo instancesBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = instancesBuffer.getBufferInstance()
	};
	VkDeviceAddress instancesBufferAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &instancesBufferAddressInfo);

	VkAccelerationStructureGeometryInstancesDataKHR instancesVk = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
		.data = {
			.deviceAddress = instancesBufferAddress
		},
	};
	VkAccelerationStructureGeometryKHR tlasGeometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = instancesVk
		}
	};
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = flags,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.srcAccelerationStructure = VK_NULL_HANDLE,
		.geometryCount = 1,
		.pGeometries = &tlasGeometry,
	};
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	};
	uint32_t numInstances = 1;
	Vulkan::Instance->fn_vkGetAccelerationStructureBuildSizesKHR(Vulkan::Instance->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numInstances, &sizeInfo);

	// buffer for TLAS
	*outTlasBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		"RtxTriangle tlas buffer"});

	// create TLAS
	VkAccelerationStructureCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = outTlasBuffer->getBufferInstance(),
		.size = sizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &createInfo, nullptr, outTlas);

	// scratch buffer
	VmaBuffer scratchBuffer = VmaBuffer({
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY});
	const VkBufferDeviceAddressInfo scratchBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.getBufferInstance()
	};
	VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &scratchBufferAddressInfo);

	buildInfo.dstAccelerationStructure = *outTlas;
	buildInfo.scratchData.deviceAddress = scratchAddress;
	VkAccelerationStructureBuildRangeInfoKHR range = {
		.primitiveCount = 1,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};

	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		// make sure instance buffer is copied before AS gets built
		VkMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
		};
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0,
			1,
			&barrier,
			0,
			nullptr,
			0,
			nullptr);

		auto rangePtr = &range;
		Vulkan::Instance->fn_vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &buildInfo, &rangePtr);
	});
	Vulkan::Instance->waitDeviceIdle();

	scratchBuffer.release();
	instancesBuffer.release();
}

RayTracingRenderer::RayTracingRenderer()
{
	renderExtent = Vulkan::Instance->swapChainExtent;

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
}

RayTracingRenderer::~RayTracingRenderer()
{
	if (tlas != VK_NULL_HANDLE)
		Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, tlas, nullptr);
	tlasBuffer.release();
	delete outImage;
}

void RayTracingRenderer::setup(VkAccelerationStructureKHR blas)
{
	// TLAS
	VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = blas
	};
	VkDeviceAddress blasAddress = Vulkan::Instance->fn_vkGetAccelerationStructureDeviceAddressKHR(Vulkan::Instance->device, &blasAddressInfo);

	VkAccelerationStructureInstanceKHR rayInst = {
		.transform = {
			.matrix = {
				{1, 0, 0, 0},
				{0, 1, 0, 0},
				{0, 0, 1, 0}
			}
		},
		.instanceCustomIndex = 0,
		.mask = 0xFF,
		.instanceShaderBindingTableRecordOffset = 0,
		.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		.accelerationStructureReference = blasAddress,
	};
	buildTlas(rayInst, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, &tlas, &tlasBuffer);

	// descriptor set
	DescriptorSetLayout setLayout{};
	setLayout.addBinding(0, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	setLayout.addBinding(1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	descriptorSet = DescriptorSet(setLayout);
	descriptorSet.pointToAccelerationStructure(tlas, 0);
	descriptorSet.pointToRWImageView(outImage->imageView, 1);

	// pipeline
	RayTracingPipelineBuilder builder{};
	builder.rgenPath = "spirv/ray_gen.rgen.spv";
	builder.rchitPaths.emplace_back("spirv/ray_chit.rchit.spv");
	builder.rchitPaths.emplace_back("spirv/ray_chit2.rchit.spv");
	builder.rmissPaths.emplace_back("spirv/ray_miss.rmiss.spv");
	builder.rmissPaths.emplace_back("spirv/ray_miss2.rmiss.spv");
	builder.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{0, -1});
	builder.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{1, -1});
	builder.useDescriptorSetLayout(0, descriptorSet.getLayout());
	builder.build(pipeline, pipelineLayout);

	// sbt
	sbt = ShaderBindingTable(pipeline, 2, 2);
}

void RayTracingRenderer::render(VkCommandBuffer cmdbuf)
{
	if (outImage == nullptr || pipeline == VK_NULL_HANDLE) return;

	auto extent = Vulkan::Instance->swapChainExtent;

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
	descriptorSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 0, pipelineLayout);
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
