#pragma once
#include "utils.hpp"

struct Updatable {
  virtual void update(float time_elapsed) = 0;
  virtual bool handle_event(SDL_Event event) = 0;
};
