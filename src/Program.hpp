#pragma once
#include "lib.h"
#include <chrono>

struct Drawable;
struct Scene;

struct Program {

	static Program* Instance;
  
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

	// input
	bool receiving_text = false;
	std::string input_str;

  std::vector<Scene*> scenes;
  
  // implementation of these decides game behavior.
  void load_resources();

  void setup();
  void update(float elapsed);
  void draw();

  void release_resources();

};

