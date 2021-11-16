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

	virtual void setParameters(VkCommandBuffer cmdbuf, SceneObject* drawable) {};
	virtual void usePipeline(VkCommandBuffer cmdbuf) = 0;
	virtual VkPipeline getPipeline() { return VK_NULL_HANDLE; }

	virtual ~Material() = default;

	static Material* find(const std::string& name);
	static void resetInstanceCounters();

protected:

	static void add(Material* material);

	// materials with dynamic uniform buffers should implement this
	virtual void resetInstanceCounter() {}

private:

	static std::unordered_map<std::string, Material*> pool;
};