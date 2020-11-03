#pragma once
#include "Drawable.hpp"
#include "Utils.hpp"

#define NUM_MATERIAL_SETS 3

struct BSDF;
struct aiMesh;
struct Texture;
struct Material;

struct Vertex {
  Vertex() {}
  Vertex(vec3 _position) : position(_position) {}
  vec3 position = vec3(0, 0, 0);
  vec3 normal = vec3(0, 0, 1);
  vec3 tangent = vec3(1, 0, 0);
  vec2 uv = vec2(0.5f, 0.5f);
};
static_assert(sizeof(Vertex) == sizeof(float) * (3 + 3 + 3 + 2), "vertex struct should be packed");

struct Mesh : Drawable {

  static std::vector<Mesh*> LoadMeshes(const std::string& source, bool initialize_graphics = true);
  
  Mesh(
      aiMesh* mesh, 
      Drawable* _parent = nullptr, 
      std::string _name = "[unnamed mesh]");
	Mesh();
	void initialize();
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

	Material* materials[NUM_MATERIAL_SETS];

  static void set_material_name_for(const std::string& mesh_name, const std::string& mat_name);

private:

  static std::string get_material_name_for(const std::string& mesh_name);
  static std::unordered_map<std::string, std::string> material_assignment;

	bool is_thin_mesh;
	bool locked = false;
	void generate_aabb();

  //---- opengl stuff ----
  uint vbo, ebo, vao = 0;

};
