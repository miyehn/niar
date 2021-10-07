#pragma once
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

class Material
{
public:
	Material() = default;
	std::string name = "[anonymous material]";
	virtual int get_id() = 0;
	virtual void use(VkCommandBuffer &cmdbuf) = 0;
	virtual ~Material();

protected:

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<DescriptorSet> descriptorSets;

	VkPipelineLayout pipelineLayout{};
	VkPipeline pipeline{};
};

class MatTest : public Material
{
public:
	MatTest();
	int get_id() override { return 0; }
	void use(VkCommandBuffer &cmdbuf) override;
	~MatTest() override;

	struct
	{
		alignas(16) glm::mat4 ModelMatrix;
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
	} uniforms;

private:

	VmaBuffer uniformBuffer;
};

Material* find_material(const std::string& name);