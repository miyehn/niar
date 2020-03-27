#pragma once

#include "Drawable.hpp"

/* a scene is a tree of drawables */
struct Scene : public Drawable {

  Scene(std::string _name = "[unnamed scene]");

  //-------- OpenGL configurations --------

  // depth test
  bool use_depth_test = true;

  // culling
  bool cull_face = true;
  GLenum cull_mode = GL_BACK;

  // blending: "blend the computed fragment color values with the values in the color buffers."
  bool blend = false; // NOTE: see no reason why this should be enabled for 3D scenes 

  // fill / wireframe (/ point)
  GLenum fill_effective_polygon = GL_FRONT_AND_BACK; // GL_FRONT_AND_BACK | GL_BACK | GL_FRONT
  GLenum fill_mode = GL_FILL; // GL_FILL | GL_LINE | GL_POINT

  //-------- where the configurations are being set before the scene is drawn --------
  virtual void draw();
  
};

