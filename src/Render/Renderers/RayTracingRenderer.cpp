// Created by raind on 1/29/2022.
//

#include "RayTracingRenderer.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/Pipeline.h"
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

	sceneTlas.init("RTX");

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
		fd.descriptorSet = DescriptorSet(layout);
		fd.descriptorSet.pointToBuffer(fd.viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.descriptorSet.pointToAccelerationStructure(sceneTlas.get(), 1);
		fd.descriptorSet.pointToRWImageView(outImage->imageView, 2);
	}

	// pipeline
	auto& b = rtPipeline.builder;
	b.rgenPath = "shaders/ray_gen.rgen";
	b.rchitDefs.emplace_back("shaders/ray_chit.rchit");
	b.rchitDefs.emplace_back("shaders/ray_chit2.rchit");
	b.rmissDefs.emplace_back("shaders/ray_miss.rmiss");
	b.rmissDefs.emplace_back("shaders/ray_miss2.rmiss");
	b.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{0, -1});
	b.hitGroups.emplace_back(RayTracingPipelineBuilder::HitGroup{1, -1});
	b.useDescriptorSetLayout(0, gpuFrameData[0].descriptorSet.getLayout());
	rtPipeline.onRebuilt = [this]() { sbt = ShaderBindingTable(rtPipeline.pipeline, 2, 2); };
	rtPipeline.build("Ray Tracing");

	// sbt
	sbt = ShaderBindingTable(rtPipeline.pipeline, 2, 2);
}

RayTracingRenderer::~RayTracingRenderer()
{
	sceneTlas.release();
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		gpuFrameData[i].viewInfoUbo.release();
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
		WARN("Scene has more MeshObjects (%zu) than MAX_RTX_INSTANCES (%u); TLAS will be clamped.",
			meshObjects.size(), MAX_RTX_INSTANCES)
	}

	ViewInfo viewInfo = getCameraViewInfo();
	gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()].viewInfoUbo.writeData(&viewInfo, sizeof(viewInfo));

	{// rebuild TLAS in the command buffer
		SCOPED_DRAW_EVENT(cmdbuf, "rebuild TLAS")
		sceneTlas.build_from_meshes(
			cmdbuf,
			Vulkan::Instance->getCurrentFrameIndex(),
			meshObjects,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
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

		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline.pipeline);
		fd.descriptorSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 0, rtPipeline.layout);
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
