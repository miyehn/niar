#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include "DescriptorSet.h"

struct PipelineState
{
	PipelineState();

	void setExtent(uint32_t width, uint32_t height);

	VkGraphicsPipelineCreateInfo getPipelineInfoTemplate();

	VkExtent2D targetExtent;

	// vertex input
	bool useVertexInput;
	VkVertexInputBindingDescription bindingDescription;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;

	// input assembly
	bool useInputAssembly;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;

	// viewport
	bool useViewport;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportInfo;

	// rasterization
	bool useRasterization;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;

	// multisampling (along edges for anti aliasing)
	bool useMultisampling;
	VkPipelineMultisampleStateCreateInfo multisamplingInfo;

	// depth stencil
	bool useDepthStencil;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

	// color blending
	bool useColorBlending;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;

	// dynamic state
	bool useDynamicState;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
};

struct GraphicsPipelineBuilder
{
	GraphicsPipelineBuilder() = default;

	void useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout);

	void build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout);

	std::string vertPath;
	std::string fragPath;

	PipelineState pipelineState{};

	VkRenderPass compatibleRenderPass = VK_NULL_HANDLE;
	uint32_t compatibleSubpass = 0;

private:
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
};

struct ComputePipelineBuilder
{
	ComputePipelineBuilder() = default;

	void useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout);

	void build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout);

	std::string shaderPath;

private:
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
};

struct RayTracingPipelineBuilder
{
	RayTracingPipelineBuilder() = default;

	void useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout);

	void build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout);

	std::string rgenPath;
	std::string rchitPath;
	std::string rmissPath;

private:
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
};