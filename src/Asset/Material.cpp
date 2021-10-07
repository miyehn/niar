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

	auto vk = Vulkan::Instance;

	gfx::PipelineBuilder pipelineBuilder{};
	pipelineBuilder.vertPath = "spirv/triangle.vert.spv";
	pipelineBuilder.fragPath = "spirv/triangle.frag.spv";
	pipelineBuilder.pipelineState.setExtent(vk->swapChainExtent.width, vk->swapChainExtent.height);
	pipelineBuilder.compatibleRenderPass = vk->getSwapChainRenderPass();
	pipelineBuilder.add_binding(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	pipeline = pipelineBuilder.build();

	descriptorSetLayout = pipelineBuilder.descriptorSetLayouts[0].layout;
	pipelineLayout = pipelineBuilder.pipelineLayout;

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

void MatTest::use(VkCommandBuffer &cmdbuf)
{
	uniformBuffer.writeData(&uniforms, Vulkan::Instance->getCurrentFrameIndex());

	auto dset = descriptorSets[Vulkan::Instance->getCurrentFrameIndex()];
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
	vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
	vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
	vkDestroyDescriptorSetLayout(Vulkan::Instance->device, descriptorSetLayout, nullptr);
}
