#pragma once
#include "Material.h"

class Texture2D;

class MatDeferredBasepass : public Material
{
public:

	MatDeferredBasepass(
		const std::string &name,
		const std::string &albedo_tex,
		const std::string &normal_tex,
		const std::string &metallic_tex,
		const std::string &roughness_tex,
		const std::string &ao_path = ""
	);
	void setParameters(SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets={}) override;
	~MatDeferredBasepass() override;

	static void cleanup();

private:

	struct {
		alignas(16) glm::mat4 ModelMatrix;
	} uniforms;

	VmaBuffer uniformBuffer;

	static VkPipeline pipeline;
	static VkPipelineLayout pipelineLayout;

	DescriptorSet dynamicSet;
};
