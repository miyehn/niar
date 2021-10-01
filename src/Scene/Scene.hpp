#pragma once

#include "Engine/Drawable.hpp"
#include "Utils/Utils.hpp"

#define MAX_SHADOWCASTING_LIGHTS 6
#define NUM_GBUFFERS 4

struct Light;
struct DirectionalLight;
struct PointLight;
struct Mesh;
struct Material;

/* a scene is a tree of drawables */
struct Scene : public Drawable {

	static Scene* Active;

	Scene(std::string _name = "[unnamed scene]");
	virtual ~Scene();

	int w, h;

	void initialize_graphics();
	void load(std::string source, bool preserve_existing_objects = false);
	std::vector<DirectionalLight*> d_lights;
	std::vector<PointLight*> p_lights;

	uint material_set = 1;

	//---- Rendering-related textures, buffers, shaders, etc. ----
	uint fbo_gbuffers = 0;
	uint color_attachments_gbuffers[NUM_GBUFFERS];
	uint tex_gbuffers[NUM_GBUFFERS];

	uint fbo_position_lights = 0;
	uint color_attachments_position_lights[MAX_SHADOWCASTING_LIGHTS];
	uint depthbuf_position_lights = 0;
	uint tex_depth = 0;

	// raw hdr linear space color output
	uint fbo_scene_color = 0;
	uint color_attachments_scene_color = 0;
	uint tex_scene_color = 0;

	// exposure-adjusted & extracted bright region
	uint fbo_scene_color_alt = 0;
	uint color_attachments_scene_color_alt[2];
	uint tex_scene_colors_alt[2];

	// ping pong textures for bloom
	uint fbo_gaussian_pingpong[2];
	uint tex_gaussian_pingpong[2];

	Material* replacement_material = nullptr;

	//-------- OpenGL configurations --------
	// depth test
	bool use_depth_test;
	// culling
	bool cull_face;
	GLenum cull_mode;
	// fill / wireframe (/ point)
	GLenum fill_effective_polygon; // GL_FRONT_AND_BACK | GL_BACK | GL_FRONT
	GLenum fill_mode; // GL_FILL | GL_LINE | GL_POINT

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

