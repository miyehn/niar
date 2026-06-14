#include "ComputeShader.h"
#include "Render/Vulkan/Pipeline.h"

const ComputePipeline& ComputeShader::getPipeline() {
	if (!computePipeline.valid()) {
		auto& b = computePipeline.builder;
		configurePipeline(b);
		computePipeline.build(b.shaderDef.shader_module_key());
	}
	return computePipeline;
}