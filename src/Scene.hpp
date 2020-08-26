#pragma once

#include "Drawable.hpp"

#define MAX_SHADOWCASTING_LIGHTS 6
#define NUM_GBUFFERS 3

struct Light;
struct DirectionalLight;
struct PointLight;
struct Mesh;

/* a scene is a tree of drawables */
struct Scene : public Drawable {

	static Scene* Active;

  Scene(std::string _name = "[unnamed scene]");
	virtual ~Scene();

	int w, h;

	void load(std::string source, bool preserve_existing_objects = false);
	std::vector<DirectionalLight*> d_lights;
	std::vector<PointLight*> p_lights;

	uint shader_set = 0;

	//---- Rendering-related textures, buffers, shaders, etc. ----
	uint fbo_gbuffers = 0;
	uint color_attachments_gbuffers[NUM_GBUFFERS];
	uint tex_gbuffers[NUM_GBUFFERS];

	uint fbo_position_lights = 0;
	uint color_attachments_position_lights[MAX_SHADOWCASTING_LIGHTS];
	uint depthbuf_position_lights = 0;
	uint tex_depth = 0;

	Blit* lighting_directional = nullptr;
	Blit* lighting_point = nullptr;

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

	std::vector<Mesh*> get_meshes();
	void draw_content(bool shadow_pass = false);

private:
	void pass_directional_lights_to_lighting_shader();
	void pass_point_lights_to_lighting_shader();
  
};

