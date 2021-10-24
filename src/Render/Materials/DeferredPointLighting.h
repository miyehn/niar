#pragma once
#include "Material.h"
#include "Render/Renderers/DeferredRenderer.h"

class DeferredPointLighting : public Material
{
public:

	explicit DeferredPointLighting(DeferredRenderer& deferredRenderer);
	void use(VkCommandBuffer &cmdbuf) override;
	int get_id() override { return 5; }
	~DeferredPointLighting() override;

	struct PointLight {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

	struct
	{
		PointLight Lights[4]; // need to match shader
	} uniforms;

private:

	VmaBuffer uniformBuffer;
	DescriptorSet dynamicSet;
};