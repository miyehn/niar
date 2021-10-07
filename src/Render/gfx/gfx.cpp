#include "Render/Vulkan/Vulkan.hpp"
#include "gfx.h"
#include "Engine/Input.hpp"
#include "Asset/Mesh.h"

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
		depthAttachment = {
			.format = Vulkan::Instance->swapChainDepthFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		depthAttachmentRef = {
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
		subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = &depthAttachmentRef
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
		VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 2,
			.pAttachments = attachments,
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

