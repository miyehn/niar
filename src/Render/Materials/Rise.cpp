#include "ComputeShaders.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vulkan/PipelineBuilder.h"

void Rise::dispatch(VmaBuffer& targetBuffer, DescriptorSet& frameGlobalDescriptorSet)
{
	static Rise* instance = nullptr;
	if (!instance)
	{
		instance = new Rise(targetBuffer, frameGlobalDescriptorSet);
		Vulkan::Instance->destructionQueue.emplace_back([](){ delete instance; });
	}

	Vulkan::Instance->immediateSubmit(
		[&](VkCommandBuffer cmdbuf)
		{
			SCOPED_DRAW_EVENT(cmdbuf, "Dispatch Rise")
			vk::insertBufferBarrier(
				cmdbuf,
				targetBuffer.getBufferInstance(),
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				VK_ACCESS_SHADER_WRITE_BIT);

			vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, instance->pipeline);
			frameGlobalDescriptorSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_FRAMEGLOBAL, instance->pipelineLayout);
			instance->dynamicSet.bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_DYNAMIC, instance->pipelineLayout);
			vkCmdDispatch(cmdbuf, 2, 1, 1);

			vk::insertBufferBarrier(
				cmdbuf,
				targetBuffer.getBufferInstance(),
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
		});
}

Rise::Rise(VmaBuffer& targetBuffer, DescriptorSet& frameGlobalDescriptorSet)
{
	ComputePipelineBuilder pipelineBuilder{};
	// shader
	pipelineBuilder.shaderPath = "spirv/sine.comp.spv";
	// frameglobal
	DescriptorSetLayout frameGlobalSetLayout = frameGlobalDescriptorSet.getLayout();
	pipelineBuilder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, frameGlobalSetLayout);
	// other input / outputs
	DescriptorSetLayout dynamicSetLayout{};
	dynamicSetLayout.addBinding(0, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	dynamicSet = DescriptorSet(dynamicSetLayout);
	dynamicSet.pointToBuffer(targetBuffer, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, dynamicSetLayout);

	pipelineBuilder.build(pipeline, pipelineLayout);
}
