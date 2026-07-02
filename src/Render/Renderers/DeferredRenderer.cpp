#include "DeferredRenderer.h"
#include "Render/Vulkan/RenderPassBuilder.h"
#include "Render/Vulkan/Pipeline.h"
#include "Render/BindlessResources.h"
#include "Render/Texture.h"
#include "Render/Mesh.h"
#include "Scene/MeshObject.h"
#include "Scene/Probe.h"
#include "Render/DebugDraw.h"
#include "Scene/Light.hpp"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Assets/ConfigAsset.hpp"
#include "Assets/EnvironmentMapAsset.h"
#include "../../Scene/SkyAtmosphere.h"
#include <imgui.h>
#include <algorithm>
#include <memory>
#include <unordered_map>

class PostProcessing
{
public:
	const GraphicsPipeline& getPipeline()
	{
		if (!graphicsPipeline.valid()) {
			auto vk = Vulkan::Instance;
			auto& b = graphicsPipeline.builder;
			b.vertDef = "shaders/fullscreen_triangle.vert";
			b.fragDef = "shaders/post_processing.frag";
			b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			b.pipelineState.useVertexInput = false;
			b.pipelineState.useDepthStencil = false;
			b.compatibleRenderPass = postProcessPass;
			b.compatibleSubpass = DEFERRED_SUBPASS_POSTPROCESSING;

			DescriptorSetLayout frameGlobalSetLayout = renderer->getFrameGlobalLayout();
			DescriptorSetLayout postProcessSetLayout = postProcessSet.getLayout();
			b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			b.useDescriptorSetLayout(DSET_INDEPENDENT, postProcessSetLayout);

			graphicsPipeline.build("Post Processing");

		}
		return graphicsPipeline;
	}

private:

	explicit PostProcessing(DeferredRenderer* renderer, Texture2D* sceneColor, Texture2D* sceneDepth)
	{
		this->renderer = renderer;
		postProcessPass = renderer->postProcessPass;

		// set layouts and allocation
		DescriptorSetLayout frameGlobalSetLayout = renderer->getFrameGlobalLayout();
		DescriptorSetLayout postProcessSetLayout{};
		postProcessSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		postProcessSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		postProcessSet = DescriptorSet(postProcessSetLayout);

		// assign values
		postProcessSet.pointToImageView(sceneColor->imageView, 0);
		postProcessSet.pointToImageView(sceneDepth->imageView, 1);
	}

	VkRenderPass postProcessPass;
	DescriptorSet postProcessSet;
	GraphicsPipeline graphicsPipeline;

	DeferredRenderer* renderer;

	friend class DeferredRenderer;
};

class DeferredLighting
{
public:

	const GraphicsPipeline& getPipeline()
	{
		if (!graphicsPipeline.valid()) {
			auto vk = Vulkan::Instance;
			auto& b = graphicsPipeline.builder;
			b.vertDef = "shaders/fullscreen_triangle.vert";
			b.fragDef = "shaders/deferred_lighting.frag";
			b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			b.pipelineState.useVertexInput = false;
			b.pipelineState.useDepthStencil = false;
			b.compatibleRenderPass = renderer->lightingPass;
			b.compatibleSubpass = DEFERRED_SUBPASS_LIGHTING;

			DescriptorSetLayout frameGlobalSetLayout = renderer->getFrameGlobalLayout();
			b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			b.useDescriptorSetLayout(DSET_INDEPENDENT, renderer->getSkyDescriptorSet().getLayout());

			graphicsPipeline.build("Deferred Lighting");
		}
		return graphicsPipeline;
	}

private:

	explicit DeferredLighting(DeferredRenderer* renderer) : renderer(renderer) {}

	DeferredRenderer* renderer;
	GraphicsPipeline graphicsPipeline;

	friend class DeferredRenderer;
};

namespace {

enum class DeferredGltfPass {
	OpaqueGBuffer,
	TranslucentLighting,
};

struct DeferredGltfPipelineKey {
	DeferredGltfPass pass;
	bool doubleSided = false;

	explicit DeferredGltfPipelineKey(const MeshSurface& surface)
		: pass(surface.isOpaque() ? DeferredGltfPass::OpaqueGBuffer : DeferredGltfPass::TranslucentLighting)
		, doubleSided(surface.doubleSided)
	{}

