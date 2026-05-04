#pragma once
#include "Material.h"
#include "GltfMaterialInfo.h"

class Texture2D;

namespace tinygltf { struct Material; }

// virtual class; see inherited ones below
class GltfMaterial : public Material
{
public:
	void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) override;
	void usePipeline(VkCommandBuffer cmdbuf) override;
	~GltfMaterial() override;

	virtual void markPipelineDirty() = 0;

	uint32_t getVersion()const { return cachedMaterialInfo._version; }

	bool isOpaque() const { return cachedMaterialInfo.blendMode == BM_OpaqueOrClip; }

protected:
	explicit GltfMaterial(const GltfMaterialInfo& info);
	DescriptorSet dynamicSet;

	// used for checking if this gltf material is obsolete and need to be re-created
	GltfMaterialInfo cachedMaterialInfo;

private:

	// static (per-material-instance)
	struct {
		glm::vec4 BaseColorFactor;
		glm::vec4 OcclusionRoughnessMetallicNormalStrengths;
		glm::vec4 EmissiveFactorClipThreshold;
		glm::vec4 _pad0;
	} materialParams;
	VmaBuffer materialParamsBuffer;
};

class PbrGltfMaterial : public GltfMaterial
{
public:
	explicit PbrGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	MaterialPipeline getPipeline() override;
	void markPipelineDirty() override { pipelineIsDirty = true; }
private:
	static bool pipelineIsDirty;
};

class PbrTranslucentGltfMaterial : public GltfMaterial
{
public:
	explicit PbrTranslucentGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	MaterialPipeline getPipeline() override;
	void markPipelineDirty() override { pipelineIsDirty = true; }
private:
	static bool pipelineIsDirty;
};

class SimpleGltfMaterial : public GltfMaterial
{
public:
	explicit SimpleGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	MaterialPipeline getPipeline() override;
	void markPipelineDirty() override { pipelineIsDirty = true; }
private:
	static bool pipelineIsDirty;
};