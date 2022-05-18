#pragma once
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

class SceneObject;

struct MaterialPipeline
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;

	bool operator==(const MaterialPipeline& rhs) const {
		return this->pipeline == rhs.pipeline && this->layout == rhs.layout;
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

	/*
	template<class T> static VkPipelineLayout findPipelineLayout()
	{
		static_assert(std::is_base_of<Material, T>::value, "trying to find pipeline layout of something immaterial (?)");
		for (const auto& it : pool)
		{
			T* typedMaterial = dynamic_cast<T*>(it.second);
			if (typedMaterial != nullptr)
			{
				auto material = dynamic_cast<Material*>(typedMaterial);
				return material->getPipeline().layout;
			}
		}
		return VK_NULL_HANDLE;
	}
	 */

protected:

	// materials with dynamic uniform buffers should implement this
	virtual void resetInstanceCounter() {}

};
