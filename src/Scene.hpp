#pragma once

#include "Drawable.hpp"

/* a scene is a tree of drawables */
struct Scene : public Drawable {
  Scene(Drawable* _root = nullptr, std::string _name = "[unnamed scene]");
  ~Scene();

  Drawable* root = nullptr;

  virtual bool handle_event(SDL_Event event);
  virtual void update(float elapsed);
  virtual void draw();
};
