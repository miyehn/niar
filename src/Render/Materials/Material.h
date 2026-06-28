#pragma once
#include "Render/Vulkan/Pipeline.h"

#include <string>

class SceneObject;

class Material
{
public:
	std::string name;

	// aka should only just be push constants
	virtual void setPerDrawParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) {};

	virtual ~Material() = default;

	virtual const GraphicsPipeline& getPipeline() = 0;
};
