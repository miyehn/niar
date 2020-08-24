#pragma once
#include "Updatable.hpp"
#include "Shader.hpp"

#define NUM_SHADER_SETS 6

struct Scene;

struct Drawable: public Updatable {

  Drawable(
      Drawable* _parent = nullptr, 
      std::string _name = "[unnamed drawable]");
  virtual ~Drawable();

  // inherited
  virtual bool handle_event(SDL_Event event);
  virtual void update(float elapsed);

  // draw function
  virtual void draw();

  // material
  Shader shaders[NUM_SHADER_SETS];

  // hierarchy
  Drawable* parent;
  std::vector<Drawable*> children = std::vector<Drawable*>();
	Scene* get_scene();
  virtual bool add_child(Drawable* child);

  // transformation
  mat4 object_to_parent();
  mat4 object_to_world();
  mat4 parent_to_object();
  mat4 world_to_object();
	
	mat3 object_to_world_rotation();
	mat3 world_to_object_rotation();

  vec3 world_position();

  vec3 local_position;
  quat rotation;
  vec3 scale;

};
