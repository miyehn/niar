#pragma once

#include "Drawable.hpp"

struct Blade{
  Blade(vec3 root);
  vec4 root_w; // v0, width
  vec4 above_h; // v1, height
  vec4 ctrl_s; // v2, stiffness
  vec4 up_o; // a unit vector up, orientation
};
static_assert(sizeof(Blade) == 16 * sizeof(float), "Blade should be packed");

struct GrassField : public Drawable {
  
  GrassField(uint num_blades, Drawable* _parent = nullptr, std::string _name = "grass");
  virtual ~GrassField();

  // inherited
  virtual void update(float elapsed);
  virtual bool handle_event(SDL_Event event);
  virtual void draw();

  // properties and methods
  std::vector<Blade> blades;

  uint shader;
  uint vbo;
  uint vao;
};

