#include "Mesh.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "BSDF.hpp"
#include "Globals.hpp"
#include "Scene.hpp"
#include "Light.hpp"

Mesh::Mesh(aiMesh* mesh, bool y_up, Drawable* _parent, std::string _name) : Drawable(_parent, _name) {

  if (!mesh->HasPositions() || !mesh->HasFaces() || !mesh->HasNormals()) {
    ERR("creating a mesh that has some data missing.");
    return;
  }

  int first_color_channel = -1;
  for (int i=0; i<AI_MAX_NUMBER_OF_COLOR_SETS; i++) {
    if (first_color_channel == -1 && mesh->HasVertexColors(i)) first_color_channel = i;
  }
  if (first_color_channel < 0) {
    WARN("creating a mesh without vertex color. using white instead.");
  }

  // iterate through vertices to get p, n, c
  for (int i=0; i<mesh->mNumVertices; i++) {
    // access p, n, c data
    aiVector3D position = mesh->mVertices[i];
    aiVector3D normal = mesh->mNormals[i];
    aiColor4D color = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);
    if (first_color_channel >= 0) {
      color = mesh->mColors[first_color_channel][i];
    }
    // create vertex from pnc
    Vertex v;
    v.position = vec3(position.x, position.y, position.z);
    v.normal = vec3(normal.x, normal.y, normal.z);
    v.color = vec4(color.r, color.g, color.b, color.a);
		if (y_up) {
			mat3 y_up_to_z_up = mat3(
					vec3(1, 0, 0),
					vec3(0, 0, 1),
					vec3(0, -1, 0));
			v.position = y_up_to_z_up * v.position;
			v.normal = y_up_to_z_up * v.normal;
		}
    // push into vertex array
    vertices.push_back(v);
  }

  // iterate through faces indices and store them
  for (int j=0; j<mesh->mNumFaces; j++) {
    aiFace face = mesh->mFaces[j];
    faces.push_back(face.mIndices[0]);
    faces.push_back(face.mIndices[1]);
    faces.push_back(face.mIndices[2]);
  }
  LOGF("loaded a mesh of %d vertices and %d triangles", vertices.size(), get_num_triangles());

  //---- OpenGL setup ----
  
  // copy construct a default shader
  shaders[0] = Shader::DeferredBasePass;
	shaders[1] = Shader::DepthOnly;
	shaders[2] = Shader::Basic;
	shaders[3] = Shader::ShadowPassDirectional;
	shaders[4] = Shader::ShadowPassPoint;
	shaders[5] = Shader::Distance;

  // generate buffers & objects
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);
  {
    //---- set buffers and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * faces.size(), faces.data(), GL_STATIC_DRAW);

    //---- set vertex attrib pointsers
    glVertexAttribPointer(
        0, // attrib index
        3, // num of data elems
        GL_FLOAT, // data type
        GL_FALSE, // normalized
        sizeof(Vertex), // stride size
        (void*)offsetof(Vertex, position)); // offset from stride start
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, // attrib index
        3, // num of data elems
        GL_FLOAT, // data type
        GL_FALSE, // normalized
        sizeof(Vertex), // stride size
        (void*)offsetof(Vertex, normal)); // offset from stride start
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2, // attrib index
        4, // num of data elems
        GL_FLOAT, // data type
        GL_FALSE, // normalized
        sizeof(Vertex), // stride size
        (void*)offsetof(Vertex, color)); // offset from stride start
    glEnableVertexAttribArray(2);
  }
  glBindVertexArray(0);

  //---- shader params ----
	
	set_all_shader_param_funcs();
}

Mesh::~Mesh() {
	if (bsdf) delete bsdf;
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
  glDeleteVertexArrays(1, &vao);
}

bool Mesh::handle_event(SDL_Event event) {
  return Drawable::handle_event(event);
}

void Mesh::update(float elapsed) {
  Drawable::update(elapsed);
}

void Mesh::draw() {

  // set shader
	Shader& shader = shaders[get_scene()->shader_set];
	if (shader.id == 0) return;

  glUseProgram(shader.id);

  // upload uniform
  shader.set_parameters();

  // bind vao and draw
  glBindVertexArray(vao);
	//GL_ERRORS();
  glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
	//GL_ERRORS();
  glBindVertexArray(0);

  glUseProgram(0);

  // draw children
  Drawable::draw();
}

std::vector<Mesh*> Mesh::LoadMeshes(const std::string& source, bool y_up) {
  // import mesh from source
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(source,
      aiProcess_GenSmoothNormals |
      aiProcess_CalcTangentSpace |
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_SortByPType);
  if (!scene) {
    ERR(importer.GetErrorString());
  }

  // access and create mesh drawables from imported source
  std::vector<Mesh*> meshes;
  for (int i=0; i<scene->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[i];
    if (mesh) meshes.push_back(new Mesh(mesh, y_up));
  }

  LOGF("loaded %d meshe(s)", meshes.size());

  // importer seems to automatically handle memory release for scene
  return meshes;

}

void Mesh::set_all_shader_param_funcs() {
	shaders[0].set_parameters = [this]() {
		shaders[0].set_mat3("OBJECT_TO_WORLD_ROT", object_to_world_rotation());
		mat4 o2w = object_to_world();
		shaders[0].set_mat4("OBJECT_TO_WORLD", o2w);
		shaders[0].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
	shaders[1].set_parameters = [this]() {
		mat4 o2w = object_to_world();
		shaders[1].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
	shaders[2].set_parameters = [this]() {
		shaders[2].set_mat3("OBJECT_TO_CAM_ROT", 
				object_to_world_rotation() * Camera::Active->world_to_camera_rotation());
		mat4 o2w = object_to_world();
		shaders[2].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
	shaders[3].set_parameters = [this]() {
		Light::set_directional_shadowpass_params_for_mesh(this, 3);
	};
	shaders[4].set_parameters = [this]() {
		Light::set_point_shadowpass_params_for_mesh(this, 4);
	};
	shaders[5].set_parameters = [this]() {
		mat4 o2w = object_to_world();
		shaders[5].set_mat4("OBJECT_TO_WORLD", o2w);
		shaders[5].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
}
