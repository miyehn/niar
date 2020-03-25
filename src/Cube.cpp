#include "Cube.hpp"

Cube::Cube(Camera* camera): Drawable(camera) {
  Assimp::Importer importer;
  scene = importer.ReadFile("../media/longcube.fbx",
      aiProcess_GenSmoothNormals |
      aiProcess_CalcTangentSpace |
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_SortByPType);
  if (!scene) {
    ERR(importer.GetErrorString());
  }

  //---- let's navigate through this scene a bit...
  // sometimes fail I have no idea why
  for (int i=0; i<scene->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[i];
    LOG(mesh->mNumFaces);
    LOG("vertices: ");
    for (int j=0; j<mesh->mNumVertices; j++) {
      aiVector3D vert = mesh->mVertices[j];
      std::cout << vert.x << ", " << vert.y << ", " << vert.z;
      if (mesh->HasNormals()) {
        aiVector3D n = mesh->mNormals[j];
        std::cout << " | " << n.x << ", " << n.y << ", " << n.z;
      }

      std::cout << std::endl;
    }

    LOG("faces: ");
    for (int j=0; j<mesh->mNumFaces; j++) {
      aiFace face = mesh->mFaces[j];
      std::cout << face.mIndices[0] << " " << face.mIndices[1] << " " << face.mIndices[2] << std::endl;
    }

    LOG("vertex colors");
    int channel = -1;
    for (int j=0; j<AI_MAX_NUMBER_OF_COLOR_SETS; j++) {
      LOG(mesh->HasVertexColors(j));
      if (channel == -1 && mesh->HasVertexColors(j)) channel = j;
    }

    aiColor4D* vertex_colors = mesh->mColors[channel];
    for (int j=0; j<mesh->mNumVertices; j++) {
      aiColor4D color = vertex_colors[j];
      std::cout << color.r << ", " << color.g << ", " << color.b << ", " << color.a << std::endl;
    }

  }

}

Cube::~Cube() {
}

void Cube::update(float time_elapsed) {
}

bool Cube::handle_event(SDL_Event event) {
  return false;
}

void Cube::draw() {
}
