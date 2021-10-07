#pragma once
#include "Utils/lib.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include <unordered_map>

namespace gfx
{
	bool init_window(const std::string& name, int width, int height, SDL_Window** window, int* drawable_width = nullptr, int* drawable_height = nullptr);

	class ShaderModule
	{
	public:
		static ShaderModule* get(const std::string &path);
		static void cleanup();

		VkShaderModule module;
	private:
		explicit ShaderModule(const std::string &path);
		~ShaderModule();
		static std::unordered_map<std::string, ShaderModule*> pool;
	};

}// namespace gfx