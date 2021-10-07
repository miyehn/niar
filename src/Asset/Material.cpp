#include "Material.h"
#include <unordered_map>
#include "Render/gfx/gfx.h"
#include "Render/Vulkan/Vulkan.hpp"

static std::unordered_map<std::string, Material*> material_pool{};

Material* find_material(const std::string& name)
{
	auto mat_iter = material_pool.find(name);
	if (mat_iter != material_pool.end()) return mat_iter->second;
	return nullptr;
}

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
		vkDestroyDescriptorSetLayout(Vulkan::Instance->device, layout, nullptr);
	}
}

MatTest::MatTest()
{
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU,
							  Vulkan::Instance->getNumSwapChainImages());
	name = "test material";
	add_material(this);

	auto vk = Vulkan::Instance;

	{// create the layouts and build the pipeline
		gfx::PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/triangle.vert.spv";
		pipelineBuilder.fragPath = "spirv/triangle.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = vk->getSwapChainRenderPass();
		pipelineBuilder.add_binding(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipeline = pipelineBuilder.build();

		descriptorSetLayouts = pipelineBuilder.getLayouts();
		pipelineLayout = pipelineBuilder.pipelineLayout;
	}

	{// allocate the descriptor sets
		auto numImages = vk->getNumSwapChainImages();

		descriptorSet = DescriptorSet(vk->device, descriptorSetLayouts[0], numImages);
		descriptorSet.pointToUniformBuffer(uniformBuffer, 0);
	}
}

void MatTest::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms, Vulkan::Instance->getCurrentFrameIndex());

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

MatBasicVulkan::MatBasicVulkan()
{
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU,
							  Vulkan::Instance->getNumSwapChainImages());
	name = "basic material";
	add_material(this);

	auto vk = Vulkan::Instance;

	{// create the layouts and build the pipeline
		gfx::PipelineBuilder pipelineBuilder{};
		pipelineBuilder.vertPath = "spirv/basic.vert.spv";
		pipelineBuilder.fragPath = "spirv/basic.frag.spv";
		pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
		pipelineBuilder.compatibleRenderPass = vk->getSwapChainRenderPass();
		pipelineBuilder.add_binding(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		pipeline = pipelineBuilder.build();

		descriptorSetLayouts = pipelineBuilder.getLayouts();
		pipelineLayout = pipelineBuilder.pipelineLayout;
	}

	{// allocate the descriptor sets
		auto numImages = vk->getNumSwapChainImages();

		descriptorSet = DescriptorSet(vk->device, descriptorSetLayouts[0], numImages);
		descriptorSet.pointToUniformBuffer(uniformBuffer, 0);
	}
}

void MatBasicVulkan::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms, Vulkan::Instance->getCurrentFrameIndex());

	auto dset = descriptorSet.getInstance();
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&dset,
							0, nullptr); // for dynamic descriptors (not reached yet)
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

MatBasicVulkan::~MatBasicVulkan()
{
	uniformBuffer.release();
}