	bool operator==(const DeferredGltfPipelineKey&) const = default;
	bool operator<(const DeferredGltfPipelineKey& other) const {
		return hash() < other.hash();
	}

	size_t hash() const noexcept {
		return (static_cast<size_t>(pass) << 1) | static_cast<size_t>(doubleSided);
	}

	struct Hash {
		size_t operator()(const DeferredGltfPipelineKey& key) const noexcept {
			return key.hash();
		}
	};
};

class DeferredGltfPipelineCache {
public:
	static const GraphicsPipeline& get(const DeferredGltfPipelineKey& key) {
		auto it = pipelines.find(key);
		if (it != pipelines.end()) return *it->second;

		auto pipeline = std::make_unique<GraphicsPipeline>();
		auto vk = Vulkan::Instance;
		auto renderer = DeferredRenderer::get();
		auto& b = pipeline->builder;

		b.vertDef = "shaders/geometry.vert";
		b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		b.pipelineState.rasterizationInfo.cullMode =
			key.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;

		if (key.pass == DeferredGltfPass::OpaqueGBuffer) {
			b.fragDef = "shaders/geometry.frag";
			b.compatibleRenderPass = renderer->basePass;
			b.compatibleSubpass = DEFERRED_SUBPASS_GEOMETRY;

			auto singleBlend = b.pipelineState.colorBlendAttachmentInfo;
			b.pipelineState.colorBlendAttachments = {singleBlend, singleBlend, singleBlend, singleBlend};
		}
		else {
			b.fragDef = "shaders/translucency_lit.frag";
			b.compatibleRenderPass = renderer->lightingPass;
			b.compatibleSubpass = DEFERRED_SUBPASS_TRANSLUCENCY;
			b.pipelineState.depthStencilInfo.depthWriteEnable = VK_FALSE;
			b.pipelineState.colorBlendAttachments = {{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
				.colorWriteMask = 15
			}};
		}

		b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, renderer->getFrameGlobalLayout());
		b.useDescriptorSetLayout(DSET_BINDLESS, BindlessResources::Instance->layout());
		b.usePushConstantRange({VK_SHADER_STAGE_VERTEX_BIT, GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE});
		b.usePushConstantRange({VK_SHADER_STAGE_FRAGMENT_BIT, GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE});

		const std::string debugName = key.pass == DeferredGltfPass::OpaqueGBuffer
			? (key.doubleSided ? "PbrGltf DoubleSided" : "PbrGltf")
			: (key.doubleSided ? "PbrTranslucent DoubleSided" : "PbrTranslucent");
		pipeline->build(debugName);

		auto [insertedIt, _] = pipelines.emplace(key, std::move(pipeline));
		return *insertedIt->second;
	}

	static void release() {
		pipelines.clear();
	}

private:
	inline static std::unordered_map<DeferredGltfPipelineKey, std::unique_ptr<GraphicsPipeline>, DeferredGltfPipelineKey::Hash> pipelines;
};

}

