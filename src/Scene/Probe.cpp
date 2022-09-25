//
// Created by raindu on 6/14/2022.
//

#include "Assets/ConfigAsset.hpp"
#include "Assets/EnvironmentMapAsset.h"
#include "Render/Texture.h"
#include "Render/Materials/Material.h"
#include "Probe.h"
#include "Assets/SceneAsset.h"
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
		uniformBuffer = VmaBuffer({&Vulkan::Instance->memoryAllocator,
								  numBlocks * alignment,
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VMA_MEMORY_USAGE_CPU_TO_GPU,
								  "Probes uniform buffer",
								  1, MAX_NUM_PROBES});

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

Probe::Probe()
{
	_scale = glm::vec3(0.25f, 0.25f, 0.25f);
	ui_show_transform = false;
}

#if GRAPHICS_DISPLAY
void Probe::draw(VkCommandBuffer cmdbuf) {
	Mesh* m = MeshAsset::find("sphere");
	EXPECT(m != nullptr, true)
	m->draw(cmdbuf);
}

Material* Probe::get_material(){
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

EnvMapVisualizer::EnvMapVisualizer() : Probe() {
	this->set_scale(glm::vec3(0.03f, 0.03f, 0.03f));
}

void EnvMapVisualizer::update(float elapsed) {
	SceneObject::update(elapsed);
	if (!Camera::Active) return;

	auto camPosWS = Camera::Active->world_position();
	auto camRot = Camera::Active->world_to_object_rotation();
	auto halfY = Camera::Active->fov;
	auto halfX = halfY * Camera::Active->aspect_ratio;

	glm::vec3 offset = Camera::Active->forward();
	offset += Camera::Active->right() * halfX * 0.4f;
	offset += -Camera::Active->up() * halfY * 0.4f;
	glm::vec3 targetPosWS = camPosWS + offset;

	glm::vec3 targetPosParentSpace = targetPosWS;
	if (parent) {
		targetPosParentSpace = myn::transform_point(parent->world_to_object(), targetPosWS);
	}

	set_local_position(targetPosParentSpace);
}
