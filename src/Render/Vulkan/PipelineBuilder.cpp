#include "PipelineBuilder.h"
#include "Render/Vertex.h"
#include "Vulkan.hpp"
#include "ShaderModule.h"

PipelineState::PipelineState()
{
	// to be set manually
	targetExtent = {.width = 0, .height = 0};

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

	if (!useVertexInput)
	{
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	}
	createInfo.pVertexInputState = &vertexInputInfo;

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

void PipelineState::setExtent(uint32_t width, uint32_t height)
{
	targetExtent.width = width; targetExtent.height = height;
	viewport.width = (float)width;
	viewport.height = (float)height;
	scissor.extent = targetExtent;
}

void GraphicsPipelineBuilder::build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout)
{
	// shader stages

	auto vertModule = ShaderModule::get(vertPath);
	auto fragModule = ShaderModule::get(fragPath);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertModule->module,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragModule->module,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// layout

	std::vector<VkDescriptorSetLayout> setLayouts;
	for (auto layout : descriptorSetLayouts)
	{
		setLayouts.push_back(layout.getLayout());
	}
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
		.pSetLayouts = setLayouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	EXPECT(vkCreatePipelineLayout(Vulkan::Instance->device, &pipelineLayoutInfo, nullptr, &outPipelineLayout), VK_SUCCESS)

	// create the fucking pipeline
	auto pipelineInfo = pipelineState.getPipelineInfoTemplate();
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.layout = outPipelineLayout;
	pipelineInfo.renderPass = compatibleRenderPass;
	pipelineInfo.subpass = compatibleSubpass;

	EXPECT(vkCreateGraphicsPipelines(Vulkan::Instance->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline), VK_SUCCESS)

	Vulkan::Instance->destructionQueue.emplace_back([outPipeline, outPipelineLayout](){
		vkDestroyPipeline(Vulkan::Instance->device, outPipeline, nullptr);
		vkDestroyPipelineLayout(Vulkan::Instance->device, outPipelineLayout, nullptr);
	});
}

void GraphicsPipelineBuilder::useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout)
{
	if (descriptorSetLayouts.size() <= setIndex) descriptorSetLayouts.resize(setIndex + 1);
	descriptorSetLayouts[setIndex] = setLayout;
}

void ComputePipelineBuilder::useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout)
{
	if (descriptorSetLayouts.size() <= setIndex) descriptorSetLayouts.resize(setIndex + 1);
	descriptorSetLayouts[setIndex] = setLayout;
}

void ComputePipelineBuilder::build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout)
{
	// shader stage
	auto shaderModule = ShaderModule::get(shaderPath);
	VkPipelineShaderStageCreateInfo shaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shaderModule->module,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};
	// layout
	std::vector<VkDescriptorSetLayout> setLayouts;
	for (auto layout : descriptorSetLayouts)
	{
		setLayouts.push_back(layout.getLayout());
	}
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
		.pSetLayouts = setLayouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	EXPECT(vkCreatePipelineLayout(Vulkan::Instance->device, &pipelineLayoutInfo, nullptr, &outPipelineLayout), VK_SUCCESS)
	// pipeline
	VkComputePipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = shaderStageInfo,
		.layout = outPipelineLayout
	};
	EXPECT(vkCreateComputePipelines(Vulkan::Instance->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline), VK_SUCCESS)

	Vulkan::Instance->destructionQueue.emplace_back([outPipeline, outPipelineLayout](){
		vkDestroyPipeline(Vulkan::Instance->device, outPipeline, nullptr);
		vkDestroyPipelineLayout(Vulkan::Instance->device, outPipelineLayout, nullptr);
	});
}

void RayTracingPipelineBuilder::useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout)
{
	if (descriptorSetLayouts.size() <= setIndex) descriptorSetLayouts.resize(setIndex + 1);
	descriptorSetLayouts[setIndex] = setLayout;
}

void RayTracingPipelineBuilder::build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout)
{
	// shader stages

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	const uint32_t rgenIndex = 0;
	shaderStages.push_back({
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.module = ShaderModule::get(rgenPath)->module,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	});

	const uint32_t rchitStartIndex = 1;
	for (auto& rchitPath : rchitPaths)
	{
		shaderStages.push_back({
		   .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		   .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		   .module = ShaderModule::get(rchitPath)->module,
		   .pName = "main", // entry point function (should be main for glsl shaders)
		   .pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
		});
	}

	const uint32_t rahitStartIndex = rchitStartIndex + rchitPaths.size();
	for (auto& rahitPath : rahitPaths)
	{
		shaderStages.push_back({
		   .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		   .stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
		   .module = ShaderModule::get(rahitPath)->module,
		   .pName = "main", // entry point function (should be main for glsl shaders)
		   .pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
		});
	}

	const uint32_t rmissStartIndex = rahitStartIndex + rahitPaths.size();
	for (auto& rmissPath : rmissPaths)
	{
		shaderStages.push_back({
		   .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		   .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
		   .module = ShaderModule::get(rmissPath)->module,
		   .pName = "main", // entry point function (should be main for glsl shaders)
		   .pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
		});
	}

	// shader groups

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groupInfos;

	// rgen
	groupInfos.push_back({
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = rgenIndex,
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	});

	// hit groups
	for (auto & hitGroup : hitGroups)
	{
		groupInfos.push_back({
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
			.generalShader = VK_SHADER_UNUSED_KHR,
			.closestHitShader = hitGroup.rchitIndex < 0 ?
				VK_SHADER_UNUSED_KHR : hitGroup.rchitIndex + rchitStartIndex,
			.anyHitShader = hitGroup.rahitIndex < 0 ?
				VK_SHADER_UNUSED_KHR : hitGroup.rahitIndex + rahitStartIndex,
			.intersectionShader = VK_SHADER_UNUSED_KHR, // not implemented yet
		});
	}
	// miss groups
	for (auto i = 0; i < rmissPaths.size(); i++)
	{
		groupInfos.push_back({
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = rmissStartIndex + i,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		});
	}

	// layout
	std::vector<VkDescriptorSetLayout> setLayouts;
	for (auto layout : descriptorSetLayouts)
	{
		setLayouts.push_back(layout.getLayout());
	}
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
		.pSetLayouts = setLayouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	EXPECT(vkCreatePipelineLayout(Vulkan::Instance->device, &pipelineLayoutInfo, nullptr, &outPipelineLayout), VK_SUCCESS)

	// pipeline
	VkRayTracingPipelineCreateInfoKHR pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.groupCount = static_cast<uint32_t>(groupInfos.size()),
		.pGroups = groupInfos.data(),
		.maxPipelineRayRecursionDepth = 1,
		.layout = outPipelineLayout
	};
	EXPECT(Vulkan::Instance->fn_vkCreateRayTracingPipelinesKHR(Vulkan::Instance->device, {}, {}, 1, &pipelineInfo, nullptr, &outPipeline), VK_SUCCESS)

	// add to cleanup queue
	Vulkan::Instance->destructionQueue.emplace_back([outPipeline, outPipelineLayout](){
		vkDestroyPipeline(Vulkan::Instance->device, outPipeline, nullptr);
		vkDestroyPipelineLayout(Vulkan::Instance->device, outPipelineLayout, nullptr);
	});
}
