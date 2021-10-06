#include "Render/Vulkan/Vulkan.hpp"
#include "gfx.h"
#include "Engine/Input.hpp"

Vulkan* Vulkan::Instance = nullptr;

namespace gfx
{
	bool init_window(const std::string &name, int width, int height, SDL_Window **window, int *drawable_width,
					 int *drawable_height)
	{
		if (Cfg.TestVulkan)
		{
			SDL_Init(SDL_INIT_VIDEO);

			// create window
			*window = SDL_CreateWindow(
				name.c_str(),
				100, 100, // SDL_WINDOWPOS_UNDEFINED, or SDL_WINDOWPOS_CENTERED
				width, height, // specify window size
				SDL_WINDOW_VULKAN
			);
			if (*window == nullptr)
			{
				ERR("Error creating SDL window: %s", SDL_GetError());
				return false;
			}

			Vulkan::Instance = new Vulkan(*window);

			// TODO: get drawable size (vulkan)?
			*drawable_width = width;
			*drawable_height = height;
		}
		else
		{
			SDL_Init(SDL_INIT_VIDEO);

			// OpenGL settings
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

			// create window
			*window = SDL_CreateWindow(
				name.c_str(),
				100, 100, // SDL_WINDOWPOS_UNDEFINED, or SDL_WINDOWPOS_CENTERED
				width, height, // specify window size
				SDL_WINDOW_OPENGL
			);
			if (*window == nullptr)
			{
				ERR("Error creating SDL window: %s", SDL_GetError());
				return false;
			}

			// store drawable sizes
			SDL_GL_GetDrawableSize(*window, drawable_width, drawable_height);

			// create context
			auto context = SDL_GL_CreateContext(*window);
			if (!context)
			{
				SDL_DestroyWindow(*window);
				ERR("Error creating OpenGL context: %s", SDL_GetError());
				return false;
			}

			// glew setup
			// https://open.gl/context#Onemorething
			glewExperimental = GL_TRUE;
			glewInit();
		}

		return true;
	}

	std::unordered_map<std::string, ShaderModule *> ShaderModule::pool;

	ShaderModule::ShaderModule(const std::string &path)
	{
		if (Cfg.TestVulkan)
		{
			auto code = myn::read_file(path);
			VkShaderModuleCreateInfo createInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = code.size(),
				.pCode = reinterpret_cast<const uint32_t *>(code.data())
			};
			EXPECT(vkCreateShaderModule(Vulkan::Instance->device, &createInfo, nullptr, &module), VK_SUCCESS)
		}
		else
		{
			// TODO
		}
	}

	ShaderModule::~ShaderModule()
	{
		vkDestroyShaderModule(Vulkan::Instance->device, module, nullptr);
	}

	ShaderModule *ShaderModule::get(const std::string &path)
	{
		auto it = ShaderModule::pool.find(path);
		if (it != ShaderModule::pool.end()) return (*it).second;

		auto new_module = new ShaderModule(path);
		ShaderModule::pool[path] = new_module;
		return new_module;
	}

	void ShaderModule::cleanup()
	{
		for (auto it = ShaderModule::pool.begin(); it != ShaderModule::pool.end(); it++)
		{
			delete (*it).second;
		}
	}

	PipelineState::PipelineState(uint32_t width, uint32_t height)
	{
		targetExtent = {.width = width, .height = height};

		useVertexInput = true;
		{
			// about how to cut the binding-th array into strides
			bindingDescription = {
				.binding = 0, // binding index. Just one binding if all vertex data is passed as one array
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // data is per-vertex (as opposed to per-instance)
			};
			// about how to interpret each stride
			attributeDescriptions.push_back(VkVertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = (uint32_t) offsetof(Vertex, position)
			});
			attributeDescriptions.push_back(VkVertexInputAttributeDescription{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = (uint32_t) offsetof(Vertex, normal)
			});
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

		useDepthStencil = false;
		{
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
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
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

	Pipeline::Pipeline()
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

		RenderPassBuilder passBuilder{};
		renderPass = passBuilder.build();

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
	    vkDestroyRenderPass(Vulkan::Instance->device, renderPass, nullptr);
	}

	void Pipeline::use()
	{

	}

	RenderPassBuilder::RenderPassBuilder()
	{
		//-------- Render pass (TODO: abstract away) --------

		// a render pass: load the attachment, r/w operations (by subpasses), then release it?
		// renderpass -> subpass -> attachmentReferences -> attachment
		// a render pass holds ref to an array of color attachments.
		// A subpass then use an array of attachmentRef to selectively get the attachments and use them
		colorAttachment = {
			.format = Vulkan::Instance->swapChainImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout when attachment is loaded
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		colorAttachmentRef = {
			// matches layout(location=X)
			.attachment = 0,
			// "which layout we'd like it to have during this subpass"
			// so, initialLayout -> this -> finalLayout ?
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef // an array, index matches layout (location=X) out vec4 outColor
		};
		dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			// wait for the swap chain to finish reading
			// Question: first access scope includes color_attachment_output as well?? (Isn't it just read by the monitor?)
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			// before we can write to it
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};
	}

	VkRenderPass RenderPassBuilder::build()
	{
		VkRenderPassCreateInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &colorAttachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};
		VkRenderPass renderPass;
		EXPECT(vkCreateRenderPass(Vulkan::Instance->device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS)
		return renderPass;
	}

	PipelineLayoutBuilder::PipelineLayoutBuilder()
	{
		uboLayoutBinding = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
			.pImmutableSamplers = nullptr // for image sampling related?
		};
	}

	VkPipelineLayout PipelineLayoutBuilder::build()
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &uboLayoutBinding,
		};
		EXPECT(vkCreateDescriptorSetLayout(Vulkan::Instance->device, &layoutInfo, nullptr, &descriptorSetLayout), VK_SUCCESS)

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};
		VkPipelineLayout pipelineLayout;
		EXPECT(vkCreatePipelineLayout(Vulkan::Instance->device, &pipelineLayoutInfo, nullptr, &pipelineLayout), VK_SUCCESS)
		return pipelineLayout;
	}
}

