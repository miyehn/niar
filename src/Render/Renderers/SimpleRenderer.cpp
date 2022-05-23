//
// Created by raind on 5/16/2022.
//

#include "Render/Materials/GltfMaterial.h"
#include "Render/Mesh.h"
#include "Render/Vulkan/RenderPassBuilder.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "SimpleRenderer.h"
#include "Render/Texture.h"
#include "Render/DebugDraw.h"

SimpleRenderer::SimpleRenderer()
{
	renderExtent = Vulkan::Instance->swapChainExtent;
	{// images
		ImageCreator sceneColorCreator(
			VK_FORMAT_R8G8B8A8_UNORM,
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

		sceneColor = new Texture2D(sceneColorCreator);
		sceneDepth = new Texture2D(sceneDepthCreator);
	}

	{// render pass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back(
			{
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			});

		passBuilder.useDepthAttachment = true;
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
			0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		VkAttachmentReference depthAttachmentRef = {
			1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
				.pDepthStencilAttachment = &depthAttachmentRef
			});

		renderPass = passBuilder.build(Vulkan::Instance);
	}

	{// frame buffer
		VkImageView attachments[] = {
			sceneColor->imageView,
			sceneDepth->imageView
		};
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = 2,
			.pAttachments = attachments,
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

	{// descriptor set (ubo)
		viewInfoUbo = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								  sizeof(ViewInfo),
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU);
		DescriptorSetLayout layout{};
		layout.addBinding(0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptorSet = DescriptorSet(layout);
		descriptorSet.pointToBuffer(viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}

	{// debug draw setup
		debugLines = new DebugLines(viewInfoUbo, renderPass);
		// x axis
		debugLines->addSegment(
			PointData(glm::vec3(-10, 0, 0), glm::u8vec4(255, 0, 0, 255)),
			PointData(glm::vec3(10, 0, 0), glm::u8vec4(255, 0, 0, 255)));
		// y axis
		debugLines->addSegment(
			PointData(glm::vec3(0, -10, 0), glm::u8vec4(0, 255, 0, 255)),
			PointData(glm::vec3(0, 10, 0), glm::u8vec4(0, 255, 0, 255)));
		// z axis
		debugLines->addSegment(
			PointData(glm::vec3(0, 0, -10), glm::u8vec4(0, 0, 255, 255)),
			PointData(glm::vec3(0, 0, 10), glm::u8vec4(0, 0, 255, 255)));

		debugLines->addBox(glm::vec3(-0.5f), glm::vec3(0.5f), glm::u8vec4(255, 255, 255, 255));
		debugLines->uploadVertexBuffer();
	}
}

SimpleRenderer::~SimpleRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyFramebuffer(vk->device, frameBuffer, nullptr);
	viewInfoUbo.release();
	delete sceneColor;
	delete sceneDepth;
	delete debugLines;
	for (const auto& p : materials) delete p.second;
}

SimpleRenderer *SimpleRenderer::get()
{
	static SimpleRenderer* renderer = nullptr;

	if (renderer == nullptr) renderer = new SimpleRenderer();

	return renderer;
}

void SimpleRenderer::updateViewInfoUbo()
{
	ViewInfo.ViewMatrix = camera->world_to_object();
	ViewInfo.ProjectionMatrix = camera->camera_to_clip();
	ViewInfo.ProjectionMatrix[1][1] *= -1; // so it's not upside down

	ViewInfo.CameraPosition = camera->world_position();
	ViewInfo.ViewDir = camera->forward();

	viewInfoUbo.writeData(&ViewInfo);
}

void SimpleRenderer::render(VkCommandBuffer cmdbuf)
{
	// reset material instance counters
	for (auto it : materials)
	{
		it.second->resetInstanceCounter();
	}

	// prepare frameglobals
	updateViewInfoUbo();

	std::vector<SceneObject*> drawables;
	drawable->foreach_descendent_bfs([&drawables](SceneObject* child) {
		drawables.push_back(child);
	}, [](SceneObject *obj){ return obj->enabled(); });

	VkClearValue clearColor = {0.2f, 0.3f, 0.4f, 1.0f};
	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.f;
	VkClearValue clearValues[] = { clearColor, clearDepth };
	VkRect2D renderArea = { .offset = {0, 0}, .extent = renderExtent };
	VkRenderPassBeginInfo passInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = frameBuffer,
		.renderArea = renderArea,
		.clearValueCount = 2,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		SCOPED_DRAW_EVENT(cmdbuf, "SimpleRenderer draw")
		// deferred base pass: draw the meshes with materials
		Material* last_material = nullptr;
		//VkPipeline last_pipeline = VK_NULL_HANDLE;
		MaterialPipeline last_pipeline = {};
		uint32_t instance_ctr = 0;
		for (auto drawable : drawables) // TODO: material (pipeline) sorting, etc.
		{
			if (Mesh* m = dynamic_cast<Mesh*>(drawable))
			{
				auto mat = getOrCreateMeshMaterial(m->materialName);
				auto pipeline = mat->getPipeline();

				// pipeline changed: re-bind; re-set frame globals if necessary
				if (pipeline != last_pipeline)
				{
					mat->usePipeline(cmdbuf);
					if (pipeline.layout != last_pipeline.layout)
					{
						descriptorSet.bind(
							cmdbuf,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							DSET_FRAMEGLOBAL,
							pipeline.layout);
					}
					last_pipeline = pipeline;
				}

				// material changed: reset instance counter
				if (mat != last_material)
				{
					instance_ctr = 0;
					last_material = mat;
				}

				mat->setParameters(cmdbuf, m);
				m->draw(cmdbuf);
				instance_ctr++;
			}
		}
		{
			SCOPED_DRAW_EVENT(cmdbuf, "debug draw")
			debugLines->bindAndDraw(cmdbuf);
		}
	}
	vkCmdEndRenderPass(cmdbuf);

	vk::blitToScreen(
		cmdbuf,
		sceneColor->resource.image,
		{0, 0, 0},
		{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});
}

/*
 * find if this material is in the pool. If it is, and its version matches with info, just return.
 * Otherwise need to create a new one:
 *  - if material is not in the pool at all, just create it.
 *  - if it IS in the pool but version doesn't match, the old one is obsolete and need to be cleaned up
 *    and then create a new one from the up-to-date info
 */
Material *SimpleRenderer::getOrCreateMeshMaterial(const std::string &materialName)
{
	auto iter = materials.find(materialName);
	GltfMaterialInfo* info = GltfMaterialInfo::get(materialName);
	EXPECT(info != nullptr, true)

	if (iter != materials.end()) {
		auto pooled_mat = iter->second;
		if (pooled_mat->getVersion() == info->_version) {
			// up to date; just return it
			return pooled_mat;
		}
		else {
			// pooled material is obsolete; delete it.
			delete pooled_mat;
		}
	}

	// create a new one
	auto newMaterial = new SimpleGltfMaterial(*info);
	materials[newMaterial->name] = newMaterial;

	return newMaterial;
}
