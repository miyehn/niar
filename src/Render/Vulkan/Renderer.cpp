#include "Renderer.h"
#include "Asset/Mesh.h"
#include "Asset/Material.h"
#include "RenderPassBuilder.h"

void SimpleRenderer::render()
{
	auto cmdbuf = Vulkan::Instance->beginFrame();
	// render to RT, etc...
	Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
	for (auto drawable : drawables)
	{
		if (Mesh* m = dynamic_cast<Mesh*>(drawable))
		{
			m->material->set_parameters(m);
			m->material->use(cmdbuf);
			m->draw(cmdbuf);
		}
	}
	Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
	Vulkan::Instance->endFrame();
}

AnotherRenderer::AnotherRenderer()
{
	swapChainExtent = Vulkan::Instance->swapChainExtent;
	{
		// create outColor
		VkImageCreateInfo imageInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.extent = {swapChainExtent.width, swapChainExtent.height, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		};
		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		EXPECT(vmaCreateImage(Vulkan::Instance->memoryAllocator, &imageInfo, &allocInfo, &outColor.image, &outColor.allocation, nullptr), VK_SUCCESS)
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, outColor.image, "outColor")

		// create outColor view
		VkImageViewCreateInfo viewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = outColor.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};
		EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &outColorView), VK_SUCCESS)
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, outColorView, "outColorView")
	}

	{
		// create depth
		VkImageCreateInfo dimgInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT,
			.extent = {swapChainExtent.width, swapChainExtent.height, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		};
		VmaAllocationCreateInfo dimgAllocInfo = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		EXPECT(vmaCreateImage(Vulkan::Instance->memoryAllocator, &dimgInfo, &dimgAllocInfo, &outDepth.image, &outDepth.allocation, nullptr), VK_SUCCESS)
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, outDepth.image, "outDepth")

		// create depth view
		VkImageViewCreateInfo dimgViewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.image = outDepth.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT,
			.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
		};
		EXPECT(vkCreateImageView(Vulkan::Instance->device, &dimgViewInfo, nullptr, &outDepthView), VK_SUCCESS)
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, outDepthView, "outDepthView")
	}

	{
		// create renderpass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back(
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout when attachment is loaded
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		});
		passBuilder.depthAttachment = {
			.format = VK_FORMAT_D32_SFLOAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		VkAttachmentReference colorAttachmentRef = {
			// matches layout(location=X)
			.attachment = 0,
			// "which layout we'd like it to have during this subpass"
			// so, initialLayout -> this -> finalLayout ?
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		VkAttachmentReference depthAttachmentRef = {
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
				.pDepthStencilAttachment = &depthAttachmentRef
			});
		intermediatePass = passBuilder.build(Vulkan::Instance->device);
	}

	{
		// create intermediate FB
		VkImageView attachments[] = {outColorView, outDepthView};

		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = intermediatePass, // the render pass it needs to be compatible with
			.attachmentCount = 2,
			.pAttachments = attachments, // a pointer to an array of VkImageView handles, each of which will be used as the corresponding attachment in a render pass instance.
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1
		};

		EXPECT(vkCreateFramebuffer(Vulkan::Instance->device, &framebufferInfo, nullptr, &intermediateFB), VK_SUCCESS)
	}

}

void AnotherRenderer::render()
{
	auto cmdbuf = Vulkan::Instance->beginFrame();

	VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.f;
	VkClearValue clearValues[] = { clearColor, clearDepth };

	VkRect2D renderArea = { .offset = {0, 0}, .extent = swapChainExtent };
	VkRenderPassBeginInfo intermediatePassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = intermediatePass,
		.framebuffer = intermediateFB,
		.renderArea = renderArea,
		.clearValueCount = 2,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(cmdbuf, &intermediatePassInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		for (auto drawable : drawables)
		{
			if (Mesh* m = dynamic_cast<Mesh*>(drawable))
			{
				m->material->set_parameters(m);
				m->material->use(cmdbuf);
				m->draw(cmdbuf);
			}
		}
	}
	vkCmdEndRenderPass(cmdbuf);

	//----------------- copy to screen (?) ----------------

	Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
	{
		// TODO
	}
	Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
	Vulkan::Instance->endFrame();
}

AnotherRenderer::~AnotherRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyRenderPass(vk->device, intermediatePass, nullptr);
	vkDestroyFramebuffer(vk->device, intermediateFB, nullptr);

	vkDestroyImageView(vk->device, outColorView, nullptr);
	vmaDestroyImage(vk->memoryAllocator, outColor.image, outColor.allocation);
	vkDestroyImageView(vk->device, outDepthView, nullptr);
	vmaDestroyImage(vk->memoryAllocator, outDepth.image, outDepth.allocation);
}

AnotherRenderer *anotherRenderer = nullptr;

AnotherRenderer *AnotherRenderer::get()
{
	if (anotherRenderer == nullptr)
	{
		anotherRenderer = new AnotherRenderer();
	}
	return anotherRenderer;
}

void AnotherRenderer::cleanup()
{
	if (anotherRenderer) delete anotherRenderer;
}
