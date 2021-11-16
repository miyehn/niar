#pragma once
#include "Material.h"

class Texture2D;

class DeferredBasepass : public Material
{
public:

	DeferredBasepass(
		const std::string &name,
		const std::string &albedo_tex,
		const std::string &normal_tex,
		const std::string &metallic_tex,
		const std::string &roughness_tex,
		const std::string &ao_path = ""
	);
	void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf) override;
	VkPipeline getPipeline() override { return pipeline; }
	~DeferredBasepass() override;

private:

	struct {
		alignas(16) glm::mat4 ModelMatrix;
	} uniforms;

	VmaBuffer uniformBuffer;

	static VkPipeline pipeline;
	static VkPipelineLayout pipelineLayout;

	DescriptorSet dynamicSet;
};
