#pragma once
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

class ComputeShader {
public:
	// set by caller
	DescriptorSet* descriptorSetPtr = nullptr;

	template<typename T>
	static T* getInstance() {
		static T* instance;
		if (!instance) {
			instance = new T();
			Vulkan::Instance->destructionQueue.emplace_back([](){ delete instance; });
		}
		return instance;
	}
	virtual void dispatch(int groupCountX, int groupCountY, int groupCountZ) = 0;
protected:
	std::string shaderPath; // set by inherited class constructor
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	void initializePipeline();
};