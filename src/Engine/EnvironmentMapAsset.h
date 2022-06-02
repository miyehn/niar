//
// Created by miyehn on 6/1/22.
//

#pragma once

#include "Asset.h"
#include <glm/glm.hpp>
#include <vector>

class Texture2D;

/*
 * reload is not supported because:
 * 1) reload automatically happens when path tracer config is reloaded
 * 2) why would I edit an exr file
 */
class EnvironmentMapAsset : public Asset
{
public:
	explicit EnvironmentMapAsset(
		const std::string& relative_path,
		const std::function<void(const EnvironmentMapAsset *envmap)> &loadAction);
	~EnvironmentMapAsset() override;

	void release_resources();

	int width = 0;
	int height = 0;

#if GRAPHICS_DISPLAY
	Texture2D* texture2D = nullptr;
#endif

	std::vector<glm::vec3> texels;

private:
	bool initialized = false;
};
