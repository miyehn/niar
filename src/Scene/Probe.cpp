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
#include "Render/Vulkan/Pipeline.h"
#include "Render/Renderers/DeferredRenderer.h"
#include "Scene/SkyAtmosphere/SkyAtmosphere.h"

class ProbeMaterial : public Material {
public:
	ProbeMaterial() {
		name = "probe material";
	}
	~ProbeMaterial() override = default;
	const GraphicsPipeline& getPipeline() override {
		if (!graphicsPipeline.valid()) {
			auto vk = Vulkan::Instance;
			auto& b = graphicsPipeline.builder;
			b.vertDef = "shaders/geometry.vert";
			b.fragDef = "shaders/envmap_visualizer.frag";
			b.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
			b.compatibleRenderPass = DeferredRenderer::get()->mainPass;
			b.compatibleSubpass = DEFERRED_SUBPASS_PROBES;

			DescriptorSetLayout frameGlobalSetLayout = DeferredRenderer::get()->getFrameGlobalLayout();
			DescriptorSetLayout independentSetLayout = DeferredRenderer::get()->getSkyDescriptorSetLayout();
			b.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
			b.useDescriptorSetLayout(DSET_INDEPENDENT, independentSetLayout);
			b.usePushConstantRange({VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)});

			b.pipelineState.colorBlendAttachments = { b.pipelineState.colorBlendAttachmentInfo };

			graphicsPipeline.build("Probe EnvMap");
		}
		return graphicsPipeline;
	}
	void setPerDrawParameters(VkCommandBuffer cmdbuf, SceneObject* obj) override {
		glm::mat4 modelMatrix = obj->object_to_world();
		vkCmdPushConstants(cmdbuf, getPipeline().layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);
		DeferredRenderer::get()->getSkyDescriptorSet().bind(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, DSET_INDEPENDENT, getPipeline().layout);
	}
private:
	GraphicsPipeline graphicsPipeline;
};

Probe::Probe()
{
	_scale = glm::vec3(0.25f, 0.25f, 0.25f);
	ui_show_transform = false;
}

#if GRAPHICS_DISPLAY
void Probe::draw(VkCommandBuffer cmdbuf) {
	Mesh* m = MeshAsset::find("sphere");
	ASSERT(m != nullptr)
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
