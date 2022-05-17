#pragma once
#include "Material.h"

class Texture2D;

namespace tinygltf { struct Material; }

// virtual class; see inherited ones below
// currently not handling emission because blender gltf 2.0 export seems broken..
class GltfMaterial : public Material
{
public:
	void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf) override;
	~GltfMaterial() override;

	void resetInstanceCounter() override;

protected:
	GltfMaterial(
		const tinygltf::Material& in_material,
		const std::vector<std::string>& texture_names
	);
	DescriptorSet dynamicSet;

private:

	// dynamic (per-object)
	struct {
		glm::mat4 ModelMatrix;
	} uniforms;
	VmaBuffer uniformBuffer;

	// static (per-material-instance)
	struct {
		glm::vec4 BaseColorFactor;
		glm::vec4 MetallicRoughnessAONormalStrengths;
		glm::vec4 _pad0;
		glm::vec4 _pad1;
	} materialParams;
	VmaBuffer materialParamsBuffer;

	uint32_t instanceCounter;
};

class PbrGltfMaterial : public GltfMaterial
{
public:
	PbrGltfMaterial(
		const tinygltf::Material& in_material,
		const std::vector<std::string>& texture_names
	) : GltfMaterial(in_material, texture_names) {}

	MaterialPipeline getPipeline() override;
};

class SimpleGltfMaterial : public GltfMaterial
{
public:
	SimpleGltfMaterial(
		const tinygltf::Material& in_material,
		const std::vector<std::string>& texture_names
	) : GltfMaterial(in_material, texture_names) {}

	MaterialPipeline getPipeline() override;
};