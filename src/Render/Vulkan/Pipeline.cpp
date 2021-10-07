#include "Pipeline.h"
#include "Asset/Vertex.h"
#include "Render/gfx/gfx.h"
#include "Render/Vulkan/RenderPassBuilder.h"

namespace gfx
{
	PipelineState::PipelineState(uint32_t width, uint32_t height)
	{
		targetExtent = {.width = width, .height = height};

		useVertexInput = true;
		{
			// about how to cut the binding-th array into strides
			Vertex::getBindingDescription(bindingDescription);
			// about how to interpret each stride
			Vertex::getAttributeDescriptions(attributeDescriptions);

			vertexInputInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = 1,
				.pVertexBindingDescriptions = &bindingDescription,
				.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
				.pVertexAttributeDescriptions = attributeDescriptions.data()
			};
		}

		useInputAssembly = true;
		{
			inputAssemblyInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.primitiveRestartEnable = VK_FALSE
			};
		}

		useViewport = true;
		{
			// may result in transformation of image (which portion of the image content should be rendered?)
			viewport = {
				.x = 0.0f, .y = 0.0f,
				.width = (float) targetExtent.width, .height = (float) targetExtent.height,
				.minDepth = 0.0f, .maxDepth = 1.0f
			};
			// no transformation but clips out everything outside
			scissor = {
				.offset = {0, 0},
				.extent = targetExtent
			};
			viewportInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = 1,
				.pViewports = &viewport,
				.scissorCount = 1,
				.pScissors = &scissor
			};
		}

		useRasterization = true;
		{
			rasterizationInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE, // don't cull depth outside but clamp to near and far (used in shadowmapping?)
				.rasterizerDiscardEnable = VK_FALSE, // discard everything
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_BACK_BIT,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE, // may be useful for shadowmapping
				.depthBiasConstantFactor = 0.0f,
				.depthBiasClamp = 0.0f,
				.depthBiasSlopeFactor = 0.0,
				.lineWidth = 1.0f,
			};
		}

		useMultisampling = true;
		{
			multisamplingInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				.sampleShadingEnable = VK_FALSE,
				.minSampleShading = 1.0f,
				.pSampleMask = nullptr,
				.alphaToCoverageEnable = VK_FALSE,
				.alphaToOneEnable = VK_FALSE
			};
		}

		useDepthStencil = true;
		{
			depthStencilInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_TRUE,
				.depthWriteEnable = VK_TRUE,
				.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = VK_FALSE,
				.front = {},
				.back = {},
				.minDepthBounds = 0.0f,
				.maxDepthBounds = 1.0f,
			};
		}

		useColorBlending = true;
		{
			// this struct is for per framebuffer
			colorBlendAttachmentInfo = {
				.blendEnable = VK_FALSE,
#if 1 // blending off
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				// is it so? also see unity shader lab: https://docs.unity3d.com/Manual/SL-Blend.html
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
#else // alpha blending
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD
#endif
				.colorWriteMask =
				VK_COLOR_COMPONENT_R_BIT
				| VK_COLOR_COMPONENT_G_BIT
				| VK_COLOR_COMPONENT_B_BIT
				| VK_COLOR_COMPONENT_A_BIT,
			};

			// this struct for global blend setting
			colorBlendInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				// if set to true, disables color blending specified in colorBlendAttachment and use bitwise blend instead.
				.logicOpEnable = VK_FALSE,
				.logicOp = VK_LOGIC_OP_COPY,
				.attachmentCount = 1,
				.pAttachments = &colorBlendAttachmentInfo,
				.blendConstants = {.0f, .0f, .0f, .0f}
			};
		}

		useDynamicState = false;
		{
			// VkDynamicState dynamicStates[] = {
			// VK_DYNAMIC_STATE_VIEWPORT,
			// VK_DYNAMIC_STATE_LINE_WIDTH
			// };
			dynamicStateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = 0,
				.pDynamicStates = nullptr
			};
		}
	}

	VkGraphicsPipelineCreateInfo PipelineState::getPipelineInfoTemplate()
	{
		VkGraphicsPipelineCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.pVertexInputState = useVertexInput ? &vertexInputInfo : nullptr;
		createInfo.pInputAssemblyState = useInputAssembly ? &inputAssemblyInfo : nullptr;
		createInfo.pViewportState = useViewport ? &viewportInfo : nullptr;
		createInfo.pRasterizationState = useRasterization ? &rasterizationInfo : nullptr;
		createInfo.pMultisampleState = useMultisampling ? &multisamplingInfo : nullptr;
		createInfo.pDepthStencilState = useDepthStencil ? &depthStencilInfo : nullptr;
		createInfo.pColorBlendState = useColorBlending ? &colorBlendInfo : nullptr;
		createInfo.pDynamicState = useDynamicState ? &dynamicStateInfo : nullptr;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;
		return createInfo;
	}

	Pipeline::Pipeline(VkRenderPass renderPass)
	{
		auto vert_module = ShaderModule::get("spirv/triangle.vert.spv");
		auto frag_module = ShaderModule::get("spirv/triangle.frag.spv");

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vert_module->module,
			.pName = "main", // entry point function (should be main for glsl shaders)
			.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
		};

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = frag_module->module,
			.pName = "main", // entry point function (should be main for glsl shaders)
			.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		PipelineLayoutBuilder layoutBuilder{};
		pipelineLayout = layoutBuilder.build();
		descriptorSetLayout = layoutBuilder.descriptorSetLayout;

		// create the fucking pipeline
		PipelineState pipelineState(Vulkan::Instance->swapChainExtent.width, Vulkan::Instance->swapChainExtent.height);
		auto pipelineInfo = pipelineState.getPipelineInfoTemplate();
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		EXPECT(vkCreateGraphicsPipelines(Vulkan::Instance->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), VK_SUCCESS)
	}

	Pipeline::~Pipeline()
	{
		vkDestroyDescriptorSetLayout(Vulkan::Instance->device, descriptorSetLayout, nullptr);
		vkDestroyPipeline(Vulkan::Instance->device, pipeline, nullptr);
		vkDestroyPipelineLayout(Vulkan::Instance->device, pipelineLayout, nullptr);
	}

	void Pipeline::use()
	{

	}
}