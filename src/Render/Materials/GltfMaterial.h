#pragma once
#include "Material.h"

class Texture2D;

namespace tinygltf { struct Material; }

// used for each renderer to create corresponding materials
struct GltfMaterialInfo {
	uint32_t _version;
	std::string name;
	std::string albedoTexName;
	std::string normalTexName;
	std::string mrTexName;
	std::string aoTexName;
	glm::vec4 BaseColorFactor;
	glm::vec4 EmissiveFactor;
	glm::vec4 MetallicRoughnessAONormalStrengths;
};

// virtual class; see inherited ones below
// currently not handling emission because blender gltf 2.0 export seems broken..
class GltfMaterial : public Material
{
public:
	void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf) override;
	~GltfMaterial() override;

	void resetInstanceCounter() override;

	static void addInfo(GltfMaterialInfo& info);
	static GltfMaterialInfo* getInfo(const std::string& materialName);

	uint32_t getVersion() { return _version; }

protected:
	explicit GltfMaterial(const GltfMaterialInfo& info);
	DescriptorSet dynamicSet;

	// used for checking if this gltf material is obsolete and need to be re-created
	uint32_t _version = 0;

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

	uint32_t instanceCounter = 0;
};

class PbrGltfMaterial : public GltfMaterial
{
public:
	explicit PbrGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	MaterialPipeline getPipeline() override;
};

class SimpleGltfMaterial : public GltfMaterial
{
public:
	explicit SimpleGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	MaterialPipeline getPipeline() override;
};