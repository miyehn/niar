#include "Scene.hpp"

Scene::Scene(std::string _name) : Drawable(nullptr, _name) {
  // depth test
  use_depth_test = true;

  // culling
  cull_face = false;
  cull_mode = GL_BACK;

  // blending: "blend the computed fragment color values with the values in the color buffers."
  blend = false; // NOTE: see no reason why this should be enabled for 3D scenes 

  // fill / wireframe (/ point)
  fill_mode = GL_FILL; // GL_FILL | GL_LINE | GL_POINT
  
}

void Scene::draw() {

  // depth test
  if (use_depth_test) glEnable(GL_DEPTH_TEST);
  else glDisable(GL_DEPTH_TEST);

  // culling
  if (cull_face) {
    glEnable(GL_CULL_FACE);
    glCullFace(cull_mode);
  } else {
    glDisable(GL_CULL_FACE);
  }

  // blending
  if (blend) glEnable(GL_BLEND);
  else glDisable(GL_BLEND);

  // fill mode
  glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

  // actually draw objects
  Drawable::draw();
}
