//
// Created by miyehn on 6/1/22.
//

#include "EnvironmentMapAsset.h"
#include "Utils/myn/Log.h"
#include <tinyexr/tinyexr.h>
#if GRAPHICS_DISPLAY
#include "Render/Texture.h"
#endif

EnvironmentMapAsset::EnvironmentMapAsset(
	const std::string &relative_path,
	const std::function<void(const EnvironmentMapAsset *)> &loadAction) : Asset(relative_path, nullptr)
{
	load_action_internal = [this, relative_path, loadAction]() {

		release_resources();

		float* data; // width * height * RGBA
		const char* err;
		int ret = LoadEXR(&data, &width, &height, (ROOT_DIR"/"+relative_path).c_str(), &err);
		if (ret == TINYEXR_SUCCESS) {
			texels.resize(width * height);
			for (int i = 0; i < width * height; i++) {
				texels[i] = glm::vec3(data[4*i], data[4*i+1], data[4*i+2]);
			}
#if GRAPHICS_DISPLAY && 0
			// TODO: debug this on PC
			texture2D = new Texture2D(
				relative_path,
				(uint8_t*)texels.data(),
				width, height,
				{3, 32, 0});
#endif
			free(data);
		} else {
			ERR("load exr '%s' failed: %s", relative_path.c_str(), err)
		}

		if (loadAction) {
			loadAction(this);
		}
	};
	reload();
}

void EnvironmentMapAsset::release_resources() {
#if GRAPHICS_DISPLAY
	delete texture2D;
#endif
	texels.clear();
}

EnvironmentMapAsset::~EnvironmentMapAsset() {
	release_resources();
}
