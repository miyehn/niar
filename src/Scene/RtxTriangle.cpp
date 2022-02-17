//
// Created by raind on 1/29/2022.
//

#include "RtxTriangle.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Mesh.h"
#include "Render/Texture.h"
#include "Render/Vulkan/ShaderBindingTable.h"
#include "Utils/myn/Misc.h"

void RtxTriangle::create_vertex_buffer()
{
	const float vertices[9] = {
		0.25f, 0.25f, 0.0f,
		0.75f, 0.25f, 0.0f,
		0.5f, 0.75f, 0.0f,
	};

	VkDeviceSize bufferSize = sizeof(float) * 9;

	// create a staging buffer
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// copy vertex buffer memory over to staging buffer
	stagingBuffer.writeData((void *) vertices);

	// now create the actual vertex buffer
	auto vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	vertexBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// and copy stuff from staging buffer to vertex buffer
	vk::copyBuffer(vertexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void RtxTriangle::create_index_buffer()
{
	const VERTEX_INDEX_TYPE indices[3] = { 0, 1, 2 };

	VkDeviceSize bufferSize = sizeof(VERTEX_INDEX_TYPE) * 3;

	// create a staging buffer
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// copy data to staging buffer
	stagingBuffer.writeData((void*)indices);

	// create the actual index buffer
	auto vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	indexBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		bufferSize,
		vkUsage,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// move stuff from staging buffer and destroy staging buffer
	vk::copyBuffer(indexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void buildBlas(
	VkAccelerationStructureGeometryKHR geom,
	VkAccelerationStructureBuildRangeInfoKHR range,
	VkBuildAccelerationStructureFlagsKHR flags,
	VkAccelerationStructureKHR* outBlas,
	VmaBuffer* outBlasBuffer)
{
	VkDeviceSize structureSize{0};
	VkDeviceSize scratchSize{0};
	const uint32_t maxPrimitivesCount = 1;

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = flags,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1,
		.pGeometries = &geom,
	};

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

	Vulkan::Instance->fn_vkGetAccelerationStructureBuildSizesKHR(
		Vulkan::Instance->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &maxPrimitivesCount, &sizeInfo);

	VmaBuffer scratchBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
	const VkBufferDeviceAddressInfo scratchBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.getBufferInstance()
	};
	VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &scratchBufferAddressInfo);

	// for compaction
	VkQueryPool queryPool{VK_NULL_HANDLE};
	VkQueryPoolCreateInfo qpCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
		.queryCount = 1
	};
	vkCreateQueryPool(Vulkan::Instance->device, &qpCreateInfo, nullptr, &queryPool);
	vkResetQueryPool(Vulkan::Instance->device, queryPool, 0, 1);

	//======== actual alloc of buffer and accel structure ========

	// pt 1 : alloc AS buffer and create it
	VmaBuffer stagingBlasBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY);
	VkAccelerationStructureCreateInfoKHR asCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = stagingBlasBuffer.getBufferInstance(),
		.size = sizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	VkAccelerationStructureKHR stagingBlas;
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &asCreateInfo, nullptr, &stagingBlas);

	// pt 2 : build (using scratch memory)
	buildInfo.dstAccelerationStructure = stagingBlas;
	buildInfo.scratchData.deviceAddress = scratchAddress;
	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		auto rangePtr = &range;
		Vulkan::Instance->fn_vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &buildInfo, &rangePtr);

		VkMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
		};
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0,
			1,
			&barrier,
			0,
			nullptr,
			0,
			nullptr
			);
		Vulkan::Instance->fn_vkCmdWriteAccelerationStructuresPropertiesKHR(
			cmdbuf,
			1, // accl structure count
			&stagingBlas,
			VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
			queryPool,
			0);
	});
	Vulkan::Instance->waitDeviceIdle();

	// pt3 : compaction (writes to actual output)
	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		VkDeviceSize compactSize = 0; // need to zero out because vkGetQueryPoolResults doesn't actually overwrite all bits..
		EXPECT(vkGetQueryPoolResults(
			Vulkan::Instance->device,
			queryPool,
			0,
			1,
			sizeof(VkDeviceSize),
			&compactSize,
			sizeof(VkDeviceSize),
			VK_QUERY_RESULT_WAIT_BIT), VK_SUCCESS);

		*outBlasBuffer = VmaBuffer(
			&Vulkan::Instance->memoryAllocator,
			compactSize,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			VMA_MEMORY_USAGE_GPU_ONLY);

		VkAccelerationStructureCreateInfoKHR compactInfo = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
			.buffer = outBlasBuffer->getBufferInstance(),
			.size = compactSize,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		};
		Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &compactInfo, nullptr, outBlas);

		VkCopyAccelerationStructureInfoKHR copyInfo = {
			.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR,
			.src = stagingBlas,
			.dst = *outBlas,
			.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR
		};
		Vulkan::Instance->fn_vkCmdCopyAccelerationStructureKHR(cmdbuf, &copyInfo);
	});
	Vulkan::Instance->waitDeviceIdle();

	vkDestroyQueryPool(Vulkan::Instance->device, queryPool, nullptr);
	scratchBuffer.release();
	stagingBlasBuffer.release();
	Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, stagingBlas, nullptr);
}

