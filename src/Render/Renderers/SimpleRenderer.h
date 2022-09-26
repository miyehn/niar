#include "Render/Vulkan/DescriptorSet.h"
#include "DeferredRenderer.h"

class Texture2D;
class Material;
class DebugLines;
class GltfMaterial;

class SimpleRenderer : public Renderer
{
public:
	void render(VkCommandBuffer cmdbuf) override;

	static SimpleRenderer* get();

	VkRenderPass renderPass;
	DescriptorSet descriptorSet;

private:
	SimpleRenderer();
	~SimpleRenderer() override;

	VkExtent2D renderExtent;
	Texture2D* sceneColor;
	Texture2D* sceneDepth;
	VkFramebuffer frameBuffer;

	DebugLines* debugLines = nullptr;

	// uniforms
	ViewInfo viewInfo;

	VmaBuffer viewInfoUbo;

	void updateViewInfoUbo();

	// materials
	std::unordered_map<std::string, GltfMaterial*> materials;
	Material* getOrCreateMeshMaterial(const std::string& materialName);
};
