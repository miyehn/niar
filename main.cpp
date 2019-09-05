//Mode.hpp declares the "Mode::current" static member variable, which is used to decide where event-handling, updating, and drawing events go:
//GL.hpp will include a non-namespace-polluting set of opengl prototypes:
#include "GL.hpp"
//Includes for libSDL:
#include <SDL.h>
#include <glm/glm.hpp>

//...and for c++ standard library functions:
#include <iostream>

int main(int argc, char **argv) {
#ifdef _WIN32
  //when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
  try {
#endif

  //------------  initialization ------------

  //Initialize SDL library:
  SDL_Init(SDL_INIT_VIDEO);

  //Ask for an OpenGL context version 3.3, core profile, enable debug:
  SDL_GL_ResetAttributes();
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

  //create window:
  SDL_Window *window = SDL_CreateWindow(
    "TetraTetris", //TODO: remember to set a title for your game!
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    540, 480, //TODO: modify window size if you'd like
    SDL_WINDOW_OPENGL
    | SDL_WINDOW_RESIZABLE //uncomment to allow resizing
    | SDL_WINDOW_ALLOW_HIGHDPI //uncomment for full resolution on high-DPI screens
  );

  //prevent exceedingly tiny windows when resizing:
  SDL_SetWindowMinimumSize(window, 100, 100);

  if (!window) {
    std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
    return 1;
  }

  //Create OpenGL context:
  SDL_GLContext context = SDL_GL_CreateContext(window);

  if (!context) {
    SDL_DestroyWindow(window);
    std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
    return 1;
  }

  //On windows, load OpenGL entrypoints: (does nothing on other platforms)
  init_GL();

  //Set VSYNC + Late Swap (prevents crazy FPS):
  if (SDL_GL_SetSwapInterval(-1) != 0) {
    std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
    if (SDL_GL_SetSwapInterval(1) != 0) {
      std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
    }
  }

  //------------ main loop ------------

  //this inline function will be called whenever the window is resized,
  // and will update the window_size and drawable_size variables:
  glm::uvec2 window_size; //size of window (layout pixels)
  glm::uvec2 drawable_size; //size of drawable (physical pixels)
  //On non-highDPI displays, window_size will always equal drawable_size.
  auto on_resize = [&](){
    int w,h;
    SDL_GetWindowSize(window, &w, &h);
    window_size = glm::uvec2(w, h);
    SDL_GL_GetDrawableSize(window, &w, &h);
    drawable_size = glm::uvec2(w, h);
    glViewport(0, 0, drawable_size.x, drawable_size.y);
  };
  on_resize();

  bool running = true;
  //This will loop until the current mode is set to null:
  while (running) {
    {
      static SDL_Event evt;
      while (SDL_PollEvent(&evt) == 1) {
        //handle resizing:
        if (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          on_resize();
        }
        //handle quit signal:
        if (evt.type == SDL_QUIT
            || (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_ESCAPE)) {
          running = false;
          break;
        }
      }
      if (!running) break;
    }

    //Wait until the recently-drawn frame is shown before doing it all again:
    SDL_GL_SwapWindow(window);
  }


  //------------  teardown ------------

  SDL_GL_DeleteContext(context);
  context = 0;

  SDL_DestroyWindow(window);
  window = NULL;

  return 0;

#ifdef _WIN32
  } catch (std::exception const &e) {
    std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unhandled exception (unknown type)." << std::endl;
    throw;
  }
#endif
}
