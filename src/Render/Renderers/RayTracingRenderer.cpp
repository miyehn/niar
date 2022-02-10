//
// Created by raind on 1/29/2022.
//

#include "RayTracingRenderer.h"
#include "Scene/RtxTriangle.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Texture.h"
#include "Render/Vulkan/RenderPassBuilder.h"

RayTracingRenderer::RayTracingRenderer()
{
	{// descriptor set (ubo)
		uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								  sizeof(Uniforms),
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU);
		DescriptorSetLayout layout{};
		layout.addBinding(0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptorSet = DescriptorSet(layout);
		descriptorSet.pointToBuffer(uniformBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}

	renderExtent = Vulkan::Instance->swapChainExtent;

	{// image
		ImageCreator sceneColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"sceneColor(rtx)");
		sceneColor = new Texture2D(sceneColorCreator);
	}

	{// render pass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back(
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			});

		passBuilder.useDepthAttachment = false;

		VkAttachmentReference colorAttachmentRef = {
			0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
				.pDepthStencilAttachment = nullptr
			});

		renderPass = passBuilder.build(Vulkan::Instance);
	}

	{// frame buffer
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = 1,
			.pAttachments = &sceneColor->imageView,
			.width = renderExtent.width,
			.height = renderExtent.height,
			.layers = 1
		};
		EXPECT(vkCreateFramebuffer(
			Vulkan::Instance->device,
			&frameBufferInfo,
			nullptr,
			&frameBuffer), VK_SUCCESS)
	}
}

RayTracingRenderer::~RayTracingRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyFramebuffer(vk->device, frameBuffer, nullptr);
	uniformBuffer.release();
	delete sceneColor;
}

void RayTracingRenderer::render(VkCommandBuffer cmdbuf)
{
	std::vector<RtxTriangle*> triangles;
	drawable->foreach_descendent_bfs([&triangles](SceneObject* child){
		if (auto tri = dynamic_cast<RtxTriangle*>(child)) {
			if (tri->enabled) triangles.push_back(tri);
		}
	});

	//====

	/*
	VkClearValue clearColor = {0.2f, 0.3f, 0.4f, 1.0f};
	VkRect2D renderArea = { .offset = {0, 0}, .extent = renderExtent };
	VkRenderPassBeginInfo passInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = frameBuffer,
		.renderArea = renderArea,
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};
	vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		SCOPED_DRAW_EVENT(cmdbuf, "rtx pass")
		for (auto& tri : triangles) tri->draw(cmdbuf);
	}
	vkCmdEndRenderPass(cmdbuf);
	 */

	if (outImage == nullptr) return;

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
		Vulkan::Instance->destructionQueue.emplace_back([](){ delete renderer; });
	}
	return renderer;
}

void RayTracingRenderer::debugSetup(std::function<void()> fn)
{
	if (fn) fn();
}
