#pragma once
#include "Render/Vulkan/PipelineBuilder.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

#include <string>
#include <vector>
#include <unordered_map>
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

	static Material* find(const std::string& name);
	static void cleanup();

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

class Geometry : public Material
{
public:

	Geometry(
		const std::string &albedo_path,
		const std::string &normal_path,
		const std::string &metallic_path,
		const std::string &roughness_path,
		const std::string &ao_path,
		const glm::vec3 &in_tint
		);
	int get_id() override { return 2; }
	void set_parameters(Drawable* drawable) override;
	void use(VkCommandBuffer &cmdbuf) override;
	~Geometry() override;

private:

	struct
	{
		alignas(16) glm::mat4 ModelMatrix;
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
		alignas(16) glm::vec3 tint;
	} uniforms;

	Texture2D* albedo; // expected to be the same most of the time
	Texture2D* normal;
	Texture2D* metallic;
	Texture2D* roughness;
	Texture2D* ao;

	VmaBuffer uniformBuffer;
	DescriptorSet descriptorSet;
};