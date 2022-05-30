#pragma once

#include "Engine/SceneObject.hpp"
#include "AABB.hpp"

struct Light;
struct DirectionalLight;
struct PointLight;
struct Mesh;

/* a scene is a tree of drawables */
class Scene : public SceneObject {
public:

	static Scene* Active;

	explicit Scene(const std::string &_name = "[unnamed scene]");

	//-------- where the configurations are being set before the scene is drawn --------

	AABB aabb;
	std::vector<Mesh*> get_meshes();
	void generate_aabb();

private:

	void set_local_position(glm::vec3 _local_position) override {}
	void set_rotation(glm::quat _rotation) override {}
	void set_scale(glm::vec3 _scale) override {}
	
};

