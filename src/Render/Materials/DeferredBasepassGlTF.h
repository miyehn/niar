#pragma once
#include "Material.h"

class Texture2D;

namespace tinygltf { struct Material; }

// currently not handling emission because blender gltf 2.0 export seems broken..
class MatDeferredBasepassGlTF : public Material
{
public:

	MatDeferredBasepassGlTF(
		const tinygltf::Material& in_material,
		const std::vector<std::string>& texture_names
	);
	void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets) override;
	VkPipeline getPipeline() override { return pipeline; }
	~MatDeferredBasepassGlTF() override;

	void resetInstanceCounter() override;

	static void cleanup();

private:

	struct {
		glm::mat4 ModelMatrix;
	} uniforms;

	struct {
		glm::vec4 BaseColorFactor;
		glm::vec4 MetallicRoughnessAONormalStrengths;
		glm::vec4 _pad0;
		glm::vec4 _pad1;
	} materialParams;

	VmaBuffer uniformBuffer;
	VmaBuffer materialParamsBuffer;

	static VkPipeline pipeline;
	static VkPipelineLayout pipelineLayout;

	DescriptorSet dynamicSet;
	uint32_t instanceCounter;
};
