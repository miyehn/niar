#include "DeferredRenderer.h"
#include "RenderPassBuilder.h"
#include "ImageCreator.h"

#define GPOSITION_ATTACHMENT 0
#define GNORMAL_ATTACHMENT 1
#define GCOLOR_ATTACHMENT 2
#define GMETALLICROUGHNESSAO_ATTACHMENT 3
#define GSCENECOLOR_ATTACHMENT 4
#define GSCENEDEPTH_ATTACHMENT 5

DeferredRenderer::DeferredRenderer()
{
	renderExtent = Vulkan::Instance->swapChainExtent;
	{// images
		ImageCreator GPositionCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GPosition");

		ImageCreator GNormalCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GNormal");

		ImageCreator GColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GColor");

		ImageCreator GMetallicRoughnessAOCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GMetallicRoughnessAO");

		ImageCreator sceneDepthCreator(
			VK_FORMAT_D32_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			"sceneDepth");

		ImageCreator sceneColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"sceneColor");

		GPositionCreator.create(GPosition, GPositionView);
		GNormalCreator.create(GNormal, GNormalView);
		GColorCreator.create(GColor, GColorView);
		GMetallicRoughnessAOCreator.create(GMetallicRoughnessAO, GMetallicRoughnessAOView);
		sceneColorCreator.create(sceneColor, sceneColorView);
		sceneDepthCreator.create(sceneDepth, sceneDepthView);
	}

	{// renderpass
		RenderPassBuilder passBuilder;

		// GPosition
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
		// GNormal
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
		// GColor
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
		// GMetallicRoughnessAO
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
		// sceneColor
		passBuilder.colorAttachments.push_back(
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout when attachment is loaded
				.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			});
		// sceneDepth
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

		// base pass
		std::vector<VkAttachmentReference> basePassColorAttachmentRefs = {
			{GPOSITION_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{GNORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{GCOLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{GMETALLICROUGHNESSAO_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		VkAttachmentReference depthAttachmentReference = {
			GSCENEDEPTH_ATTACHMENT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = static_cast<uint32_t>(basePassColorAttachmentRefs.size()),
				.pColorAttachments = basePassColorAttachmentRefs.data(), // an array, index matches layout (location=X) out vec4 outColor
				.pDepthStencilAttachment = &depthAttachmentReference
			});

		// composition pass
		VkAttachmentReference sceneColorAttachmentReference = {
			GSCENECOLOR_ATTACHMENT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &sceneColorAttachmentReference,
				.pDepthStencilAttachment = nullptr
			});

		// dependencies
		passBuilder.dependencies.push_back(
			{
				.srcSubpass = 0,
				.dstSubpass = 1,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
			});

		// build the renderpass
		renderPass = passBuilder.build(Vulkan::Instance->device);
	}

	{// framebuffer
		VkImageView attachments[] = {
			GPositionView,
			GNormalView,
			GColorView,
			GMetallicRoughnessAOView,
			sceneColorView,
			sceneDepthView
		};
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass, // the render pass it needs to be compatible with
			.attachmentCount = 6,
			.pAttachments = attachments, // a pointer to an array of VkImageView handles, each of which will be used as the corresponding attachment in a render pass instance.
			.width = renderExtent.width,
			.height = renderExtent.height,
			.layers = 1
		};
		EXPECT(vkCreateFramebuffer(Vulkan::Instance->device, &framebufferInfo, nullptr, &framebuffer), VK_SUCCESS)
	}
}

DeferredRenderer::~DeferredRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyRenderPass(vk->device, renderPass, nullptr);
	vkDestroyFramebuffer(vk->device, framebuffer, nullptr);

	std::vector<VkImageView> imageViews = {
		GPositionView, GNormalView, GColorView, GMetallicRoughnessAOView, sceneColorView, sceneDepthView
	};
	for (auto imageView : imageViews) vkDestroyImageView(vk->device, imageView, nullptr);

	std::vector<VmaAllocatedImage> images = {
		GPosition, GNormal, GColor, GMetallicRoughnessAO, sceneColor, sceneDepth
	};
	for (auto image : images) vmaDestroyImage(vk->memoryAllocator, image.image, image.allocation);
}

void DeferredRenderer::render()
{

}

DeferredRenderer* deferredRenderer = nullptr;

DeferredRenderer *DeferredRenderer::get()
{
	if (deferredRenderer == nullptr)
	{
		deferredRenderer = new DeferredRenderer();
	}
	return deferredRenderer;
}

void DeferredRenderer::cleanup()
{
	if (deferredRenderer) delete deferredRenderer;
}