void buildTlas(
	VkAccelerationStructureInstanceKHR inst,
	VkBuildAccelerationStructureFlagsKHR flags,
	VkAccelerationStructureKHR* outTlas,
	VmaBuffer* outTlasBuffer)
{
	VmaBuffer instancesBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
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
	*outTlasBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY);

	// create TLAS
	VkAccelerationStructureCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = outTlasBuffer->getBufferInstance(),
		.size = sizeInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	Vulkan::Instance->fn_vkCreateAccelerationStructureKHR(Vulkan::Instance->device, &createInfo, nullptr, outTlas);

	// scratch buffer
	VmaBuffer scratchBuffer = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
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

		// TODO
		auto rangePtr = &range;
		Vulkan::Instance->fn_vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &buildInfo, &rangePtr);
	});
	Vulkan::Instance->waitDeviceIdle();

	scratchBuffer.release();
	instancesBuffer.release();
}

RtxTriangle::RtxTriangle()
{
	name = "RTX Triangle";
	create_vertex_buffer();
	create_index_buffer();

	auto vBufferRaw = vertexBuffer.getBufferInstance();
	auto iBufferRaw = indexBuffer.getBufferInstance();

	const VkBufferDeviceAddressInfo vBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vBufferRaw
	};
	const VkBufferDeviceAddressInfo iBufferAddressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = iBufferRaw
	};
	VkDeviceAddress vBufferAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &vBufferAddressInfo);
	VkDeviceAddress iBufferAddress = vkGetBufferDeviceAddress(Vulkan::Instance->device, &iBufferAddressInfo);

	// BLAS

	VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		.vertexData = {
			.deviceAddress = vBufferAddress
		},
		.vertexStride = sizeof(float) * 3,
		.maxVertex = 3,
		.indexType = VK_INDEX_TYPE,
		.indexData = {
			.deviceAddress = iBufferAddress
		},
	};

	VkAccelerationStructureGeometryKHR asGeom = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry = {
			.triangles = triangles
		},
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildRangeInfoKHR offset = {
		.primitiveCount = 1,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0
	};

	buildBlas(asGeom, offset,
			  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR |
			  VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
			  &blas, &blasBuffer);

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
		.instanceShaderBindingTableRecordOffset = 0, // same hit group
		.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		.accelerationStructureReference = blasAddress,
	};
	buildTlas(rayInst, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, &tlas, &tlasBuffer);

	// output image
	auto renderExtent = Vulkan::Instance->swapChainExtent;
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

	// descriptor set
	DescriptorSetLayout setLayout{};
	setLayout.addBinding(0, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	setLayout.addBinding(1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	descriptorSet = DescriptorSet(setLayout);// commit bindings
	descriptorSet.pointToAccelerationStructure(tlas, 0);
	descriptorSet.pointToRWImageView(outImage->imageView, 1);

	// pipeline
	RayTracingPipelineBuilder builder{};
	builder.rgenPath = "spirv/ray_gen.rgen.spv"; // rgen
	builder.rchitPaths.emplace_back("spirv/ray_chit.rchit.spv");
	builder.rchitPaths.emplace_back("spirv/ray_chit2.rchit.spv");
	builder.rmissPaths.emplace_back("spirv/ray_miss.rmiss.spv"); // rmiss
	builder.rmissPaths.emplace_back("spirv/ray_miss2.rmiss.spv"); // rmiss
	builder.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{0, -1}); // rhit
	builder.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{1, -1}); // rhit
	builder.useDescriptorSetLayout(0, descriptorSet.getLayout());
	builder.build(pipeline, pipelineLayout);

	// sbt (TODO: move into pipeline builder?)
	sbt = new ShaderBindingTable(pipeline, 2, 2);
}

RtxTriangle::~RtxTriangle()
{
	Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, blas, nullptr);
	Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, tlas, nullptr);
	blasBuffer.release();
	tlasBuffer.release();
	vertexBuffer.release();
	indexBuffer.release();
	delete outImage;
	delete sbt;
}

void RtxTriangle::update(float elapsed)
{
	SceneObject::update(elapsed);

	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		SCOPED_DRAW_EVENT(cmdbuf, "trace rays")

		auto extent = Vulkan::Instance->swapChainExtent;

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
			&sbt->raygenRegion,
			&sbt->missRegion,
			&sbt->hitRegion,
			&sbt->callableRegion,
			extent.width,
			extent.height,
			/*depth*/1);
	});
}