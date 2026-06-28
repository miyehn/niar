#include "GltfMaterial.h"
#include "Scene/MeshObject.h"
#include "Scene/SceneObject.hpp"
#include "Render/BindlessResources.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/Pipeline.h"
#include "Render/Renderers/DeferredRenderer.h"
#include "Render/Texture.h"

#include <tiny_gltf.h>
#include "Render/Renderers/SimpleRenderer.h"

void GltfMaterial::setPerDrawParameters(VkCommandBuffer cmdbuf, SceneObject *drawable)
{
	// per-object model matrix via push constants
	glm::mat4 modelMatrix = drawable->object_to_world();
	vkCmdPushConstants(cmdbuf, getPipeline().layout, VK_SHADER_STAGE_VERTEX_BIT,
		GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE, &modelMatrix);

	const auto* meshObject = dynamic_cast<const MeshObject*>(drawable);
	// todo [myn]: currently seems like drawable passed in here is always a MeshObject anyway.
	ASSERT(meshObject != nullptr)
	const uint32_t bindlessMaterialIndex = meshObject->mesh.bindlessMaterialIndex;
	ASSERT(bindlessMaterialIndex != INVALID_BINDLESS_INDEX)
#if TMP_BINDLESS_DEBUG
	BindlessResources::Instance->assertMaterialIndexOccupied(bindlessMaterialIndex);
#endif
	vkCmdPushConstants(cmdbuf, getPipeline().layout, VK_SHADER_STAGE_FRAGMENT_BIT,
		GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE, &bindlessMaterialIndex);
}

void GltfMaterial::bindMaterialDescriptors(VkCommandBuffer cmdbuf, VkPipelineLayout layout)
{
	dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, layout);
}

GltfMaterial::~GltfMaterial()
{
	materialParamsBuffer.release();
}

GltfMaterial::GltfMaterial(const GltfMaterialInfo &info)
{
	cachedMaterialInfo = info;

	this->name = info.name;
	LOG("loading material '%s'..", name.c_str())

	std::string bufferName = "Material renderingParams buffer (" + info.name + ")";
	materialParamsBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
									 sizeof(materialParams),
									 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
									 VMA_MEMORY_USAGE_CPU_TO_GPU,
									 bufferName});

	{// pipeline and layouts

		// set layouts and allocation
		DescriptorSetLayout dynamicSetLayout{};
		dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSetLayout.addBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSetLayout.addBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		dynamicSet = DescriptorSet(dynamicSetLayout); // this commits the bindings

		// assign actual values to them
		auto albedo = Texture::get<Texture2D>(info.albedoTexName);
		auto normal = Texture::get<Texture2D>(info.normalTexName);
		auto orm = Texture::get<Texture2D>(info.ormTexName);
		auto emissive = Texture::get<Texture2D>(info.emissiveTexName);

		materialParams.BaseColorFactor = info.BaseColorFactor;
		materialParams.OcclusionRoughnessMetallicNormalStrengths = info.OcclusionRoughnessMetallicNormalStrengths;
		materialParams.EmissiveFactorClipThreshold = glm::vec4(
			info.EmissiveFactor.r,
			info.EmissiveFactor.g,
			info.EmissiveFactor.b,
			info.clipThreshold);
		materialParams._pad0 = glm::vec4();
		materialParamsBuffer.writeData(&materialParams, sizeof(materialParams));

		dynamicSet.pointToBuffer(materialParamsBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		dynamicSet.pointToImageView(albedo->imageView, 1);
		dynamicSet.pointToImageView(normal->imageView, 2);
		dynamicSet.pointToImageView(orm->imageView, 3);
		dynamicSet.pointToImageView(emissive->imageView, 4);
	}
}

GraphicsPipeline PbrGltfMaterial::graphicsPipeline;
void PbrGltfMaterial::destroyPipeline() {
	graphicsPipeline.destroy();
}

