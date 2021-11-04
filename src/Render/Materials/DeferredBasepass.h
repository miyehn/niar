#pragma once
#include "Material.h"

class Texture2D;

class MatDeferredBasepass : public Material
{
public:

	MatDeferredBasepass(
		const std::string &albedo_path,
		const std::string &normal_path,
		const std::string &metallic_path,
		const std::string &roughness_path,
		const std::string &ao_path = ""
	);
	void setParameters(SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets={}) override;
	~MatDeferredBasepass() override;

private:

	struct {
		alignas(16) glm::mat4 ModelMatrix;
	} uniforms;

	VmaBuffer uniformBuffer;

	Texture2D* albedo;
	Texture2D* normal;
	Texture2D* metallic;
	Texture2D* roughness;
	Texture2D* ao;

	static VkPipeline pipeline;
	static VkPipelineLayout pipelineLayout;

	DescriptorSet dynamicSet;

    friend class Material;
};
