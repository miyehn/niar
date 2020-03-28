#pragma once

#include "Drawable.hpp"

struct Cube : Drawable {
  
  Cube(
      Drawable* _parent = nullptr, 
      std::string _name = "cube");
  virtual ~Cube();

  // inherited
  virtual bool handle_event(SDL_Event event);
  virtual void update(float elapsed);
  virtual void draw();

  // properties and methods
  const aiScene* scene;
  
  // material / shader
  uint shader;
  uint vbo;
  uint vao;
};

