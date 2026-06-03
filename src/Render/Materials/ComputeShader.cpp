#include "ComputeShader.h"
#include "Render/Vulkan/Pipeline.h"

const ComputePipeline& ComputeShader::getPipeline() {
	ASSERT(descriptorSetPtr != nullptr)

	if (!computePipeline.valid()) {
		auto& b = computePipeline.builder;
		b.shaderPath = shaderPath;
		b.useDescriptorSetLayout(DSET_INDEPENDENT, descriptorSetPtr->getLayout());
		computePipeline.build(debugName);
	}
	return computePipeline;
}