#include "DeferredRenderer.h"
#include "Render/Vulkan/RenderPassBuilder.h"
#include "Render/Texture.h"
#include "Render/Mesh.h"
#include "Scene/MeshObject.h"
#include "Scene/Probe.h"
#include "Render/Materials/GltfMaterial.h"
#include "Render/DebugDraw.h"
#include "Scene/Light.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Assets/ConfigAsset.hpp"
#include "Assets/EnvironmentMapAsset.h"
#include "Scene/SkyAtmosphere/SkyAtmosphere.h"
#include <imgui.h>
#include <algorithm>

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

			DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
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
		this->renderer = renderer;
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

	DeferredRenderer* renderer;

	friend class DeferredRenderer;
};

class DeferredLighting : public Material
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
			pipelineBuilder.fragPath = "spirv/deferred_lighting.frag.spv";
			pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			pipelineBuilder.pipelineState.useVertexInput = false;
			pipelineBuilder.pipelineState.useDepthStencil = false;
			pipelineBuilder.compatibleRenderPass = renderer->mainPass;
			pipelineBuilder.compatibleSubpass = DEFERRED_SUBPASS_LIGHTING;

			DescriptorSetLayout frameGlobalSetLayout = renderer->frameGlobalDescriptorSet.getLayout();
			pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			pipelineBuilder.useDescriptorSetLayout(DSET_INDEPENDENT, SkyAtmosphere::getInstance()->getDescriptorSet().getLayout());

			pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
		}

		return materialPipeline;
	}

	void usePipeline(VkCommandBuffer cmdbuf) override {
		MaterialPipeline materialPipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, materialPipeline.pipeline);
	}

