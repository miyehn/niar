#include "Scene.hpp"

Scene::Scene(Drawable* _root, std::string _name) : Drawable(nullptr, _name) {
  root = _root;
}

Scene::~Scene(){
  delete root;
}

bool Scene::handle_event(SDL_Event event) {
  if (root) return root->handle_event(event);
  return false;
}

void Scene::update(float elapsed) {
  if (root) root->update(elapsed);
}

void Scene::draw() {
  if (root) root->draw();
}
