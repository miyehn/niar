#pragma once
#include "Camera.hpp"

struct GameObject: Updatable {

  // inherited
  virtual void update() = 0;
  virtual bool handle_event(SDL_Event event) = 0;

  // game object can be shown on screen
  virtual void draw() = 0;

  Camera* camera = nullptr; // should cast to Camera* before use
  glm::mat4 transformation = glm::mat4(1.0f);
};
