//
// Created by raind on 1/29/2022.
//

#include "RayTracingRenderer.h"
#include "Scene/RtxTriangle.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Texture.h"
#include "Render/Vulkan/RenderPassBuilder.h"

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
	delete outImage;
}

void RayTracingRenderer::render(VkCommandBuffer cmdbuf)
{
	if (outImage == nullptr) return;

	/*
	std::vector<RtxTriangle*> triangles;
	drawable->foreach_descendent_bfs([&triangles](SceneObject* child){
		if (auto tri = dynamic_cast<RtxTriangle*>(child)) {
			if (tri->enabled()) triangles.push_back(tri);
		}
	});
	*/

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
