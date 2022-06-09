//
// Created by raind on 6/9/2022.
//

#pragma once

#include "Scene/AABB.hpp"
#include "SceneObject.hpp"

struct Mesh;
struct BSDF;

class MeshObject : public SceneObject
{
public:
	explicit MeshObject(Mesh* in_mesh);
	~MeshObject() override;

#if GRAPHICS_DISPLAY
	void update(float elapsed) override;
	void draw(VkCommandBuffer cmdbuf) override;
#endif

	void set_local_position(glm::vec3 in_local_position) override;
	void set_rotation(glm::quat in_rotation) override;
	void set_scale(glm::vec3 in_scale) override;

	Mesh* mesh = nullptr;
	BSDF* bsdf = nullptr;
	AABB aabb;

private:
	void generate_aabb();

	bool locked = false;
};

