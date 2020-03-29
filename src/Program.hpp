#pragma once
#include <chrono>
#include "lib.h"

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

struct Drawable;
struct Scene;

struct Program {
  
  Program(
      std::string name, 
      int width, 
      int height);
  ~Program();
  void run();
    
  // generic program properties
  std::string name;
  size_t width;
  size_t height;
  
  SDL_Window* window;
  SDL_GLContext context;
  TimePoint previous_time;

  std::vector<Scene*> scenes;
  
  // implementation of these decides game behavior.
  void load_resources();
  void setup();
  void update(float elapsed);
  void draw();

  void release_resources();

};

