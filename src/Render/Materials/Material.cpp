#include "Material.h"
#include "Engine/Drawable.hpp"
#include "Scene/Camera.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Renderers/Renderer.h"
#include "Render/Renderers/DeferredRenderer.h"
#include "Texture.h"

std::unordered_map<std::string, Material*> material_pool{};

void add_material(Material* material)
{
	auto mat_iter = material_pool.find(material->name);
	if (mat_iter != material_pool.end())
	{
		WARN("Adding material of duplicate names. Overriding..")
		delete mat_iter->second;
	}
	material_pool[material->name] = material;
}

Material::~Material()
{
	vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
	vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	for (auto layout : descriptorSetLayouts)
	{
		vkDestroyDescriptorSetLayout(Vulkan::Instance->device, layout.layout, nullptr);
	}
}

Material *Material::find(const std::string &name)
{
	auto mat_iter = material_pool.find(name);
	if (mat_iter != material_pool.end()) return mat_iter->second;
	return nullptr;
}

void Material::cleanup()
{
	for (auto it : material_pool)
	{
		delete it.second;
	}
}

MatTest::MatTest(const std::string &tex_path)
{
	name = "test material";
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	texture = dynamic_cast<Texture2D*>(Texture::get(tex_path));
	add_material(this);

	auto vk = Vulkan::Instance;

	{// pipeline and descriptor sets
		PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/basic.vert.spv";
		pipelineBuilder.fragPath = "spirv/basic.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = AnotherRenderer::get()->getRenderPass();
		pipelineBuilder.add_binding(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipelineBuilder.add_binding(0, 1, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipeline = pipelineBuilder.build();

		descriptorSetLayouts = pipelineBuilder.descriptorSetLayouts;
		pipelineLayout = pipelineBuilder.pipelineLayout;

		descriptorSet = DescriptorSet(vk->device, descriptorSetLayouts[0]);
		descriptorSet.pointToUniformBuffer(uniformBuffer, 0);
		descriptorSet.pointToImageView(texture->imageView, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}
}

void MatTest::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms);

	auto dset = descriptorSet.getInstance();
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&dset,
							0, nullptr); // for dynamic descriptors (not reached yet)
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

MatTest::~MatTest()
{
	uniformBuffer.release();
}

void MatTest::set_parameters(Drawable *drawable)
{
	uniforms = {
		.ModelMatrix = drawable->object_to_world(),
		.ViewMatrix = Camera::Active->world_to_camera(),
		.ProjectionMatrix = Camera::Active->camera_to_clip()
	};
	// so it's not upside down
	uniforms.ProjectionMatrix[1][1] *= -1;
}

Geometry::Geometry(
	const std::string &albedo_path,
	const std::string &normal_path,
	const std::string &metallic_path,
	const std::string &roughness_path,
	const std::string &ao_path,
	const glm::vec3 &in_tint
){
	name = "geometry";
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU);
	add_material(this);

	{// params
		albedo = dynamic_cast<Texture2D*>(Texture::get(albedo_path));
		normal = dynamic_cast<Texture2D*>(Texture::get(normal_path));
		metallic = dynamic_cast<Texture2D*>(Texture::get(metallic_path));
		roughness = dynamic_cast<Texture2D*>(Texture::get(roughness_path));
		ao = dynamic_cast<Texture2D*>(Texture::get(ao_path));
		uniforms.tint = in_tint;
	}

	auto vk = Vulkan::Instance;

	{// pipeline and layouts
		PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/geometry.vert.spv";
		pipelineBuilder.fragPath = "spirv/geometry.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = DeferredRenderer::get()->getRenderPass();
		pipelineBuilder.add_binding(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipelineBuilder.add_binding(0, 1, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipelineBuilder.add_binding(0, 2, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipelineBuilder.add_binding(0, 3, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipelineBuilder.add_binding(0, 4, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		pipelineBuilder.add_binding(0, 5, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		// since it has 4 color outputs:
		auto blendInfo = pipelineBuilder.pipelineState.colorBlendAttachmentInfo;
		const VkPipelineColorBlendAttachmentState blendInfoArray[4] = { blendInfo, blendInfo, blendInfo, blendInfo };
		pipelineBuilder.pipelineState.colorBlendInfo.attachmentCount = 4;
		pipelineBuilder.pipelineState.colorBlendInfo.pAttachments = blendInfoArray;
		// build
		pipeline = pipelineBuilder.build();

		descriptorSetLayouts = pipelineBuilder.descriptorSetLayouts;
		pipelineLayout = pipelineBuilder.pipelineLayout;
	}
	{// allocate the descriptor sets
		descriptorSet = DescriptorSet(vk->device, descriptorSetLayouts[0]);
		descriptorSet.pointToUniformBuffer(uniformBuffer, 0);
		descriptorSet.pointToImageView(albedo->imageView, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSet.pointToImageView(normal->imageView, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSet.pointToImageView(metallic->imageView, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSet.pointToImageView(roughness->imageView, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorSet.pointToImageView(ao->imageView, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}

}
void Geometry::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms);

	auto dset = descriptorSet.getInstance();
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&dset,
							0, nullptr); // for dynamic descriptors (not reached yet)
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

Geometry::~Geometry()
{
	uniformBuffer.release();
}

void Geometry::set_parameters(Drawable *drawable)
{
	uniforms = {
		.ModelMatrix = drawable->object_to_world(),
		.ViewMatrix = Camera::Active->world_to_camera(),
		.ProjectionMatrix = Camera::Active->camera_to_clip(),
		.tint = glm::vec3(1)
	};
	// so it's not upside down
	uniforms.ProjectionMatrix[1][1] *= -1;
}
