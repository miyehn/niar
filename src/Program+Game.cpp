
#include "Program.hpp"

using namespace std;
using namespace glm;

void Program::setup() { // TODO: should move this all into specifig game objects
  cout << "hello from SDL." << endl;
  
  shader = newShaderProgram(
                            "../shaders/grass.vert",
                            "../shaders/grass.tesc",
                            "../shaders/grass.tese",
                            "../shaders/grass.frag");
  glUseProgram(shader);
  
  vao = newVAO(vertices, 24);
  setVertexAtrribPointers();
  
  glPatchParameteri(GL_PATCH_VERTICES, 1);
  
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile("../media/blueCube.fbx",
          aiProcess_Triangulate);
  if (!scene) {
    cout << importer.GetErrorString();
  }

  camera = Camera();
  camera.aspect_ratio = aspectRatio();

  cout << "done with setup." << endl;
}

void Program::draw() {

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
  
  mat4 transformation = camera.obj_to_screen(vec3(0.0f, 0.0f, 0.0f));
  glUniformMatrix4fv(unifLoc(shader, "transformation"), 1, GL_FALSE, value_ptr(transformation));
  glDrawArrays(GL_PATCHES, 0, 6);
    
}

void Program::setVertexAtrribPointers() {
    // set attrib pointer for position.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
}

