#include "Render/Vulkan/DescriptorSet.h"
#include "Renderer.h"

class Texture2D;

class SimpleRenderer : public Renderer
{
private:
	SimpleRenderer();
	~SimpleRenderer() override;

public:
	void render(VkCommandBuffer cmdbuf) override;

	void debugSetup(std::function<void()> fn) override;

	static SimpleRenderer* get();

	VkRenderPass renderPass;
	DescriptorSet descriptorSet;

private:
	VkExtent2D renderExtent;
	Texture2D* sceneColor;
	Texture2D* sceneDepth;
	VkFramebuffer frameBuffer;

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
