#include "DeferredRenderer.h"
#include "Render/Vulkan/RenderPassBuilder.h"
#include "Render/Texture.h"
#include "Render/Mesh.h"
#include "Render/Materials/Material.h"
#include "Render/Materials/DeferredLighting.h"
#include "Scene/Light.hpp"
#include "Render/Vulkan/VulkanUtils.h"

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
		// TODO: relax the synchronization constraints
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

	{// frame-global descriptor set

		viewInfoUbo = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								  sizeof(ViewInfo),
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU);


		DescriptorSetLayout frameGlobalSetLayout{};
		frameGlobalSetLayout.addBinding(0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet = DescriptorSet(frameGlobalSetLayout);

		frameGlobalDescriptorSet.pointToBuffer(viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalDescriptorSet.pointToImageView(GPosition->imageView, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToImageView(GNormal->imageView, 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToImageView(GColor->imageView, 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToImageView(GMetallicRoughnessAO->imageView, 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
	}
}

DeferredRenderer::~DeferredRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyRenderPass(vk->device, renderPass, nullptr);
	vkDestroyFramebuffer(vk->device, framebuffer, nullptr);
	viewInfoUbo.release();

	std::vector<Texture2D*> images = {
		GPosition, GNormal, GColor, GMetallicRoughnessAO, sceneColor, sceneDepth
	};
	for (auto image : images) delete image;
}

void DeferredRenderer::render(VkCommandBuffer cmdbuf)
{
	{// update frame-global uniforms
		memset(&ViewInfo, 0, sizeof(ViewInfo)); // clear
		ViewInfo.CameraPosition = camera->world_position();
		ViewInfo.ProjectionMatrix = camera->camera_to_clip();
		ViewInfo.ProjectionMatrix[1][1] *= -1; // so it's not upside down
		ViewInfo.ViewDir = camera->forward();
		ViewInfo.ViewMatrix = camera->world_to_object();
		viewInfoUbo.writeData(&ViewInfo);
	}

	std::vector<SceneObject*> drawables;
	drawable->foreach_descendent_bfs([&drawables](SceneObject* child) {
		drawables.push_back(child);
	}, [](SceneObject *obj){ return obj->enabled; });

	VkClearValue clearColor = {0, 0, 0, 1.0f};
	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.f;
	VkClearValue clearValues[] = { clearColor, clearColor, clearColor, clearColor, clearColor, clearDepth };
	VkRect2D renderArea = { .offset = {0, 0}, .extent = renderExtent };
	VkRenderPassBeginInfo passInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffer,
		.renderArea = renderArea,
		.clearValueCount = 6,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Base pass")
		// deferred base pass: draw the meshes with materials
		Material* last_material = nullptr;
		uint32_t instance_ctr = 0;
		for (auto drawable : drawables) // TODO: material sorting, etc.
		{
			if (Mesh* m = dynamic_cast<Mesh*>(drawable))
			{
				auto mat = m->get_material();
				if (mat != last_material)
				{
					m->get_material()->usePipeline(cmdbuf, {
						{frameGlobalDescriptorSet, DSET_FRAMEGLOBAL}
					});
					last_material = mat;
					instance_ctr = 0;
				}

				m->draw(cmdbuf);
				instance_ctr++;
			}
		}

		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Lighting pass")
		MatDeferredLighting* mat_lighting =
			dynamic_cast<MatDeferredLighting*>(Material::find("DeferredLighting"));

		int point_light_ctr = 0;
		int directional_light_ctr = 0;
		for (auto drawable : drawables)
		{
			if (auto L = dynamic_cast<PointLight*>(drawable))
			{
				if (point_light_ctr >= MAX_LIGHTS_PER_PASS) break;
				mat_lighting->pointLights.Data[point_light_ctr].position = L->world_position();
				mat_lighting->pointLights.Data[point_light_ctr].color = L->get_emission();
				point_light_ctr++;
			}
			else if (auto L = dynamic_cast<DirectionalLight*>(drawable))
			{
				if (directional_light_ctr >= MAX_LIGHTS_PER_PASS) break;
				mat_lighting->directionalLights.Data[directional_light_ctr].direction = L->get_direction();
				mat_lighting->directionalLights.Data[directional_light_ctr].color = L->get_emission();
				directional_light_ctr++;
			}
		}

		// update ViewInfo to include num lights
		ViewInfo.NumPointLights = point_light_ctr;
		ViewInfo.NumDirectionalLights = directional_light_ctr;
		viewInfoUbo.writeData(&ViewInfo);

		mat_lighting->numPointLights = point_light_ctr;
		mat_lighting->numDirectionalLights = directional_light_ctr;
		mat_lighting->usePipeline(cmdbuf, {
			{frameGlobalDescriptorSet, DSET_FRAMEGLOBAL}
		});
		vk::draw_fullscreen_triangle(cmdbuf);

		vkCmdEndRenderPass(cmdbuf);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Post Processing")
		// MatPostProcessing* mat_postprocessing = dynamic_cast<MatPostProcessing*>(Material::find("Post Processing"));
		// TODO

		vk::blitToScreen(
			cmdbuf,
			sceneColor->resource.image,
			{0, 0, 0},
			{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});
	}
}

DeferredRenderer* deferredRenderer = nullptr;

DeferredRenderer *DeferredRenderer::get()
{
	if (deferredRenderer == nullptr)
	{
		deferredRenderer = new DeferredRenderer();
		new MatDeferredLighting(deferredRenderer);
		// new MatPostProcessing(deferredRenderer->sceneColor, deferredRenderer->sceneDepth);
	}
	return deferredRenderer;
}

void DeferredRenderer::cleanup()
{
	if (deferredRenderer) delete deferredRenderer;
}
