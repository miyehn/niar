#pragma once
#include "Camera.hpp"

struct GameObject: Updatable {

  // inherited
  virtual void update(float time_elapsed) = 0;
  virtual bool handle_event(SDL_Event event) = 0;

  // game object can be shown on screen
  virtual void draw() = 0;

  Camera* camera = nullptr;
  GameObject(Camera* cam) { this->camera = cam; }
  virtual ~GameObject() {
    for (uint i=0; i<children.size(); i++) delete children[i];
  }
  vector<GameObject*> children = vector<GameObject*>();

  glm::mat4 transformation = glm::mat4(1.0f);
};
