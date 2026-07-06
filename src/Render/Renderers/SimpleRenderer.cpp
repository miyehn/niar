//
// Created by raind on 5/16/2022.
//

#include "Render/Mesh.h"
#include "Scene/MeshObject.h"
#include "Render/BindlessResources.h"
#include "Render/Vulkan/RenderPassBuilder.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "SimpleRenderer.h"
#include "Render/Texture.h"
#include "Render/DebugDraw.h"
#include <memory>
#include <unordered_map>

namespace {

struct SimpleGltfPipelineKey {
	bool doubleSided = false;

	explicit SimpleGltfPipelineKey(const MeshSurface& surface)
		: doubleSided(surface.doubleSided)
	{}

	bool operator==(const SimpleGltfPipelineKey&) const = default;
	bool operator<(const SimpleGltfPipelineKey& other) const {
		return hash() < other.hash();
	}

	size_t hash() const noexcept {
		return static_cast<size_t>(doubleSided);
	}

	struct Hash {
		size_t operator()(const SimpleGltfPipelineKey& key) const noexcept {
			return key.hash();
		}
	};
};

class SimpleGltfPipelineCache {
public:
	static const GraphicsPipeline& get(const SimpleGltfPipelineKey& key) {
		auto it = pipelines.find(key);
		if (it != pipelines.end()) return *it->second;

		auto pipeline = std::make_unique<GraphicsPipeline>();
		auto vk = Vulkan::Instance;
		auto renderer = SimpleRenderer::get();
		auto& b = pipeline->builder;
		b.vertDef = "shaders/geometry.vert";
		b.fragDef = "shaders/simple_gltf.frag";
		b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		b.pipelineState.rasterizationInfo.cullMode =
			key.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		b.compatibleRenderPass = renderer->renderPass;
		b.compatibleSubpass = 0;

		b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, renderer->getFrameGlobalLayout());
		b.useDescriptorSetLayout(DSET_BINDLESS, BindlessResources::Instance->layout());
		b.usePushConstantRange({VK_SHADER_STAGE_VERTEX_BIT, GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE});
		b.usePushConstantRange({VK_SHADER_STAGE_FRAGMENT_BIT, GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE});

		b.pipelineState.colorBlendAttachments = { b.pipelineState.colorBlendAttachmentInfo };

		pipeline->build(key.doubleSided ? "SimpleGltf DoubleSided" : "SimpleGltf");

		auto [insertedIt, _] = pipelines.emplace(key, std::move(pipeline));
		return *insertedIt->second;
	}

	static void release() {
		pipelines.clear();
	}

private:
	inline static std::unordered_map<SimpleGltfPipelineKey, std::unique_ptr<GraphicsPipeline>, SimpleGltfPipelineKey::Hash> pipelines;
};

}

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

	{// per-frame descriptor set and debug draw (ring buffer)
		DescriptorSetLayout layout{};
		layout.addBinding(0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto& fd = gpuFrameData[i];

			fd.viewInfoUbo = VmaBuffer({&Vulkan::Instance->memoryAllocator,
								   sizeof(glm::ViewInfo),
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								   VMA_MEMORY_USAGE_CPU_TO_GPU,
								   "View info uniform buffer (simple renderer)"});
			fd.descriptorSet = DescriptorSet(layout);
			fd.descriptorSet.pointToBuffer(fd.viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

			fd.debugLines = new DebugLines(fd.descriptorSet.getLayout(), renderPass);
			// x axis
			fd.debugLines->addSegment(
				PointData(glm::vec3(-10, 0, 0), glm::u8vec4(255, 0, 0, 255)),
				PointData(glm::vec3(10, 0, 0), glm::u8vec4(255, 0, 0, 255)));
			// y axis
			fd.debugLines->addSegment(
				PointData(glm::vec3(0, -10, 0), glm::u8vec4(0, 255, 0, 255)),
				PointData(glm::vec3(0, 10, 0), glm::u8vec4(0, 255, 0, 255)));
			// z axis
			fd.debugLines->addSegment(
				PointData(glm::vec3(0, 0, -10), glm::u8vec4(0, 0, 255, 255)),
				PointData(glm::vec3(0, 0, 10), glm::u8vec4(0, 0, 255, 255)));
			fd.debugLines->addBox(glm::vec3(-0.5f), glm::vec3(0.5f), glm::u8vec4(255, 255, 255, 255));
			fd.debugLines->uploadVertexBuffer();
		}
	}

}

