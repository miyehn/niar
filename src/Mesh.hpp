#pragma once
#include "Drawable.hpp"

struct Vertex {
  Vertex() {}
  Vertex(vec3 _position) : position(_position) {}
  vec3 position = vec3(0, 0, 0);
  vec3 normal = vec3(0, 0, 1);
  vec4 color = vec4(1, 1, 1, 1);
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
  uint get_num_triangles() { return faces.size() / 3; }

  //---- opengl stuff ----
  uint shader, vbo, ebo, vao = 0;

};
