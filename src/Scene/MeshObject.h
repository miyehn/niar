//
// Created by raind on 6/9/2022.
//

#pragma once

#include "Scene/AABB.hpp"
#include "SceneObject.hpp"
#include "Render/Mesh.h"

struct BSDF;

class MeshObject : public SceneObject
{
public:
	explicit MeshObject(const Mesh &in_mesh);

#if GRAPHICS_DISPLAY
	void update(float elapsed) override;
	void draw(VkCommandBuffer cmdbuf) override;
#endif

	void set_local_position(glm::vec3 in_local_position) override;
	void setRotation(glm::quat in_rotation) override;
	void set_scale(glm::vec3 in_scale) override;

	Mesh mesh = {};
	AABB aabb; // todo [myn]: should probably change to use local space instead
	BSDF* bsdf = nullptr;

private:
	void generate_aabb();

	bool locked = false;
};

