#pragma once
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/Pipeline.h"

class ComputeShader {
public:
	virtual ~ComputeShader() = default;

	// to use a CS, caller gets a pointer to T through this,
	// sets whatever input required by T, and calls dispatch
	template<typename T>
	static T* getInstance() {
		static T* instance;
		if (!instance) {
			instance = new T();
			Vulkan::Instance->destructionQueue.emplace_back([](){ delete instance; });
		}
		return instance;
	}
	virtual void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) = 0;
protected:
	const ComputePipeline& getPipeline();
	virtual void configurePipeline(ComputePipelineBuilder& builder) = 0;
private:
	ComputePipeline computePipeline;
};
