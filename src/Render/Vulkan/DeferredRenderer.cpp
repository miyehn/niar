#include "DeferredRenderer.h"
#include "RenderPassBuilder.h"
#include "Asset/Texture.h"
#include "ImageCreator.h"
#include "Asset/Mesh.h"
#include "Asset/Material.h"
#include "Asset/DeferredPointLighting.h"
#include "VulkanUtils.h"

#define GPOSITION_ATTACHMENT 0
#define GNORMAL_ATTACHMENT 1
#define GCOLOR_ATTACHMENT 2
#define GMETALLICROUGHNESSAO_ATTACHMENT 3
#define SCENECOLOR_ATTACHMENT 4
#define SCENEDEPTH_ATTACHMENT 5

DeferredRenderer::DeferredRenderer()
{
	renderExtent = Vulkan::Instance->swapChainExtent;
	{// images
		ImageCreator GPositionCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GPosition");

		ImageCreator GNormalCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GNormal");

		ImageCreator GColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GColor");

		ImageCreator GMetallicRoughnessAOCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GMetallicRoughnessAO");

		ImageCreator sceneColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"sceneColor");

		ImageCreator sceneDepthCreator(
			VK_FORMAT_D32_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			"sceneDepth");

		GPosition = new Texture2D(GPositionCreator);
		GNormal = new Texture2D(GNormalCreator);
		GColor = new Texture2D(GColorCreator);
		GMetallicRoughnessAO = new Texture2D(GMetallicRoughnessAOCreator);
		sceneColor = new Texture2D(sceneColorCreator);
		sceneDepth = new Texture2D(sceneDepthCreator);
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
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
			SCENEDEPTH_ATTACHMENT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = static_cast<uint32_t>(basePassColorAttachmentRefs.size()),
				.pColorAttachments = basePassColorAttachmentRefs.data(), // an array, index matches layout (location=X) out vec4 outColor
				.pDepthStencilAttachment = &depthAttachmentReference
			});

		// point lighting pass
		std::vector<VkAttachmentReference> pointLightingInputAttachmentRefs = {
			{GPOSITION_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{GNORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{GCOLOR_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{GMETALLICROUGHNESSAO_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
		};
		std::vector<VkAttachmentReference> pointLightingColorAttachmentRefs = {
			{SCENECOLOR_ATTACHMENT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				// input attachments
				.inputAttachmentCount = static_cast<uint32_t>(pointLightingInputAttachmentRefs.size()),
				.pInputAttachments = pointLightingInputAttachmentRefs.data(),
				// output attachments
				.colorAttachmentCount = static_cast<uint32_t>(pointLightingColorAttachmentRefs.size()),
				.pColorAttachments = pointLightingColorAttachmentRefs.data(),
				.pDepthStencilAttachment = nullptr
			});

		// dependencies
		passBuilder.dependencies.push_back(
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			});
		passBuilder.dependencies.push_back(
			{
				.srcSubpass = 0,
				.dstSubpass = 1,
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			});
		passBuilder.dependencies.push_back(
			{
				.srcSubpass = 1,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			});

		// build the renderpass
		renderPass = passBuilder.build(Vulkan::Instance->device);
	}

	{// framebuffer
		VkImageView attachments[] = {
			GPosition->imageView,
			GNormal->imageView,
			GColor->imageView,
			GMetallicRoughnessAO->imageView,
			sceneColor->imageView,
			sceneDepth->imageView
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
		EXPECT(vkCreateFramebuffer(
			Vulkan::Instance->device,
			&framebufferInfo,
			nullptr,
			&framebuffer), VK_SUCCESS)
	}
}

DeferredRenderer::~DeferredRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyRenderPass(vk->device, renderPass, nullptr);
	vkDestroyFramebuffer(vk->device, framebuffer, nullptr);

	std::vector<Texture2D*> images = {
		GPosition, GNormal, GColor, GMetallicRoughnessAO, sceneColor, sceneDepth
	};
	for (auto image : images) delete image;
}

void DeferredRenderer::render()
{
	auto cmdbuf = Vulkan::Instance->beginFrame();

	VkClearValue clearColor = {0, 0, 0, 1.0f};
	VkClearValue clearColor2 = {0.9f, 0.5f, 0.3f, 1.0f};
	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.f;
	VkClearValue clearValues[] = { clearColor, clearColor, clearColor, clearColor, clearColor2, clearDepth };

	VkRect2D renderArea = { .offset = {0, 0}, .extent = renderExtent };
	VkRenderPassBeginInfo intermediatePassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffer,
		.renderArea = renderArea,
		.clearValueCount = 6,
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
	vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
	{
		DeferredPointLighting* lighting = dynamic_cast<DeferredPointLighting*>(Material::find("deferred lighting pass"));

		lighting->uniforms.CameraPosition = Camera::Active->position;
		lighting->uniforms.NumLights = 0;

		lighting->use(cmdbuf);
		vk::draw_fullscreen_triangle(cmdbuf);
	}
	vkCmdEndRenderPass(cmdbuf);

	vk::blitToScreen(
		cmdbuf,
		sceneColor->resource.image,
		{0, 0, 0},
		{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});

	Vulkan::Instance->endFrame();
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
