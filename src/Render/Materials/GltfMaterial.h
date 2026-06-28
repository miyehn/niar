#pragma once
#include "Material.h"
#include "GltfMaterialInfo.h"
#include "Render/Vulkan/Pipeline.h"

// virtual class; see inherited ones below
class GltfMaterial : public Material
{
public:
	// per-draw
	void setPerDrawParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) override;

	uint32_t getVersion() const { return cachedMaterialInfo._version; }

	bool isOpaque() const { return cachedMaterialInfo.blendMode == BM_OpaqueOrClip; }

protected:
	explicit GltfMaterial(const GltfMaterialInfo& info);

	// used for checking if this gltf material is obsolete and need to be re-created
	GltfMaterialInfo cachedMaterialInfo;
};

class PbrGltfMaterial : public GltfMaterial
{
public:
	explicit PbrGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	const GraphicsPipeline& getPipeline() override;
	static void destroyPipeline();
private:
	static GraphicsPipeline graphicsPipeline;
};

class PbrTranslucentGltfMaterial : public GltfMaterial
{
public:
	explicit PbrTranslucentGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	const GraphicsPipeline& getPipeline() override;
	static void destroyPipeline();
private:
	static GraphicsPipeline graphicsPipeline;
};

class SimpleGltfMaterial : public GltfMaterial
{
public:
	explicit SimpleGltfMaterial(const GltfMaterialInfo& info) : GltfMaterial(info) {}

	const GraphicsPipeline& getPipeline() override;
	static void destroyPipeline();
private:
	static GraphicsPipeline graphicsPipeline;
};