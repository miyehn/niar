#pragma once
#include "Updatable.hpp"

struct Camera;

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

  // hierarchy
  Drawable* parent;
  std::vector<Drawable*> children = std::vector<Drawable*>();
  virtual bool add_child(Drawable* child);

  // transformation
  mat4 object_to_parent();
  mat4 object_to_world();
  mat4 parent_to_object();
  mat4 world_to_object();
  vec3 world_position();

  vec3 local_position;
  quat rotation;
  vec3 scale;

};
