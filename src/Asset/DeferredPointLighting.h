#pragma once
#include "Material.h"
#include "Render/Vulkan/DeferredRenderer.h"

class DeferredPointLighting : public Material
{
public:

	explicit DeferredPointLighting(DeferredRenderer& deferredRenderer);
	void use(VkCommandBuffer &cmdbuf) override;
	int get_id() override { return 5; }
	~DeferredPointLighting() override;

	Texture2D* GBUF0;
	Texture2D* GBUF1;
	Texture2D* GBUF2;
	Texture2D* GBUF3;

	struct PointLight {
		glm::vec3 position;
		glm::vec3 color;
	};

	struct
	{
		glm::vec3 CameraPosition;
		int NumLights;
		PointLight Lights[4]; // need to match shader
	} uniforms;

private:

	VmaBuffer uniformBuffer;
	DescriptorSet descriptorSet;
};