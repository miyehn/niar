#pragma once
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

// operates on a vertex/compute buffer, slowly lifts up each point
class Rise
{
public:
	static void dispatch(VmaBuffer& targetBuffer, DescriptorSet& frameGlobalDescriptorSet);

private:
	Rise(VmaBuffer& targetBuffer, DescriptorSet& frameGlobalDescriptorSet);
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	DescriptorSet dynamicSet;
};