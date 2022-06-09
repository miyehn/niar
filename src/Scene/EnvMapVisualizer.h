//
// Created by raindu on 6/14/2022.
//
#pragma once

#include "SceneObject.hpp"

class ProbeMaterial;

class EnvMapVisualizer : public SceneObject {
public:
	EnvMapVisualizer();

#if GRAPHICS_DISPLAY
	static Material* get_material();
	void draw(VkCommandBuffer cmdbuf) override;
#endif
};

