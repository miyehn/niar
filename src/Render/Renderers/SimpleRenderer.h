#include "Render/Vulkan/DescriptorSet.h"
#include "Renderer.h"

class Texture2D;
class Material;
class DebugLines;

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
	struct
	{
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

		glm::vec3 CameraPosition;
		float _pad0 = 2.333f;

		glm::vec3 ViewDir;
		float _pad1 = 2.333f;

	} ViewInfo;

	VmaBuffer viewInfoUbo;

	void updateViewInfoUbo();

	static Material* getOrCreateMeshMaterial(const std::string& materialName);
};
