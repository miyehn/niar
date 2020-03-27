#include "Scene.hpp"

Scene::Scene(std::string _name) : Drawable(nullptr, _name) {
  // TODO: keep a draw configuration stack to allow a few meshes being drawn with different configs?
  
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
  glPolygonMode(fill_effective_polygon, fill_mode);

  // actually draw objects
  Drawable::draw();
}
