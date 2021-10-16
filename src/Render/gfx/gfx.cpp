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


}

