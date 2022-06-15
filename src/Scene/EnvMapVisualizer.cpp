//
// Created by raindu on 6/14/2022.
//

#include "Render/Materials/Material.h"
#include "EnvMapVisualizer.h"
#include "Assets/MeshAsset.h"
#include "Render/Mesh.h"
#include "Utils/myn/Log.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Renderers/DeferredRenderer.h"

#define MAX_NUM_PROBES 128

class ProbeMaterial : public Material {
public:
	ProbeMaterial() {
		name = "probe material";

		VkDeviceSize alignment = Vulkan::Instance->minUniformBufferOffsetAlignment;
		uint32_t numBlocks = (sizeof(uniforms) + alignment - 1) / alignment;

		// TODO: dynamically get numStrides (num instances of this material)
		uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								  numBlocks * alignment,
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU,
								  1, MAX_NUM_PROBES);

		{// pipeline and layouts

			// set layouts and allocation
			DescriptorSetLayout dynamicSetLayout{};
			dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
			dynamicSet = DescriptorSet(dynamicSetLayout); // this commits the bindings

			// assign actual values to them
			dynamicSet.pointToBuffer(uniformBuffer, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		}
	}
	~ProbeMaterial() override {
		uniformBuffer.release();
	}
	MaterialPipeline getPipeline() override {
		static MaterialPipeline materialPipeline = {};
		if (materialPipeline.pipeline == VK_NULL_HANDLE || materialPipeline.layout == VK_NULL_HANDLE)
		{
			auto vk = Vulkan::Instance;

			// now build the pipeline
			GraphicsPipelineBuilder pipelineBuilder{};
			pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
			pipelineBuilder.fragPath = "spirv/envmap_visualizer.frag.spv";
			pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->renderPass;
			pipelineBuilder.compatibleSubpass = DEFERRED_SUBPASS_PROBES;

			DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->frameGlobalDescriptorSet.getLayout();
			DescriptorSetLayout dynamicSetLayout = dynamicSet.getLayout();
			pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

			auto blendInfo = pipelineBuilder.pipelineState.colorBlendAttachmentInfo;
			pipelineBuilder.pipelineState.colorBlendInfo.attachmentCount = 1;
			pipelineBuilder.pipelineState.colorBlendInfo.pAttachments = &blendInfo;

			pipelineBuilder.build(materialPipeline.pipeline, materialPipeline.layout);
		}

		return materialPipeline;
	}
	void setParameters(VkCommandBuffer cmdbuf, SceneObject* obj) override {
		// per-object params (dynamic)
		uniforms = {
			.ModelMatrix = obj->object_to_world(),
		};
		uniformBuffer.writeData(&uniforms, 0, 0, instanceCounter);

		uint32_t offset = uniformBuffer.strideSize * instanceCounter;
		dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_DYNAMIC, getPipeline().layout, 0, 1, &offset);

		instanceCounter++;
	}
	void usePipeline(VkCommandBuffer cmdbuf) override {
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline().pipeline);
	}
	void resetInstanceCounter() override {
		instanceCounter = 0;
	}
private:
	DescriptorSet dynamicSet;

	// dynamic (per-object)
	struct {
		glm::mat4 ModelMatrix;
	} uniforms;
	VmaBuffer uniformBuffer;

	// this material can only ever have 1 material, but may be used on many probes
	// actually that might not be true... if I decide to use it in multiple renderers it would take a uniform buffer and a renderpass as input
	// reference DebugPoint/Lines

	uint32_t instanceCounter = 0;
};

EnvMapVisualizer::EnvMapVisualizer()
{
	_scale = glm::vec3(0.25f, 0.25f, 0.25f);
}

#if GRAPHICS_DISPLAY
void EnvMapVisualizer::draw(VkCommandBuffer cmdbuf) {
	Mesh* m = MeshAsset::find("sphere");
	EXPECT(m != nullptr, true)
	m->draw(cmdbuf);
}

Material* EnvMapVisualizer::get_material(){
	static ProbeMaterial* material = nullptr;
	if (!material) {
		material = new ProbeMaterial();
		Vulkan::Instance->destructionQueue.emplace_back([]() {
			delete material;
		});
	}
	return material;
}

#endif