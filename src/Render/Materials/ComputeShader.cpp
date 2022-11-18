#include "ComputeShader.h"
#include "Render/Vulkan/PipelineBuilder.h"

void ComputeShader::initializePipeline() {
	if (pipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE) return;

	ASSERT(descriptorSetPtr != nullptr)

	ComputePipelineBuilder pipelineBuilder{};
	pipelineBuilder.shaderPath = shaderPath;
	pipelineBuilder.useDescriptorSetLayout(DSET_DYNAMIC, descriptorSetPtr->getLayout());
	pipelineBuilder.build(pipeline, pipelineLayout);
}