DeferredRenderer::DeferredRenderer()
{
	renderExtent = Vulkan::Instance->swapChainExtent;

	// init the components
	sceneTlas.init("Deferred");

	{// images
		ImageCreator GNormalCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GNormal");

		ImageCreator GColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GColor");

		ImageCreator GORMCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GORM");

		ImageCreator GMotionCreator(
			VK_FORMAT_R16G16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"GMotion");

		ImageCreator sceneColorCreator(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			{renderExtent.width, renderExtent.height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
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

		GNormal = new Texture2D(GNormalCreator);
		GColor = new Texture2D(GColorCreator);
		GORM = new Texture2D(GORMCreator);
		GMotion = new Texture2D(GMotionCreator);
		sceneColor = new Texture2D(sceneColorCreator);
		sceneDepth = new Texture2D(sceneDepthCreator);
		postProcessed = new Texture2D(postProcessedCreator);
	}

	{// base pass
		RenderPassBuilder passBuilder;

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
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
		// GMotion
		passBuilder.colorAttachments.push_back(
			{
				.format = VK_FORMAT_R16G16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// base pass
		std::vector<VkAttachmentReference> basePassColorAttachmentRefs = {
			{GNORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{GCOLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{GORM_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{GMOTION_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		constexpr uint32_t basePassDepthAttachment = 4;
		VkAttachmentReference depthAttachmentReference = {
			basePassDepthAttachment,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = static_cast<uint32_t>(basePassColorAttachmentRefs.size()),
			.pColorAttachments = basePassColorAttachmentRefs.data(), // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		// base pass dependencies
		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_GEOMETRY,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		basePass = passBuilder.build(Vulkan::Instance);
	}

	{// lighting and translucency pass
		RenderPassBuilder passBuilder;

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
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		std::vector<VkAttachmentReference> lightingColorAttachmentRefs = {
			{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		VkAttachmentReference depthAttachmentReference = {
			1,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		// lighting subpass
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			// output attachments
			.colorAttachmentCount = static_cast<uint32_t>(lightingColorAttachmentRefs.size()),
			.pColorAttachments = lightingColorAttachmentRefs.data(),
			.pDepthStencilAttachment = nullptr
		});

		// translucency subpass
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = lightingColorAttachmentRefs.data(),
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		// dependencies
		passBuilder.dependencies.push_back({
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = DEFERRED_SUBPASS_LIGHTING,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		passBuilder.dependencies.push_back({
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = DEFERRED_SUBPASS_TRANSLUCENCY,
			.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_LIGHTING,
			.dstSubpass = DEFERRED_SUBPASS_TRANSLUCENCY,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_TRANSLUCENCY,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		lightingPass = passBuilder.build(Vulkan::Instance);
	}

	{// envmap visualization pass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back({
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		passBuilder.dependencies.push_back({
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		envmapVisualizationPass = passBuilder.build(Vulkan::Instance);
	}

	{// post processing pass
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

		VkAttachmentReference colorAttachmentRef = {
			0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		// post-processing
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = nullptr
		});

		passBuilder.dependencies.push_back({
			.srcSubpass = DEFERRED_SUBPASS_POSTPROCESSING,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		postProcessPass = passBuilder.build(Vulkan::Instance);
	}

	{// debug draw pass
		RenderPassBuilder passBuilder;
		passBuilder.colorAttachments.push_back({
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
		passBuilder.subpasses.push_back({
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = &depthAttachmentReference
		});

		passBuilder.dependencies.push_back({
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});
		debugDrawPass = passBuilder.build(Vulkan::Instance);
	}

	{// framebuffer for base pass
		VkImageView attachments[] = {
			GNormal->imageView,
			GColor->imageView,
			GORM->imageView,
			GMotion->imageView,
			sceneDepth->imageView
		};
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = basePass,
			.attachmentCount = 5,
			.pAttachments = attachments,
			.width = renderExtent.width,
			.height = renderExtent.height,
			.layers = 1
		};
		EXPECT(vkCreateFramebuffer(
			Vulkan::Instance->device,
			&framebufferInfo,
			nullptr,
			&baseFramebuffer), VK_SUCCESS)
	}

	{// framebuffer for lighting and translucency pass
		VkImageView attachments[] = {
			sceneColor->imageView,
			sceneDepth->imageView
		};
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = lightingPass, // the render pass it needs to be compatible with
			.attachmentCount = 2,
			.pAttachments = attachments, // a pointer to an array of VkImageView handles, each of which will be used as the corresponding attachment in a render pass instance.
			.width = renderExtent.width,
			.height = renderExtent.height,
			.layers = 1
		};
		EXPECT(vkCreateFramebuffer(
			Vulkan::Instance->device,
			&framebufferInfo,
			nullptr,
			&lightingFramebuffer), VK_SUCCESS)
	}

	{// framebuffer for envmap visualization
		VkImageView attachments[2] = { sceneColor->imageView, sceneDepth->imageView };
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = envmapVisualizationPass,
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
			&envmapVisualizationFramebuffer), VK_SUCCESS)
	}

	{// also framebuffer for postprocessing
		VkImageView attachments[1] = { postProcessed->imageView };
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = postProcessPass,
			.attachmentCount = 1,
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

	{// framebuffer for debug draw
		VkImageView attachments[2] = { postProcessed->imageView, sceneDepth->imageView };
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = debugDrawPass,
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
			&debugDrawFramebuffer), VK_SUCCESS)
	}

	{// frame-global descriptor set (per-frame ring buffer)
		DescriptorSetLayout frameGlobalSetLayout{};
		frameGlobalSetLayout.addBinding(0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		frameGlobalSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		frameGlobalSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		frameGlobalSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		frameGlobalSetLayout.addBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		frameGlobalSetLayout.addBinding(7, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		frameGlobalSetLayout.addBinding(8, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		frameGlobalSetLayout.addBinding(9, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		bool loadedEnvironmentMap = Config->lookup<int>("LoadEnvironmentMap");
		const Texture2D* envmap = loadedEnvironmentMap
			? Asset::find<EnvironmentMapAsset>(Config->lookup<std::string>("EnvironmentMap"))->texture2D
			: Texture2D::black();

		auto gbufferSamplerInfo = SamplerCache::defaultInfo();
		gbufferSamplerInfo.magFilter = VK_FILTER_NEAREST;
		gbufferSamplerInfo.minFilter = VK_FILTER_NEAREST;
		gbufferSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		gbufferSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		gbufferSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		gbufferSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		gbufferSamplerInfo.maxLod = 0;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto& fd = gpuFrameData[i];

			fd.viewInfoUbo = VmaBuffer({&Vulkan::Instance->memoryAllocator,
								   sizeof(glm::ViewInfo),
								   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								   VMA_MEMORY_USAGE_CPU_TO_GPU,
								   "View info uniform buffer (deferred renderer)"});
			fd.pointLightsBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
										 sizeof(pointLights),
										 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
										 VMA_MEMORY_USAGE_CPU_TO_GPU,
										 "Point lights buffer"});
			fd.directionalLightsBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
												 sizeof(directionalLights),
												 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
												 VMA_MEMORY_USAGE_CPU_TO_GPU,
												 "Directional lights buffer"});
		}

		{// GI: depends on viewInfoUbos, but need to initialize before slot 9
			GI::InitInfo giInitInfo{};
			giInitInfo.sceneDepth = sceneDepth;
			giInitInfo.GNormal = GNormal;
			giInitInfo.GMotion = GMotion;
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				giInitInfo.viewInfoUbos[i] = &gpuFrameData[i].viewInfoUbo;
				giInitInfo.sceneInstanceRecordBuffers[i] = &sceneTlas.getSceneInstanceRecordBuffer(i);
				giInitInfo.pointLightBuffers[i] = &gpuFrameData[i].pointLightsBuffer;
				giInitInfo.directionalLightBuffers[i] = &gpuFrameData[i].directionalLightsBuffer;
			}
			giInitInfo.tlas = sceneTlas.get();
			giInitInfo.environmentMap = envmap;
			gi.init(giInitInfo);
		}

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto& fd = gpuFrameData[i];
			fd.frameGlobalDescriptorSet = DescriptorSet(frameGlobalSetLayout);
			fd.frameGlobalDescriptorSet.pointToBuffer(fd.viewInfoUbo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			fd.frameGlobalDescriptorSet.pointToImageView(sceneDepth->imageView, 1, &gbufferSamplerInfo);
			fd.frameGlobalDescriptorSet.pointToImageView(GNormal->imageView, 2, &gbufferSamplerInfo);
			fd.frameGlobalDescriptorSet.pointToImageView(GColor->imageView, 3, &gbufferSamplerInfo);
			fd.frameGlobalDescriptorSet.pointToImageView(GORM->imageView, 4, &gbufferSamplerInfo);
			fd.frameGlobalDescriptorSet.pointToBuffer(fd.pointLightsBuffer, 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			fd.frameGlobalDescriptorSet.pointToBuffer(fd.directionalLightsBuffer, 6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			fd.frameGlobalDescriptorSet.pointToImageView(envmap->imageView, 7);
			fd.frameGlobalDescriptorSet.pointToAccelerationStructure(sceneTlas.get(), 8);
			fd.frameGlobalDescriptorSet.pointToImageView(gi.getIndirectLighting()->imageView, 9, &gbufferSamplerInfo);
		}
	}

	skyAtmosphereRender.init();

	// misc
	cfgExposure = 3.0f;
	cfgToneMappingOption = 1;

	deferredLighting = new DeferredLighting(this);
	postProcessing = new PostProcessing(this, sceneColor, sceneDepth);

	{// debug draw stuff (per-frame ring buffer)
		auto layout = getFrameGlobalLayout();
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto& fd = gpuFrameData[i];
#if 0 // example debug points
			fd.debugPoints = new DebugPoints(layout, debugDrawPass, 0);
			fd.debugPoints->addPoint(glm::vec3(0, 1, 0), glm::u8vec4(255, 0, 0, 255));
			fd.debugPoints->addPoint(glm::vec3(1, 1, 0), glm::u8vec4(255, 0, 0, 255));
			fd.debugPoints->addPoint(glm::vec3(2, 1, 0), glm::u8vec4(255, 0, 0, 255));
			fd.debugPoints->addPoint(glm::vec3(3, 1, 0), glm::u8vec4(255, 0, 0, 255));
			fd.debugPoints->uploadVertexBuffer();
#endif
			fd.debugLines = new DebugLines(layout, debugDrawPass, 0);
			// x axis
			fd.debugLines->addSegment(
				PointData(glm::vec3(0, 0, 0), glm::u8vec4(255, 0, 0, 255)),
				PointData(glm::vec3(10, 0, 0), glm::u8vec4(255, 0, 0, 255)));
			// y axis
			fd.debugLines->addSegment(
				PointData(glm::vec3(0, 0, 0), glm::u8vec4(0, 255, 0, 255)),
				PointData(glm::vec3(0, 10, 0), glm::u8vec4(0, 255, 0, 255)));
			// z axis
			fd.debugLines->addSegment(
				PointData(glm::vec3(0, 0, 0), glm::u8vec4(0, 0, 255, 255)),
				PointData(glm::vec3(0, 0, 10), glm::u8vec4(0, 0, 255, 255)));
			//fd.debugLines->addBox(glm::vec3(-0.5f), glm::vec3(0.5f), glm::u8vec4(255, 255, 255, 255));
			fd.debugLines->uploadVertexBuffer();
		}
	}
}

DeferredRenderer::~DeferredRenderer()
{
	auto vk = Vulkan::Instance;
	vkDestroyFramebuffer(vk->device, baseFramebuffer, nullptr);
	vkDestroyFramebuffer(vk->device, lightingFramebuffer, nullptr);
	vkDestroyFramebuffer(vk->device, envmapVisualizationFramebuffer, nullptr);
	vkDestroyFramebuffer(vk->device, postProcessFramebuffer, nullptr);
	vkDestroyFramebuffer(vk->device, debugDrawFramebuffer, nullptr);
	gi.release();
	skyAtmosphereRender.release();
	sceneTlas.release();
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		auto& fd = gpuFrameData[i];
		fd.viewInfoUbo.release();
		fd.pointLightsBuffer.release();
		fd.directionalLightsBuffer.release();
		delete fd.debugPoints;
		delete fd.debugLines;
	}

	delete deferredLighting;
	delete postProcessing;

	DeferredGltfPipelineCache::release();

	std::vector<Texture2D*> images = {
		GNormal, GColor, GORM, GMotion, sceneColor, sceneDepth, postProcessed
	};
	for (auto image : images) delete image;
}

void DeferredRenderer::render(VkCommandBuffer cmdbuf)
{
	glm::ViewInfo viewInfo = getCameraViewInfo();
	viewInfo.RenderSize = glm::vec2(renderExtent.width, renderExtent.height);
	{
        int numPointLights = 0;
        int numDirectionalLights = 0;
        drawable->foreach_descendent_bfs([this, &numPointLights, &numDirectionalLights](SceneObject* child){
            // convert whatever unit (cd, lx, nt) to watt:
            // from blender, PBR_WATTS_TO_LUMENS = 683 // so lumen to watt is 1.0f/683
            // the last div by 2*PI is converting irradiance to radiance (???)
            // todo: rename the "getMultipliedColor" interface altogether
            if (child->enabled()) {
                if (auto L = dynamic_cast<PointLight*>(child))
                {
                    if (numPointLights < MAX_LIGHTS_PER_PASS) {
                        pointLights.Data[numPointLights].position = L->world_position();
                        pointLights.Data[numPointLights].color = L->getLuminousIntensityCd();
                        numPointLights++;
                    }
                }
                else if (auto L = dynamic_cast<DirectionalLight*>(child))
                {
                    if (numDirectionalLights < MAX_LIGHTS_PER_PASS) {
                        directionalLights.Data[numDirectionalLights].direction = L->getLightDirection();
                        directionalLights.Data[numDirectionalLights].color = L->getIrradianceLx();
                        numDirectionalLights++;
                    }
                }
            }
        });
        viewInfo.NumPointLights = numPointLights;
        viewInfo.NumDirectionalLights = numDirectionalLights;

        viewInfo.Exposure = cfgExposure;
        viewInfo.ToneMappingOption = cfgToneMappingOption;

        // background option
        if (SkyAtmosphere::getInstance()->enabled()) {
            viewInfo.BackgroundOption = BG_SkyAtmosphere;
        } else if (Config->lookup<int>("LoadEnvironmentMap")) {
            viewInfo.BackgroundOption = BG_EnvironmentMap;
        } else {
            viewInfo.BackgroundOption = BG_None;
        }

        auto& fd = gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()];
        fd.viewInfoUbo.writeData(&viewInfo, sizeof(viewInfo));
        fd.pointLightsBuffer.writeData(&pointLights, numPointLights * sizeof(glm::PointLightInfo));
        fd.directionalLightsBuffer.writeData(&directionalLights, numDirectionalLights * sizeof(glm::DirectionalLightInfo));
	}

	auto& fd = gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()];
	auto bindFrameGlobal = [&](VkPipelineLayout layout) {
		fd.frameGlobalDescriptorSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_FRAMEGLOBAL, layout);
	};

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
				if (mo->mesh.surface.isOpaque()) {
					opaqueMeshes.push_back(mo);
				} else {
					translucentMeshes.push_back(mo);
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
		auto materialSortFn = [](MeshObject* a, MeshObject* b) {
			const auto& aSurface = a->mesh.surface;
			const auto& bSurface = b->mesh.surface;
			const auto aKey = DeferredGltfPipelineKey(aSurface);
			const auto bKey = DeferredGltfPipelineKey(bSurface);
			if (aKey != bKey) return aKey < bKey;
			if (aSurface.bindlessMaterialIndex != bSurface.bindlessMaterialIndex) {
				return aSurface.bindlessMaterialIndex < bSurface.bindlessMaterialIndex;
			}
			return aSurface.materialName < bSurface.materialName;
		};
		std::sort(opaqueMeshes.begin(), opaqueMeshes.end(), materialSortFn);

		// translucent objects sorting
		auto distToCameraSortFn = [this, &viewInfo](MeshObject* a, MeshObject* b) {
			auto distToCamA = glm::dot(a->world_position() - viewInfo.CameraPosition, viewInfo.ViewDir);
			auto distToCamB = glm::dot(b->world_position() - viewInfo.CameraPosition, viewInfo.ViewDir);
			return distToCamA >= distToCamB;
		};
		std::sort(translucentMeshes.begin(), translucentMeshes.end(), distToCameraSortFn);
	}

	skyAtmosphereRender.update_luts(cmdbuf, sky);

	{
		SCOPED_DRAW_EVENT(cmdbuf, "rebuild deferred scene TLAS")
		sceneTlas.build_from_meshes(
			cmdbuf,
			Vulkan::Instance->getCurrentFrameIndex(),
			opaqueMeshes,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}

	auto renderMeshes = [this, &cmdbuf, &bindFrameGlobal](const std::vector<MeshObject*>& meshes) {
		const GraphicsPipeline* last_pipeline = nullptr;
		for (auto mo : meshes)
		{
			auto key = DeferredGltfPipelineKey(mo->mesh.surface);
			auto& pipeline = DeferredGltfPipelineCache::get(key);

			// pipeline changed: re-bind pipeline; re-set frame globals if necessary
			if (!last_pipeline || pipeline != *last_pipeline) {
				vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
				if (!last_pipeline || pipeline.layout != last_pipeline->layout) {
					bindFrameGlobal(pipeline.layout);
					BindlessResources::Instance->descriptorSet().bind(
						cmdbuf,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						DSET_BINDLESS,
						pipeline.layout);
				}
				last_pipeline = &pipeline;
			}

			{ // each object has its push constants
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
	};

	VkClearValue clearColor = {0, 0, 0, 0};
	VkClearValue clearDepth;
	clearDepth.depthStencil.depth = 1.f;
	VkClearValue baseClearValues[] = { clearColor, clearColor, clearColor, clearColor, clearDepth };
	VkRect2D renderArea = { .offset = {0, 0}, .extent = renderExtent };
	VkRenderPassBeginInfo basePassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = basePass,
		.framebuffer = baseFramebuffer,
		.renderArea = renderArea,
		.clearValueCount = 5,
		.pClearValues = baseClearValues
	};
	vkCmdBeginRenderPass(cmdbuf, &basePassInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		SCOPED_DRAW_EVENT(cmdbuf, "Opaque base pass")
		// deferred base pass: draw the meshes with materials
		renderMeshes(opaqueMeshes);
	}
	vkCmdEndRenderPass(cmdbuf);

	gi.render(cmdbuf, Vulkan::Instance->getCurrentFrameIndex(), getSkyDescriptorSet());

	VkClearValue lightingClearValues[] = { clearColor, clearDepth };
	VkRenderPassBeginInfo lightingPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = lightingPass,
		.framebuffer = lightingFramebuffer,
		.renderArea = renderArea,
		.clearValueCount = 2,
		.pClearValues = lightingClearValues
	};
	vkCmdBeginRenderPass(cmdbuf, &lightingPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		SCOPED_DRAW_EVENT(cmdbuf, "Opaque lighting pass")
		auto& deferredLightingPipeline = deferredLighting->getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingPipeline.pipeline);
		bindFrameGlobal(deferredLightingPipeline.layout);
		getSkyDescriptorSet().bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_INDEPENDENT, deferredLightingPipeline.layout);
		vk::drawFullscreenTriangle(cmdbuf);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Translucency")
		vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
		renderMeshes(translucentMeshes);
	}
	vkCmdEndRenderPass(cmdbuf);

	if (drawEnvmapVisualization) {
		SCOPED_DRAW_EVENT(cmdbuf, "EnvMap visualization")
		VkRenderPassBeginInfo passInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = envmapVisualizationPass,
			.framebuffer = envmapVisualizationFramebuffer,
			.renderArea = renderArea,
			.clearValueCount = 0,
			.pClearValues = nullptr
		};
		vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			bool firstInstance = true;
			for (auto probe : probes) // TODO: material (pipeline) sorting, etc.
			{
				if (firstInstance) {
					auto layout = Probe::bind_envmap_visualization_pipeline(cmdbuf);
					bindFrameGlobal(layout);
				}
				probe->set_envmap_visualization_draw_params(cmdbuf);
				probe->draw(cmdbuf);
				firstInstance = false;
			}
		}
		vkCmdEndRenderPass(cmdbuf);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Post Processing")
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
			auto& postProcessPipeline = postProcessing->getPipeline();
			bindFrameGlobal(postProcessPipeline.layout); // 0
			postProcessing->postProcessSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_INDEPENDENT, postProcessPipeline.layout);
			vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, postProcessPipeline.pipeline);
			vk::drawFullscreenTriangle(cmdbuf);
		}
		vkCmdEndRenderPass(cmdbuf);
	}

	if (drawDebug) {
		SCOPED_DRAW_EVENT(cmdbuf, "Debug draw")
		VkRenderPassBeginInfo passInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = debugDrawPass,
			.framebuffer = debugDrawFramebuffer,
			.renderArea = renderArea,
			.clearValueCount = 0,
			.pClearValues = nullptr
		};

		vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
		if (fd.debugLines) {
			bindFrameGlobal(fd.debugLines->getPipelineLayout());
			fd.debugLines->bindAndDraw(cmdbuf);
		}
		if (fd.debugPoints) {
			bindFrameGlobal(fd.debugPoints->getPipelineLayout());
			fd.debugPoints->bindAndDraw(cmdbuf);
		}
		vkCmdEndRenderPass(cmdbuf);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Present")
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

DescriptorSetLayout DeferredRenderer::getFrameGlobalLayout()
{
	return gpuFrameData[0].frameGlobalDescriptorSet.getLayout();
}

DescriptorSet& DeferredRenderer::getSkyDescriptorSet()
{
	return skyAtmosphereRender.get_descriptor_set(SkyAtmosphere::getInstance());
}

void DeferredRenderer::draw_config_ui() {
	ImGui::SliderFloat("##exposure", &cfgExposure, -25, 25, "exposure comp: %.3f");
	ImGui::Combo(
		"tone mapping",
		&cfgToneMappingOption,
		"Off\0Reinhard2\0ACES\0\0");
	ImGui::Checkbox("draw debug", &drawDebug);
	ImGui::Checkbox("draw envmap visualization", &drawEnvmapVisualization);
}
