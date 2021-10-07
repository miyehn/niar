#pragma once
#include "Render/Vulkan/Vulkan.hpp"

namespace gfx
{
	struct PipelineState
	{
		PipelineState(uint32_t width, uint32_t height);
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

	struct RenderPassBuilder
	{
		RenderPassBuilder();
		VkAttachmentDescription colorAttachment;
		VkAttachmentReference colorAttachmentRef;
		VkAttachmentDescription depthAttachment;
		VkAttachmentReference depthAttachmentRef;
		VkSubpassDescription subpass;
		VkSubpassDependency dependency;
		VkRenderPass build();
	};

	struct PipelineLayoutBuilder
	{
		PipelineLayoutBuilder();
		VkDescriptorSetLayoutBinding uboLayoutBinding; // TODO: an array?
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout build();
	};

	class Pipeline
	{
	public:
		Pipeline();
		~Pipeline();
		void use();

		VkPipeline getPipeline() { return pipeline; }
		VkRenderPass getRenderPass() { return renderPass; }
		VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
		VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; }
	private:
		VkPipeline pipeline;
		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
	};
}

