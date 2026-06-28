#pragma once
#include "GltfMaterialInfo.h"
#include "Render/Vulkan/Pipeline.h"
#include <string>

class SceneObject;

// virtual class; see inherited ones below
class GltfMaterial
{
public:
	std::string name;

	// per-draw
	void setPerDrawParameters(VkCommandBuffer cmdbuf, SceneObject* drawable);

	uint32_t getVersion() const { return cachedMaterialInfo._version; }

	bool isOpaque() const { return cachedMaterialInfo.blendMode == BM_OpaqueOrClip; }

	virtual const GraphicsPipeline& getPipeline() = 0;
	virtual ~GltfMaterial() = default;

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