#pragma once
#include "Render/Vulkan/PipelineBuilder.h"

#include <string>

class SceneObject;

struct MaterialPipeline
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;

	bool operator==(const MaterialPipeline& rhs) const {
		return this->pipeline == rhs.pipeline && this->layout == rhs.layout;
	}
	bool operator<(const MaterialPipeline& rhs) const {
		// sort order: group material pipelines of the same layout together
		if (this->layout != rhs.layout) return this->layout < rhs.layout;
		return this->pipeline < rhs.pipeline;
	}
};

class Material
{
public:
	std::string name;

	virtual void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) {};
	virtual void usePipeline(VkCommandBuffer cmdbuf) = 0;

	virtual ~Material() = default;

	virtual MaterialPipeline getPipeline() = 0;
};
