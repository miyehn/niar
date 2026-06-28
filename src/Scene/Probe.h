//
// Created by raindu on 6/14/2022.
//
#pragma once

#include "SceneObject.hpp"

class Probe : public SceneObject {
public:
	Probe();

#if GRAPHICS_DISPLAY
	static VkPipelineLayout bind_envmap_visualization_pipeline(VkCommandBuffer cmdbuf);
	void set_envmap_visualization_draw_params(VkCommandBuffer cmdbuf);
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