const GraphicsPipeline& PbrGltfMaterial::getPipeline()
{
	if (!graphicsPipeline.valid()) {
		auto vk = Vulkan::Instance;
		auto& b = graphicsPipeline.builder;
		b.vertDef = "shaders/geometry.vert";
		b.fragDef = "shaders/geometry.frag";
		b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		b.pipelineState.rasterizationInfo.cullMode =
			cachedMaterialInfo.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		b.compatibleRenderPass = DeferredRenderer::get()->basePass;

		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->getFrameGlobalLayout();
		DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
		b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		b.useDescriptorSetLayout(DSET_BINDLESS, BindlessResources::Instance->layout());
		b.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);
		b.usePushConstantRange({VK_SHADER_STAGE_VERTEX_BIT, GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE});
		b.usePushConstantRange({VK_SHADER_STAGE_FRAGMENT_BIT, GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE});

		// 3 color outputs; store by value so the pointer stays valid for deferred rebuilds
		auto singleBlend = b.pipelineState.colorBlendAttachmentInfo;
		b.pipelineState.colorBlendAttachments = {singleBlend, singleBlend, singleBlend};

		graphicsPipeline.build("PbrGltf");
	}
	return graphicsPipeline;
}

GraphicsPipeline PbrTranslucentGltfMaterial::graphicsPipeline;
void PbrTranslucentGltfMaterial::destroyPipeline() { graphicsPipeline.destroy(); }

const GraphicsPipeline& PbrTranslucentGltfMaterial::getPipeline()
{
	if (!graphicsPipeline.valid()) {
		auto vk = Vulkan::Instance;
		auto& b = graphicsPipeline.builder;
		b.vertDef = "shaders/geometry.vert";
		b.fragDef = "shaders/translucency_lit.frag";
		b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		b.pipelineState.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		b.compatibleRenderPass = DeferredRenderer::get()->lightingPass;
		b.compatibleSubpass = DEFERRED_SUBPASS_TRANSLUCENCY;

		DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->getFrameGlobalLayout();
		DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
		b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		b.useDescriptorSetLayout(DSET_BINDLESS, BindlessResources::Instance->layout());
		b.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);
		b.usePushConstantRange({VK_SHADER_STAGE_VERTEX_BIT, GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE});
		b.usePushConstantRange({VK_SHADER_STAGE_FRAGMENT_BIT, GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE});

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

		graphicsPipeline.build("PbrTranslucent");
	}
	return graphicsPipeline;
}

GraphicsPipeline SimpleGltfMaterial::graphicsPipeline;
void SimpleGltfMaterial::destroyPipeline() { graphicsPipeline.destroy(); }

const GraphicsPipeline& SimpleGltfMaterial::getPipeline()
{
	if (!graphicsPipeline.valid()) {
		auto vk = Vulkan::Instance;

		auto& b = graphicsPipeline.builder;
		b.vertDef = "shaders/geometry.vert";
		b.fragDef = "shaders/simple_gltf.frag";
		b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		b.pipelineState.rasterizationInfo.cullMode =
			cachedMaterialInfo.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		b.compatibleRenderPass = SimpleRenderer::get()->renderPass;

		DescriptorSetLayout frameGlobalSetLayout = SimpleRenderer::get()->getFrameGlobalLayout();
		DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
		b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
		b.useDescriptorSetLayout(DSET_BINDLESS, BindlessResources::Instance->layout());
		b.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);
		b.usePushConstantRange({VK_SHADER_STAGE_VERTEX_BIT, GLTF_MODEL_MATRIX_PUSH_OFFSET, GLTF_MODEL_MATRIX_PUSH_SIZE});
		b.usePushConstantRange({VK_SHADER_STAGE_FRAGMENT_BIT, GLTF_MATERIAL_INDEX_PUSH_OFFSET, GLTF_MATERIAL_INDEX_PUSH_SIZE});

		b.pipelineState.colorBlendAttachments = { b.pipelineState.colorBlendAttachmentInfo };

		graphicsPipeline.build("SimpleGltf");
	}
	return graphicsPipeline;
}