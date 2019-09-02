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
    
}

Program::~Program() {
  for (uint i=0; i<objects.size(); i++) {
    delete objects[i];
  }
}

void Program::run() {
    setup();
    
    while (true) {

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

      draw();
      SDL_GL_SwapWindow(window);
    }
    
    // tear down
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

/**
 * model space (input coordinates),
 * world space (model placement in world),
 * camera space (inverse of camera placement in world),
 * multiply by perspective matrix and send to vert
 * tip: could do: transpose(make_mat4(entries)) to get a matrix from 16 entries
 */
mat4 Program::getTransformation(vec3 objectPosition) {
    mat4 worldTransform = translate(mat4(1.0f), objectPosition); // world space
    
    mat4 cameraR = rotate(mat4(1.0f), cameraPitch, vec3(1.0f, 0.0f, 0.0f));
    mat4 cameraTransform = translate(mat4(1.0f), cameraPosition) * cameraR;
    cameraTransform = inverse(cameraTransform); // camera space
    
    mat4 projection = perspective(fov, aspectRatio(), cutoffNear, cutoffFar);
    
    return projection * cameraTransform * worldTransform;
}
