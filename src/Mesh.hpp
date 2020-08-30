#pragma once
#include "Drawable.hpp"
#include "Utils.hpp"

#define NUM_MATERIAL_SETS 2

struct BSDF;
struct aiMesh;
struct Texture;
struct Material;

struct Vertex {
  Vertex() {}
  Vertex(vec3 _position) : position(_position) {}
  vec3 position = vec3(0, 0, 0);
  vec3 normal = vec3(0, 0, 1);
  vec2 uv = vec2(0.5f, 0.5f);
};
static_assert(sizeof(Vertex) == sizeof(float) * (3 + 3 + 2), "vertex struct should be packed");

struct Mesh : Drawable {

  static std::vector<Mesh*> LoadMeshes(const std::string& source);
  
  Mesh(
      aiMesh* mesh = nullptr, 
      Drawable* _parent = nullptr, 
      std::string _name = "[unnamed mesh]");
  virtual ~Mesh();

  virtual bool handle_event(SDL_Event event);
  virtual void update(float elapsed);
  virtual void draw();

	virtual void set_local_position(vec3 _local_position);
	virtual void set_rotation(quat _rotation);
	virtual void set_scale(vec3 _scale);

  std::vector<Vertex> vertices;
  std::vector<uint> faces;
  uint get_num_triangles() { return faces.size() / 3; }

	AABB aabb;
	BSDF* bsdf = nullptr;

	Texture* texture = nullptr;
	Material* materials[NUM_MATERIAL_SETS];

private:

	bool is_thin_mesh;
	bool locked = false;
	void generate_aabb();

  //---- opengl stuff ----
  uint vbo, ebo, vao = 0;

};
