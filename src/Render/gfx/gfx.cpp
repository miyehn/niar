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

