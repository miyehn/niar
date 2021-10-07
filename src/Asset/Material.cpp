#include "Material.h"
#include <unordered_map>
#include "Render/gfx/gfx.h"

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

MatTest::MatTest()
{
	uniformBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator,
							  sizeof(uniforms),
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VMA_MEMORY_USAGE_CPU_TO_GPU,
							  Vulkan::Instance->getNumSwapChainImages());
	name = "test material";
	add_material(this);

	// descriptor set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
		.pImmutableSamplers = nullptr // for image sampling related?
	};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &uboLayoutBinding,
	};
	EXPECT(vkCreateDescriptorSetLayout(Vulkan::Instance->device, &layoutInfo, nullptr, &descriptorSetLayout), VK_SUCCESS)

	// pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	EXPECT(vkCreatePipelineLayout(Vulkan::Instance->device, &pipelineLayoutInfo, nullptr, &pipelineLayout), VK_SUCCESS)

	// pipeline
	auto vert_module = gfx::ShaderModule::get("spirv/triangle.vert.spv");
	auto frag_module = gfx::ShaderModule::get("spirv/triangle.frag.spv");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_module->module,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_module->module,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// create the fucking pipeline
	gfx::PipelineState pipelineState(Vulkan::Instance->swapChainExtent.width, Vulkan::Instance->swapChainExtent.height);
	auto pipelineInfo = pipelineState.getPipelineInfoTemplate();
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = Vulkan::Instance->getSwapChainRenderPass();
	pipelineInfo.subpass = 0;

	EXPECT(vkCreateGraphicsPipelines(Vulkan::Instance->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), VK_SUCCESS)

	// and descriptor sets (ouch)
	{
		auto numImages = Vulkan::Instance->getNumSwapChainImages();
		auto descriptorPool = Vulkan::Instance->getDescriptorPool();
		std::vector<VkDescriptorSetLayout> layouts(numImages, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(numImages),
			.pSetLayouts = layouts.data(),
		};
		descriptorSets.resize(numImages);
		EXPECT(vkAllocateDescriptorSets(Vulkan::Instance->device, &allocInfo, descriptorSets.data()), VK_SUCCESS)

		for (size_t i=0; i<numImages; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {
				.buffer = uniformBuffer.getBufferInstance(i),
				.offset = 0,
				.range = VK_WHOLE_SIZE,
			};
			// "Structure specifying the parameters of a descriptor set write operation"
			VkWriteDescriptorSet descriptorWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				// actual write data (one of three)
				.pImageInfo = nullptr,
				.pBufferInfo = &bufferInfo, // where in which buffer
				.pTexelBufferView = nullptr,
			};
			vkUpdateDescriptorSets(Vulkan::Instance->device, 1, &descriptorWrite, 0, nullptr);
		}
	}
}

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
void MatTest::use(VkCommandBuffer &cmdbuf)
{
	// temporary update
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	uniforms = {
		.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.ProjectionMatrix = glm::perspective(glm::radians(45.0f), Vulkan::Instance->swapChainExtent.width / (float) Vulkan::Instance->swapChainExtent.height, 0.1f, 10.0f),
	};
	// so it's not upside down
	uniforms.ProjectionMatrix[1][1] *= -1;

	uniformBuffer.writeData(&uniforms, Vulkan::Instance->getCurrentFrameIndex());

	auto dset = descriptorSets[Vulkan::Instance->getCurrentFrameIndex()];
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&dset,
							0, nullptr); // for dynamic descriptors (not reached yet)
}

MatTest::~MatTest()
{
	uniformBuffer.release();
	vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
	vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	vkDestroyDescriptorSetLayout(Vulkan::Instance->device, descriptorSetLayout, nullptr);
}
