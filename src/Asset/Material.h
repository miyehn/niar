#pragma once
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"

#include <string>
#include <vector>

class Material
{
public:
	Material() = default;
	std::string name = "[anonymous material]";
	virtual int get_id() = 0;
	virtual void use(VkCommandBuffer &cmdbuf) = 0;
	virtual ~Material() = default;
};

class MatTest : public Material
{
public:
	MatTest();
	int get_id() override { return 0; }
	void use(VkCommandBuffer &cmdbuf) override;
	~MatTest() override;
private:

	struct
	{
		alignas(16) mat4 ModelMatrix;
		alignas(16) mat4 ViewMatrix;
		alignas(16) mat4 ProjectionMatrix;
	} uniforms;

	VkDescriptorSetLayout descriptorSetLayout{};
	VkPipelineLayout pipelineLayout{};
	VkPipeline pipeline{};

	VmaBuffer uniformBuffer;
	std::vector<VkDescriptorSet> descriptorSets;
};

Material* find_material(const std::string& name);