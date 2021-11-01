#pragma once

#include "Engine/Drawable.hpp"
#include "Utils/Utils.hpp"

struct Light;
struct DirectionalLight;
struct PointLight;
struct Mesh;
struct GlMaterial;

/* a scene is a tree of drawables */
struct Scene : public Drawable {

	static Scene* Active;

	Scene(const std::string &_name = "[unnamed scene]");
	virtual ~Scene();

	void load(const std::string& path, bool preserve_existing_objects = true);

	//-------- where the configurations are being set before the scene is drawn --------
	virtual void draw();

	AABB aabb;
	std::vector<Mesh*> get_meshes();
	void generate_aabb();
	void draw_content(bool shadow_pass = false);

private:

	virtual void set_local_position(vec3 _local_position) {}
	virtual void set_rotation(quat _rotation) {}
	virtual void set_scale(vec3 _scale) {}
	
};

