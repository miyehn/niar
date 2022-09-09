//
// Created by raindu on 6/14/2022.
//
#pragma once

#include "SceneObject.hpp"

class ProbeMaterial;

class Probe : public SceneObject {
public:
	Probe();

#if GRAPHICS_DISPLAY
	static Material* get_material();
	void draw(VkCommandBuffer cmdbuf) override;
#endif
};

class EnvMapVisualizer : public Probe {
public:
	EnvMapVisualizer();
#if GRAPHICS_DISPLAY
	void update(float elapsed) override;
#endif
};