SimpleRenderer::~SimpleRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyFramebuffer(vk->device, frameBuffer, nullptr);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		gpuFrameData[i].viewInfoUbo.release();
		delete gpuFrameData[i].debugLines;
	}
	delete sceneColor;
	delete sceneDepth;
	SimpleGltfPipelineCache::release();
}

SimpleRenderer *SimpleRenderer::get()
{
	static SimpleRenderer* renderer = nullptr;

	if (renderer == nullptr) renderer = new SimpleRenderer();

	return renderer;
}

DescriptorSetLayout SimpleRenderer::getFrameGlobalLayout()
{
	return gpuFrameData[0].descriptorSet.getLayout();
}

void SimpleRenderer::render(VkCommandBuffer cmdbuf)
{
	// prepare frameglobals
	glm::ViewInfo viewInfo = getCameraViewInfo();
	viewInfo.RenderSize = glm::vec2(renderExtent.width, renderExtent.height);
	viewInfo.JitterOffset = {}; // no TAA for SimpleRenderer.
	gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()].viewInfoUbo.writeData(&viewInfo, sizeof(viewInfo));

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
		auto& fd = gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()];
		// deferred base pass: draw the meshes with materials
		//VkPipeline last_pipeline = VK_NULL_HANDLE;
		const GraphicsPipeline* last_pipeline = nullptr;
		for (auto drawable : drawables) // TODO: material (pipeline) sorting, etc.
		{
			if (auto* mo = dynamic_cast<MeshObject*>(drawable))
			{
				auto key = SimpleGltfPipelineKey(mo->mesh.surface);
				auto& pipeline = SimpleGltfPipelineCache::get(key);

				// pipeline changed: re-bind; re-set frame globals if necessary
				if (!last_pipeline || pipeline != *last_pipeline)
				{
					vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
					if (!last_pipeline || pipeline.layout != last_pipeline->layout)
					{
						fd.descriptorSet.bind(
							cmdbuf,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							DSET_FRAMEGLOBAL,
							pipeline.layout);
						BindlessResources::Instance->descriptorSet().bind(
							cmdbuf,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							DSET_BINDLESS,
							pipeline.layout);
					}
					last_pipeline = &pipeline;
				}

				{ // each object's push constants
					glm::mat4 modelMatrix = mo->object_to_world();
					vkCmdPushConstants(cmdbuf, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
						GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE, &modelMatrix);

					const uint32_t bindlessMaterialIndex = mo->mesh.surface.bindlessMaterialIndex;
					ASSERT(bindlessMaterialIndex != INVALID_BINDLESS_INDEX)
#if TMP_BINDLESS_DEBUG
					BindlessResources::Instance->assertMaterialIndexOccupied(bindlessMaterialIndex);
#endif
					vkCmdPushConstants(cmdbuf, pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT,
						GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE, &bindlessMaterialIndex);

				}

				mo->draw(cmdbuf);
			}
		}
		{
			SCOPED_DRAW_EVENT(cmdbuf, "debug draw")
			fd.descriptorSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_FRAMEGLOBAL, fd.debugLines->getPipelineLayout());
			fd.debugLines->bindAndDraw(cmdbuf);
		}
	}
	vkCmdEndRenderPass(cmdbuf);

	vk::blitToScreen(
		cmdbuf,
		sceneColor->resource.image,
		{0, 0, 0},
		{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});
}
