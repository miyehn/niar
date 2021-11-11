#pragma once
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

class SceneObject;

struct DescriptorSetBindingSlot
{
	DescriptorSet descriptorSet;
	uint32_t bindingSlot;
};

class Material
{
public:
	std::string name;

	virtual void setParameters(SceneObject* drawable) {};
	virtual void usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets = {}) = 0;

	virtual ~Material() = default;

	static Material* find(const std::string& name);
	static void cleanup();

protected:

	static void add(Material* material);

private:

	static std::unordered_map<std::string, Material*> pool;
};