private:

	explicit DeferredLighting(DeferredRenderer* renderer) : renderer(renderer) {
		name = "Deferred Lighting";
	}

	DeferredRenderer* renderer;

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

		ImageCreator GORMCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GORM");

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
		GORM = new Texture2D(GORMCreator);
		sceneColor = new Texture2D(sceneColorCreator);
		sceneDepth = new Texture2D(sceneDepthCreator);
		postProcessed = new Texture2D(postProcessedCreator);
	}

	{// main pass
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
		// GORM
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
			{GORM_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		VkAttachmentReference depthAttachmentReference = {
			SCENEDEPTH_ATTACHMENT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		passBuilder.subpasses.push_back({
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
			{GORM_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
		};
		std::vector<VkAttachmentReference> lightingColorAttachmentRefs = {
			{SCENECOLOR_ATTACHMENT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		passBuilder.subpasses.push_back({
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

		// translucency
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = lightingColorAttachmentRefs.data(),
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		// dependencies
		// TODO: any dependency w external needed?
		passBuilder.dependencies.push_back({
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
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_PROBES,
			.dstSubpass = DEFERRED_SUBPASS_TRANSLUCENCY,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		// build the renderpass
		mainPass = passBuilder.build(Vulkan::Instance);
	}

	{// post procesing pass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back({
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
		// post-processing
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
			GORM->imageView,
			sceneColor->imageView,
			sceneDepth->imageView
		};
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = mainPass, // the render pass it needs to be compatible with
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

		viewInfoUbo = VmaBuffer({&Vulkan::Instance->memoryAllocator,
								  sizeof(viewInfo),
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU,
								  "View info uniform buffer (deferred renderer)"});

		pointLightsBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
									   sizeof(pointLights),
									   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
									   VMA_MEMORY_USAGE_CPU_TO_GPU,
									   "Point lights buffer"});
		directionalLightsBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
											 sizeof(directionalLights),
											 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
											 VMA_MEMORY_USAGE_CPU_TO_GPU,
											 "Directional lights buffer"});

		DescriptorSetLayout frameGlobalSetLayout{};
		frameGlobalSetLayout.addBinding(0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalSetLayout.addBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(7, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		frameGlobalDescriptorSet = DescriptorSet(frameGlobalSetLayout);

		frameGlobalDescriptorSet.pointToBuffer(viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalDescriptorSet.pointToImageView(GPosition->imageView, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToImageView(GNormal->imageView, 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToImageView(GColor->imageView, 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToImageView(GORM->imageView, 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
		frameGlobalDescriptorSet.pointToBuffer(pointLightsBuffer, 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalDescriptorSet.pointToBuffer(directionalLightsBuffer, 6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		bool loadedEnvironmentMap = Config->lookup<int>("LoadEnvironmentMap");
		if (loadedEnvironmentMap) {
			auto envmap = Asset::find<EnvironmentMapAsset>(Config->lookup<std::string>("EnvironmentMap"));
			frameGlobalDescriptorSet.pointToImageView(envmap->texture2D->imageView, 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		} else {
			frameGlobalDescriptorSet.pointToImageView(Texture::get<Texture2D>("_black")->imageView, 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}
	}

	// misc
	viewInfo.Exposure = 3.0f;
	viewInfo.ToneMappingOption = 1;

	deferredLighting = new DeferredLighting(this);
	postProcessing = new PostProcessing(this, sceneColor, sceneDepth);

	{// debug draw stuff
#if 0 // example debug points
		if (!debugPoints) debugPoints = new DebugPoints(viewInfoUbo, postProcessPass, DEFERRED_SUBPASS_DEBUGDRAW);
		debugPoints->addPoint(glm::vec3(0, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->addPoint(glm::vec3(1, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->addPoint(glm::vec3(2, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->addPoint(glm::vec3(3, 1, 0), glm::u8vec4(255, 0, 0, 255));
		debugPoints->uploadVertexBuffer();
#endif

		std::vector<PointData> lines;
		if (!debugLines) debugLines = new DebugLines(frameGlobalDescriptorSet.getLayout(), postProcessPass, DEFERRED_SUBPASS_DEBUGDRAW);
		// x axis
		debugLines->addSegment(
			PointData(glm::vec3(0, 0, 0), glm::u8vec4(255, 0, 0, 255)),
			PointData(glm::vec3(10, 0, 0), glm::u8vec4(255, 0, 0, 255)));
		// y axis
		debugLines->addSegment(
			PointData(glm::vec3(0, 0, 0), glm::u8vec4(0, 255, 0, 255)),
			PointData(glm::vec3(0, 10, 0), glm::u8vec4(0, 255, 0, 255)));
		// z axis
		debugLines->addSegment(
			PointData(glm::vec3(0, 0, 0), glm::u8vec4(0, 0, 255, 255)),
			PointData(glm::vec3(0, 0, 10), glm::u8vec4(0, 0, 255, 255)));

		//debugLines->addBox(glm::vec3(-0.5f), glm::vec3(0.5f), glm::u8vec4(255, 255, 255, 255));
		debugLines->uploadVertexBuffer();

	}
}

DeferredRenderer::~DeferredRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyFramebuffer(vk->device, framebuffer, nullptr);
	vkDestroyFramebuffer(vk->device, postProcessFramebuffer, nullptr);
	viewInfoUbo.release();

	pointLightsBuffer.release();
	directionalLightsBuffer.release();

	delete deferredLighting;
	delete postProcessing;

	std::vector<Texture2D*> images = {
		GPosition, GNormal, GColor, GORM, sceneColor, sceneDepth, postProcessed
	};
	for (auto image : images) delete image;

	delete debugPoints;
	delete debugLines;

	for (const auto& p : materials) delete p.second;
}

void DeferredRenderer::updateUniformBuffers()
{
	viewInfo.ViewMatrix = camera->world_to_object();
	viewInfo.ProjectionMatrix = camera->camera_to_clip();
	viewInfo.ProjectionMatrix[1][1] *= -1; // so it's not upside down

	viewInfo.CameraPosition = camera->world_position();
	viewInfo.ViewDir = camera->forward();

	viewInfo.AspectRatio = camera->aspect_ratio;
	viewInfo.HalfVFovRadians = camera->fov * 0.5f;

	int numPointLights = 0;
	int numDirectionalLights = 0;
	drawable->foreach_descendent_bfs([this, &numPointLights, &numDirectionalLights](SceneObject* child){
		if (child->enabled()) {
			if (auto L = dynamic_cast<PointLight*>(child))
			{
				if (numPointLights < MAX_LIGHTS_PER_PASS) {
					pointLights.Data[numPointLights].position = L->world_position();
					pointLights.Data[numPointLights].color = L->get_emission() / (4 * PI);
					numPointLights++;
				}
			}
			else if (auto L = dynamic_cast<DirectionalLight*>(child))
			{
				if (numDirectionalLights < MAX_LIGHTS_PER_PASS) {
					directionalLights.Data[numDirectionalLights].direction = L->get_light_direction();
					directionalLights.Data[numDirectionalLights].color = L->get_emission();
					numDirectionalLights++;
				}
			}
		}
	});
	viewInfo.NumPointLights = numPointLights;
	viewInfo.NumDirectionalLights = numDirectionalLights;

	// background option
	if (SkyAtmosphere::getInstance()->enabled()) {
		viewInfo.BackgroundOption = BG_SkyAtmosphere;
	} else if (Config->lookup<int>("LoadEnvironmentMap")) {
		viewInfo.BackgroundOption = BG_EnvironmentMap;
	} else {
		viewInfo.BackgroundOption = BG_None;
	}

	viewInfoUbo.writeData(&viewInfo);
	pointLightsBuffer.writeData(&pointLights, numPointLights * sizeof(PointLightInfo));
	directionalLightsBuffer.writeData(&directionalLights, numDirectionalLights * sizeof(DirectionalLightInfo));
}

void DeferredRenderer::render(VkCommandBuffer cmdbuf)
{
	// reset material instance counters
	for (auto it : materials) {
		it.second->resetInstanceCounter();
	}

	updateUniformBuffers();

	// here the layout is for just so it gets ANY compatible layout
	frameGlobalDescriptorSet.bind(
		cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS,DSET_FRAMEGLOBAL, deferredLighting->getPipeline().layout);

	// objects gathering and sorting
	std::vector<MeshObject*> opaqueMeshes;
	std::vector<MeshObject*> translucentMeshes;
	std::vector<Probe*> probes;
	SkyAtmosphere* sky = nullptr;
	{
		// gather
		drawable->foreach_descendent_bfs([&](SceneObject* child) {
			// meshes
			if (auto mo = dynamic_cast<MeshObject*>(child)) {
				if (auto mat = dynamic_cast<GltfMaterial*>(getOrCreateMeshMaterial(mo->mesh->materialName))) {
					if (mat->isOpaque()) { // opaque
						opaqueMeshes.push_back(mo);
					} else { // translucent
						translucentMeshes.push_back(mo);
					}
				}
			} else if (auto probe = dynamic_cast<Probe*>(child)) {
				// probes
				probes.push_back(probe);
			} else {
				// sky
				auto castSky = dynamic_cast<SkyAtmosphere*>(child);
				if (castSky) sky = castSky;
			}
		}, [](SceneObject *obj){ return obj->enabled(); });

		// opaque objects sorting
		auto materialSortFn = [this](MeshObject* a, MeshObject* b) {
			auto aMaterial = dynamic_cast<GltfMaterial*>(getOrCreateMeshMaterial(a->mesh->materialName));
			auto bMaterial = dynamic_cast<GltfMaterial*>(getOrCreateMeshMaterial(b->mesh->materialName));
			auto aPipeline = aMaterial->getPipeline();
			auto bPipeline = bMaterial->getPipeline();
			if (aPipeline != bPipeline) { // different pipeline -> sort by pipeline
				return aPipeline < bPipeline;
			} else { // same pipeline -> compare material name
				return aMaterial->name < bMaterial->name;
			}
		};
		std::sort(opaqueMeshes.begin(), opaqueMeshes.end(), materialSortFn);

		// translucent objects sorting
		auto distToCameraSortFn = [this](MeshObject* a, MeshObject* b) {
			auto distToCamA = glm::dot(a->world_position() - viewInfo.CameraPosition, viewInfo.ViewDir);
			auto distToCamB = glm::dot(b->world_position() - viewInfo.CameraPosition, viewInfo.ViewDir);
			return distToCamA >= distToCamB;
		};
		std::sort(translucentMeshes.begin(), translucentMeshes.end(), distToCameraSortFn);
	}

	// TODO: find a better place to put this
	if (sky) {
		sky->updateAndComposite();
	}

	VkClearValue clearColor = {0, 0, 0, 0};
	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.f;
	VkClearValue clearValues[] = { clearColor, clearColor, clearColor, clearColor, clearColor, clearDepth };
	VkRect2D renderArea = { .offset = {0, 0}, .extent = renderExtent };
	VkRenderPassBeginInfo passInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = mainPass,
		.framebuffer = framebuffer,
		.renderArea = renderArea,
		.clearValueCount = 6,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		SCOPED_DRAW_EVENT(cmdbuf, "Opaque base pass")
		// deferred base pass: draw the meshes with materials
		Material* last_material = nullptr;
		MaterialPipeline last_pipeline = {};
		for (auto mo : opaqueMeshes)
		{
			auto mat = getOrCreateMeshMaterial(mo->mesh->materialName);//mo->get_material();
			auto pipeline = mat->getPipeline();

			// pipeline changed: re-bind; re-set frame globals if necessary
			if (pipeline != last_pipeline) {
				mat->usePipeline(cmdbuf);
				last_pipeline = pipeline;
			}

			// material changed
			if (mat != last_material) {
				last_material = mat;
			}

			mat->setParameters(cmdbuf, mo);
			mo->draw(cmdbuf);
		}

	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Opaque lighting pass")
		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);

		deferredLighting->usePipeline(cmdbuf);
		auto pipelineLayout = deferredLighting->getPipeline().layout;
		SkyAtmosphere::getInstance()->getDescriptorSet().bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_INDEPENDENT, pipelineLayout);
		vk::drawFullscreenTriangle(cmdbuf);

	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "EnvMap visualization")
		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
		bool firstInstance = true;
		auto mat = Probe::get_material();
		for (auto probe : probes) // TODO: material (pipeline) sorting, etc.
		{
			if (firstInstance) {
				mat->resetInstanceCounter();
				mat->usePipeline(cmdbuf);
			}
			mat->setParameters(cmdbuf, probe);
			probe->draw(cmdbuf);
			firstInstance = false;
		}
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Translucency")
		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
		Material* last_material = nullptr;
		MaterialPipeline last_pipeline = {};
		for (auto mo : translucentMeshes) {
			auto mat = getOrCreateMeshMaterial(mo->mesh->materialName);//mo->get_material();
			auto pipeline = mat->getPipeline();

			// pipeline changed: re-bind; re-set frame globals if necessary
			if (pipeline != last_pipeline) {
				mat->usePipeline(cmdbuf);
				last_pipeline = pipeline;
			}

			// material changed
			if (mat != last_material) {
				last_material = mat;
			}

			mat->setParameters(cmdbuf, mo);
			mo->draw(cmdbuf);
		}
	}
	vkCmdEndRenderPass(cmdbuf);

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Post Processing & Present")
		VkRenderPassBeginInfo passInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = postProcessPass,
			.framebuffer = postProcessFramebuffer,
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
			pooled_mat->markPipelineDirty();
			delete pooled_mat;
		}
	}

	// create a new one
	GltfMaterial* newMaterial;
	if (info->blendMode == BM_OpaqueOrClip) {
		newMaterial = new PbrGltfMaterial(*info);
	} else {
		newMaterial = new PbrTranslucentGltfMaterial(*info);
	}
	materials[newMaterial->name] = newMaterial;

	return newMaterial;
}

void DeferredRenderer::draw_config_ui() {
	ImGui::SliderFloat("", &viewInfo.Exposure, -25, 25, "exposure comp: %.3f");
	ImGui::Combo(
		"tone mapping",
		&viewInfo.ToneMappingOption,
		"Off\0Reinhard2\0ACES\0\0");
	ImGui::Checkbox("draw debug", &drawDebug);
}
