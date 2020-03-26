#pragma once
#include "Drawable.hpp"

struct Vertex {
  vec3 position;
  vec3 normal;
  vec4 color;
};
static_assert(sizeof(Vertex) == sizeof(float) * (3 + 3 + 4), "vertex struct should be packed");

struct Mesh : public Drawable {
  
  // not sure what to do with this constructor...
  Mesh(aiMesh* mesh = nullptr, Drawable* _parent = nullptr, std::string _name = "[unnamed mesh]");
  virtual ~Mesh();

  virtual bool handle_event(SDL_Event event);
  virtual void update(float elapsed);
  virtual void draw();

  std::vector<Vertex> vertices;
  std::vector<uint> faces;

};
