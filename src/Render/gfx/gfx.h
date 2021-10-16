#pragma once
#include "Utils/lib.h"
#include "Render/Vulkan/PipelineBuilder.h"
#include <unordered_map>

namespace gfx
{
	bool init_window(const std::string& name, int width, int height, SDL_Window** window, int* drawable_width = nullptr, int* drawable_height = nullptr);

}// namespace gfx