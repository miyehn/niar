//
//  Program.cpp
//  sdl-empty
//
//  Created by miyehn on 8/20/19.
//  Copyright © 2019 miyehn. All rights reserved.
//

#include "Program.hpp"

using namespace std;
using namespace glm;

Program::Program(string name, int width, int height) {
  
  this->name = name;
  this->width = width;
  this->height = height;
  
  SDL_Init(SDL_INIT_VIDEO);
  
  // OpenGL settings
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  
  // create window
  this->window = SDL_CreateWindow(
                                  name.c_str(),
                                  100, 100, // SDL_WINDOWPOS_UNDEFINED, or SDL_WINDOWPOS_CENTERED
                                  width, height, // specify window size
                                  SDL_WINDOW_OPENGL
                                  );
  if (!window) {
      cerr << "Error creating SDL window: " << SDL_GetError() << endl;
      exit(1);
  }
  
  // create context
  this->context = SDL_GL_CreateContext(window);
  if (!context) {
      SDL_DestroyWindow(window);
      cerr << "Error creating OpenGL context: " << SDL_GetError() << endl;
      exit(1);
  }
  
  // glew setup (not sure why necessary)
  // https://open.gl/context#Onemorething
  glewExperimental = GL_TRUE;
  glewInit();

  this->previous_time = chrono::high_resolution_clock::now();

}

Program::~Program() {
  for (uint i=0; i<objects.size(); i++) {
    delete objects[i];
  }
}

void Program::run() {
  setup();

  bool running = true;
  
  while (running) {

    SDL_Event event;
    bool quit = false;

    while (SDL_PollEvent(&event)==1 && !quit) {
      if (event.type == SDL_QUIT) { quit=true; break; }

      else if (event.type==SDL_KEYUP && 
          event.key.keysym.sym==SDLK_ESCAPE) { quit=true; break; }

      else if (event.type==SDL_KEYUP || event.type==SDL_KEYDOWN) {
        camera.handle_event(event);
        for (uint i=0; i<objects.size(); i++) {
          objects[i]->handle_event(event);
        }
      }
    }
    if (quit) break;
    
    TimePoint current_time = chrono::high_resolution_clock::now();
    float time_elapsed = chrono::duration< float >(current_time - previous_time).count();
    previous_time = current_time;

    update(time_elapsed);

    draw();
    SDL_GL_SwapWindow(window);
  }
  
  // tear down
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  exit(0);
}

void Program::update(float time_elapsed) {
  
  camera.update(time_elapsed);
  for (uint i=0; i<objects.size(); i++) {
    objects[i]->update(time_elapsed);
  }
  
}
