#pragma once

#include "Drawable.hpp"

#define NUM_GBUFFERS 3

struct Light;

/* a scene is a tree of drawables */
struct Scene : public Drawable {

	static Scene* Active;

  Scene(std::string _name = "[unnamed scene]");

	std::vector<Light*> lights;

	//---- Rendering-related textures, buffers, shaders, etc. ----
	uint fbo_gbuffers = 0;
	uint tex_gbuffers[NUM_GBUFFERS];
	uint buf_depth = 0;

	Blit* composition = nullptr;

  //-------- OpenGL configurations --------
  // depth test
  bool use_depth_test;
  // culling
  bool cull_face;
  GLenum cull_mode;
  // blending: "blend the computed fragment color values with the values in the color buffers."
  bool blend; // NOTE: see no reason why this should be enabled for 3D scenes 
  // fill / wireframe (/ point)
  GLenum fill_effective_polygon; // GL_FRONT_AND_BACK | GL_BACK | GL_FRONT
  GLenum fill_mode; // GL_FILL | GL_LINE | GL_POINT

  //-------- where the configurations are being set before the scene is drawn --------
  virtual void draw();

private:
	void pass_lights_to_composition_shader();
  
};

