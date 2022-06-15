#include "DeferredRenderer.h"
#include "Render/Vulkan/RenderPassBuilder.h"
#include "Render/Texture.h"
#include "Render/Mesh.h"
#include "Scene/MeshObject.h"
#include "Scene/EnvMapVisualizer.h"
#include "Render/Materials/GltfMaterial.h"
#include "Render/DebugDraw.h"
#include "Scene/Light.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include <imgui.h>

class PostProcessing : public Material
{
public:
	MaterialPipeline getPipeline() override
	{
		static MaterialPipeline materialPipeline = {};
		if (materialPipeline.pipeline == VK_NULL_HANDLE || materialPipeline.layout == VK_NULL_HANDLE)
		{
			auto vk = Vulkan::Instance;
			// build the pipeline
			GraphicsPipelineBuilder pipelineBuilder{};
			pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
			pipelineBuilder.fragPath = "spirv/post_processing.frag.spv";
			pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			pipelineBuilder.pipelineState.useVertexInput = false;
			pipelineBuilder.pipelineState.useDepthStencil = false;
			pipelineBuilder.compatibleRenderPass = postProcessPass;
			pipelineBuilder.compatibleSubpass = DEFERRED_SUBPASS_POSTPROCESSING;

			DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
			DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
			pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

			pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
		}

		return materialPipeline;
	}
	void usePipeline(VkCommandBuffer cmdbuf) override
	{
		MaterialPipeline materialPipeline = getPipeline();
		dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, materialPipeline.layout);
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, materialPipeline.pipeline);
	}

