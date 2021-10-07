#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <string>

namespace gfx
{
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

	struct PipelineBuilder
	{
		PipelineBuilder();

		void add_binding(uint32_t setIndex, uint32_t bindingIndex, VkShaderStageFlags shaderStages, VkDescriptorType type);

		VkPipeline build();

		std::vector<VkDescriptorSetLayout> getLayouts();

		std::string vertPath;
		std::string fragPath;

		PipelineState pipelineState{};
		VkRenderPass compatibleRenderPass = VK_NULL_HANDLE;

		struct DescriptorSetLayout
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			VkDescriptorSetLayout layout;
		};
		std::vector<DescriptorSetLayout> descriptorSetLayouts;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
	};
}


