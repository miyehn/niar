#include "Mesh.hpp"
#include "utils.hpp"

Mesh::Mesh(aiMesh* mesh, Drawable* _parent, std::string _name) : Drawable(_parent, _name) {

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
  LOG("created a mesh of " + std::to_string(vertices.size()) + " vertices");

}

Mesh::~Mesh() {
}

bool Mesh::handle_event(SDL_Event event) {
  return Drawable::handle_event(event);
}

void Mesh::update(float elapsed) {
  Drawable::update(elapsed);
}

void Mesh::draw() {
  Drawable::draw();
}
