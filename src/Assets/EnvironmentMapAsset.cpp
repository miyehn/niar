//
// Created by miyehn on 6/1/22.
//

#include "EnvironmentMapAsset.h"
#include "Utils/myn/Log.h"
#include <tinyexr/tinyexr.h>
#if GRAPHICS_DISPLAY
#include "Render/Texture.h"
#include "Render/Vulkan/VulkanUtils.h"
#endif

EnvironmentMapAsset::EnvironmentMapAsset(
	const std::string &relative_path) : Asset(relative_path, nullptr)
{
	load_action_internal = [this, relative_path]() {

		release_resources();

		float* data4x32; // width * height * RGBA
		const char* err;
		int ret = LoadEXR(&data4x32, &width, &height, (ROOT_DIR"/" + relative_path).c_str(), &err);
		if (ret == TINYEXR_SUCCESS) {
			texels3x32.resize(width * height);
			for (int i = 0; i < width * height; i++) {
				texels3x32[i] = glm::vec3(data4x32[4 * i], data4x32[4 * i + 1], data4x32[4 * i + 2]);
			}
#if GRAPHICS_DISPLAY
			// TODO: use in rasterizer
			texture2D = new Texture2D(
				relative_path,
				(uint8_t*)data4x32,
				width, height,
				{4, 32, 0},
				true);
			NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, texture2D->resource.image, "Environment map")
#endif
			free(data4x32);
		} else {
			ERR("load exr '%s' failed: %s", relative_path.c_str(), err)
		}
	};
	reload();
}

void EnvironmentMapAsset::release_resources() {
#if GRAPHICS_DISPLAY
	delete texture2D;
	texture2D = nullptr;
#endif
	texels3x32.clear();
	Asset::release_resources();
}