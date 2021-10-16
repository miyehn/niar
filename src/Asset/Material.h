#pragma once
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

class Drawable;
class Texture2D;

class Material
{
public:
	Material() = default;
	std::string name = "[anonymous material]";
	virtual int get_id() = 0;
	virtual void set_parameters(Drawable* drawable) = 0;
	virtual void use(VkCommandBuffer &cmdbuf) = 0;
	virtual ~Material();

protected:

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	VkPipelineLayout pipelineLayout{};
	VkPipeline pipeline{};
};

class MatTest : public Material
{
public:

	explicit MatTest(const std::string &tex_path);
	int get_id() override { return 0; }
	void set_parameters(Drawable* drawable) override;
	void use(VkCommandBuffer &cmdbuf) override;
	~MatTest() override;

private:

	struct
	{
		alignas(16) glm::mat4 ModelMatrix;
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
	} uniforms;

	Texture2D* texture; // expected to be the same most of the time

	VmaBuffer uniformBuffer;
	DescriptorSet descriptorSet;
};

Material* find_material(const std::string& name);