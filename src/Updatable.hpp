#pragma once
#include "utils.hpp"

struct Updatable {
  virtual void update() = 0;
  virtual bool handle_event(SDL_Event event) = 0;
};