private:

	explicit PostProcessing(DeferredRenderer* renderer, Texture2D* sceneColor, Texture2D* sceneDepth)
	{
		name = "Post Processing";
		postProcessPass = renderer->postProcessPass;

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet = DescriptorSet(dynamicSetLayout);

		// assign values
		dynamicSet.pointToImageView(sceneColor->imageView, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet.pointToImageView(sceneDepth->imageView, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}

	VkRenderPass postProcessPass;
	DescriptorSet dynamicSet;

	friend class DeferredRenderer;
};

class DeferredLighting : public Material
{

#define MAX_LIGHTS_PER_PASS 128 // 4KB if each light takes { vec4, vec4 }. Must not exceed definition in shader.
public:

	MaterialPipeline getPipeline() override
	{
		static MaterialPipeline materialPipeline = {};
		if (materialPipeline.pipeline == VK_NULL_HANDLE || materialPipeline.layout == VK_NULL_HANDLE)
		{
			auto vk = Vulkan::Instance;
			// build the pipeline
			GraphicsPipelineBuilder pipelineBuilder{};
			pipelineBuilder.vertPath = "spirv/fullscreen_triangle.vert.spv";
			pipelineBuilder.fragPath = "spirv/deferred_lighting.frag.spv";
			pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			pipelineBuilder.pipelineState.useVertexInput = false;
			pipelineBuilder.pipelineState.useDepthStencil = false;
			pipelineBuilder.compatibleRenderPass = mainRenderPass;
			pipelineBuilder.compatibleSubpass = DEFERRED_SUBPASS_LIGHTING;

			DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
			DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
			pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

			pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
		}

		return materialPipeline;
	}

	void usePipeline(VkCommandBuffer cmdbuf) override
	{
		pointLightsBuffer.writeData(&pointLights, numPointLights * sizeof(PointLightInfo));
		directionalLightsBuffer.writeData(&directionalLights, numDirectionalLights * sizeof(DirectionalLightInfo));

		MaterialPipeline materialPipeline = getPipeline();
		dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, materialPipeline.layout);
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, materialPipeline.pipeline);
	}
	~DeferredLighting() override
	{
		pointLightsBuffer.release();
		directionalLightsBuffer.release();
	}

	struct PointLightInfo {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

	struct DirectionalLightInfo {
		alignas(16) glm::vec3 direction;
		alignas(16) glm::vec3 color;
	};

	struct {
		PointLightInfo Data[MAX_LIGHTS_PER_PASS]; // need to match shader
	} pointLights;

	struct {
		DirectionalLightInfo Data[MAX_LIGHTS_PER_PASS]; // need to match shader
	} directionalLights;

	// optimization, set by renderer to decide how much data to actually write to uniform buffers
	uint32_t numPointLights, numDirectionalLights;

private:

	explicit DeferredLighting(DeferredRenderer* renderer)
	{
		name = "Deferred Lighting";
		mainRenderPass = renderer->renderPass;
		pointLightsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
									  sizeof(pointLights),
									  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
									  VMA_MEMORY_USAGE_CPU_TO_GPU);
		directionalLightsBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
											sizeof(directionalLights),
											VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
											VMA_MEMORY_USAGE_CPU_TO_GPU);

		{// create the layouts and build the pipeline

			// set layouts and allocation
			DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
			DescriptorSetLayout dynamicSetLayout{};
			dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			dynamicSet = DescriptorSet(dynamicSetLayout);

			// assign values
			dynamicSet.pointToBuffer(pointLightsBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			dynamicSet.pointToBuffer(directionalLightsBuffer, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}
	}

	VmaBuffer pointLightsBuffer;
	VmaBuffer directionalLightsBuffer;

	DescriptorSet dynamicSet;
	VkRenderPass mainRenderPass;

	friend class DeferredRenderer;

};

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
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"sceneColor");

		ImageCreator sceneDepthCreator(
			VK_FORMAT_D32_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			"sceneDepth");

		ImageCreator postProcessedCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"postProcessed");

		GPosition = new Texture2D(GPositionCreator);
		GNormal = new Texture2D(GNormalCreator);
		GColor = new Texture2D(GColorCreator);
		GMetallicRoughnessAO = new Texture2D(GMetallicRoughnessAOCreator);
		sceneColor = new Texture2D(sceneColorCreator);
		sceneDepth = new Texture2D(sceneDepthCreator);
		postProcessed = new Texture2D(postProcessedCreator);
	}

	{// main renderpass
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
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
		// sceneDepth
		passBuilder.useDepthAttachment = true;
		passBuilder.depthAttachment = {
			.format = VK_FORMAT_D32_SFLOAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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

		// lighting pass
		std::vector<VkAttachmentReference> lightingInputAttachmentRefs = {
			{GPOSITION_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{GNORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{GCOLOR_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{GMETALLICROUGHNESSAO_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
		};
		std::vector<VkAttachmentReference> lightingColorAttachmentRefs = {
			{SCENECOLOR_ATTACHMENT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		passBuilder.subpasses.push_back(
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				// input attachments
				.inputAttachmentCount = static_cast<uint32_t>(lightingInputAttachmentRefs.size()),
				.pInputAttachments = lightingInputAttachmentRefs.data(),
				// output attachments
				.colorAttachmentCount = static_cast<uint32_t>(lightingColorAttachmentRefs.size()),
				.pColorAttachments = lightingColorAttachmentRefs.data(),
				.pDepthStencilAttachment = nullptr
			});
		// probes visualization
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = lightingColorAttachmentRefs.data(),
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		// dependencies
		// TODO: any dependency w external needed?
		passBuilder.dependencies.push_back(
			{// basepass vs. lighting
				.srcSubpass = DEFERRED_SUBPASS_GEOMETRY,
				.dstSubpass = DEFERRED_SUBPASS_LIGHTING,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			});
		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_LIGHTING,
			.dstSubpass = DEFERRED_SUBPASS_PROBES,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		// build the renderpass
		renderPass = passBuilder.build(Vulkan::Instance);
	}

	{// post procesing pass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back(
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkAttachmentReference colorAttachmentRef = {
			0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		VkAttachmentReference depthAttachmentReference = {
			1,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		// post processing
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = nullptr
		});
		// debug stuff
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		// TODO: constraints correct?
		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_POSTPROCESSING,
			.dstSubpass = DEFERRED_SUBPASS_DEBUGDRAW,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		postProcessPass = passBuilder.build(Vulkan::Instance);
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

	{// also framebuffer for postprocessing
		VkImageView attachments[2] = { postProcessed->imageView, sceneDepth->imageView };
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = postProcessPass,
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
			&postProcessFramebuffer), VK_SUCCESS)
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

	// misc
	ViewInfo.Exposure = 0.0f;
	ViewInfo.ToneMappingOption = 1;

	deferredLighting = new DeferredLighting(this);
	postProcessing = new PostProcessing(this, sceneColor, sceneDepth);

	{// debug draw stuff
#if 1 // example debug points
		if (!debugPoints) debugPoints = new DebugPoints(viewInfoUbo, postProcessPass, DEFERRED_SUBPASS_DEBUGDRAW);
		debugPoints->addPoint(glm::vec3(0, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->addPoint(glm::vec3(1, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->addPoint(glm::vec3(2, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->addPoint(glm::vec3(3, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->uploadVertexBuffer();
#endif

		std::vector<PointData> lines;
		if (!debugLines) debugLines = new DebugLines(viewInfoUbo, postProcessPass, DEFERRED_SUBPASS_DEBUGDRAW);
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

DeferredRenderer::~DeferredRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyFramebuffer(vk->device, framebuffer, nullptr);
	vkDestroyFramebuffer(vk->device, postProcessFramebuffer, nullptr);
	viewInfoUbo.release();

	delete deferredLighting;
	delete postProcessing;

	std::vector<Texture2D*> images = {
		GPosition, GNormal, GColor, GMetallicRoughnessAO, sceneColor, sceneDepth, postProcessed
	};
	for (auto image : images) delete image;

	delete debugPoints;
	delete debugLines;

	for (const auto& p : materials) delete p.second;
}

void DeferredRenderer::updateViewInfoUbo()
{
	ViewInfo.ViewMatrix = camera->world_to_object();
	ViewInfo.ProjectionMatrix = camera->camera_to_clip();
	ViewInfo.ProjectionMatrix[1][1] *= -1; // so it's not upside down

	ViewInfo.CameraPosition = camera->world_position();
	ViewInfo.ViewDir = camera->forward();

	viewInfoUbo.writeData(&ViewInfo);
}

void DeferredRenderer::render(VkCommandBuffer cmdbuf)
{
	// reset material instance counters
	for (auto it : materials)
	{
		it.second->resetInstanceCounter();
	}

	updateViewInfoUbo();

	frameGlobalDescriptorSet.bind(
		cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,DSET_FRAMEGLOBAL, deferredLighting->getPipeline().layout);

	std::vector<SceneObject*> drawables;
	drawable->foreach_descendent_bfs([&drawables](SceneObject* child) {
		drawables.push_back(child);
	}, [](SceneObject *obj){ return obj->enabled(); });

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
		//VkPipeline last_pipeline = VK_NULL_HANDLE;
		MaterialPipeline last_pipeline = {};
		uint32_t instance_ctr = 0;
		for (auto drawable : drawables) // TODO: material (pipeline) sorting, etc.
		{
			if (auto* mo = dynamic_cast<MeshObject*>(drawable))
			{
				auto mat = getOrCreateMeshMaterial(mo->mesh->materialName);//mo->get_material();
				auto pipeline = mat->getPipeline();

				// pipeline changed: re-bind; re-set frame globals if necessary
				if (pipeline != last_pipeline)
				{
					mat->usePipeline(cmdbuf);
					last_pipeline = pipeline;
				}

				// material changed: reset instance counter
				if (mat != last_material)
				{
					instance_ctr = 0;
					last_material = mat;
				}

				mat->setParameters(cmdbuf, mo);
				mo->draw(cmdbuf);
				instance_ctr++;
			}
		}

		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Lighting pass")

		int point_light_ctr = 0;
		int directional_light_ctr = 0;
		for (auto drawable : drawables)
		{
			if (auto L = dynamic_cast<PointLight*>(drawable))
			{
				if (point_light_ctr >= MAX_LIGHTS_PER_PASS) break;
				deferredLighting->pointLights.Data[point_light_ctr].position = L->world_position();
				deferredLighting->pointLights.Data[point_light_ctr].color = L->get_emission() / (4 * PI);
				point_light_ctr++;
			}
			else if (auto L = dynamic_cast<DirectionalLight*>(drawable))
			{
				if (directional_light_ctr >= MAX_LIGHTS_PER_PASS) break;
				deferredLighting->directionalLights.Data[directional_light_ctr].direction = L->get_direction();
				deferredLighting->directionalLights.Data[directional_light_ctr].color = L->get_emission();
				directional_light_ctr++;
			}
		}

		// update ViewInfo to include num lights
		ViewInfo.NumPointLights = point_light_ctr;
		ViewInfo.NumDirectionalLights = directional_light_ctr;
		viewInfoUbo.writeData(&ViewInfo);

		deferredLighting->numPointLights = point_light_ctr;
		deferredLighting->numDirectionalLights = directional_light_ctr;
		deferredLighting->usePipeline(cmdbuf);
		vk::drawFullscreenTriangle(cmdbuf);

		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
	}
	{
		SCOPED_DRAW_EVENT(cmdbuf, "Probes")
		bool firstInstance = true;
		auto mat = EnvMapVisualizer::get_material();
		for (auto drawable : drawables) // TODO: material (pipeline) sorting, etc.
		{
			if (auto* probe = dynamic_cast<EnvMapVisualizer*>(drawable)) {
				if (firstInstance) {
					mat->resetInstanceCounter();
					mat->usePipeline(cmdbuf);
				}
				mat->setParameters(cmdbuf, probe);
				probe->draw(cmdbuf);
				firstInstance = false;
			}
		}
		vkCmdEndRenderPass(cmdbuf);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Post Processing & Present")
		VkRenderPassBeginInfo passInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = postProcessPass,
			.framebuffer = postProcessFramebuffer, // TODO
			.renderArea = renderArea,
			.clearValueCount = 0,
			.pClearValues = nullptr
		};
		vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			SCOPED_DRAW_EVENT(cmdbuf, "Post processing")
			postProcessing->usePipeline(cmdbuf);
			vk::drawFullscreenTriangle(cmdbuf);
			vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
		}
		{
			if (drawDebug) {
				SCOPED_DRAW_EVENT(cmdbuf, "Debug draw")
				if (debugLines) debugLines->bindAndDraw(cmdbuf);
				if (debugPoints) debugPoints->bindAndDraw(cmdbuf);
			}
			vkCmdEndRenderPass(cmdbuf);
		}

		vk::blitToScreen(
			cmdbuf,
			postProcessed->resource.image,
			{0, 0, 0},
			{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});
	}
}

DeferredRenderer *DeferredRenderer::get()
{
	static DeferredRenderer* deferredRenderer = nullptr;

	if (deferredRenderer == nullptr) deferredRenderer = new DeferredRenderer();

	return deferredRenderer;
}

/*
 * find if this material is in the pool. If it is, and its version matches with info, just return.
 * Otherwise need to create a new one:
 *  - if material is not in the pool at all, just create it.
 *  - if it IS in the pool but version doesn't match, the old one is obsolete and need to be cleaned up
 *    and then create a new one from the up-to-date info
 */
Material* DeferredRenderer::getOrCreateMeshMaterial(const std::string &materialName)
{
	auto iter = materials.find(materialName);
	GltfMaterialInfo* info = GltfMaterialInfo::get(materialName);
	EXPECT(info != nullptr, true)

	if (iter != materials.end()) {
		auto pooled_mat = iter->second;
		if (pooled_mat->getVersion() == info->_version) {
			// up to date
			return pooled_mat;
		} else {
			// obsolete; delete and create a new one below
			delete pooled_mat;
		}
	}

	// create a new one
	auto newMaterial = new PbrGltfMaterial(*info);
	materials[newMaterial->name] = newMaterial;

	return newMaterial;
}

void DeferredRenderer::draw_config_ui() {
	ImGui::SliderFloat("", &ViewInfo.Exposure, -5, 5, "exposure comp: %.3f");
	ImGui::Combo(
		"tone mapping",
		&ViewInfo.ToneMappingOption,
		"Off\0Reinhard2\0ACES\0\0");
	ImGui::Checkbox("draw debug", &drawDebug);
}
