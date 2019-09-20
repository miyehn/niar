//Mode.hpp declares the "Mode::current" static member variable, which is used to decide where event-handling, updating, and drawing events go:
//GL.hpp will include a non-namespace-polluting set of opengl prototypes:

#include "Program.hpp"

int main(int argc, char **argv) {
#ifdef _WIN32
  //when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
  try {
#endif

  //------------  initialization ------------

  string window_name = "grass tess";
  int window_w = 540;
  int window_h = 480;

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
    window_name.c_str(), //TODO: remember to set a title for your game!
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    window_w, window_h, //TODO: modify window size if you'd like
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

  Program* program = new Program(window_name, window_w, window_h, window, context);
  program->run();

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


Program::Program(string name, int width, int height, SDL_Window* window, SDL_GLContext context): camera(width, height) {

  this->name = name;
  this->width = width;
  this->height = height;
  this->window = window;
  this->context = context;
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
    time_elapsed = glm::min(0.1f, time_elapsed);
    previous_time = current_time;

    update(time_elapsed);

    draw();

    SDL_GL_SwapWindow(window);
  }

  // tear down
  SDL_GL_DeleteContext(context);
  context = 0;
  SDL_DestroyWindow(window);
  window = NULL;
}

void Program::update(float time_elapsed) {

  camera.update(time_elapsed);
  for (uint i=0; i<objects.size(); i++) {
    objects[i]->update(time_elapsed);
  }

}

void Program::draw() {

  glClearColor(0.1f, 0.1f, 0.1f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);

  for (uint i=0; i<objects.size(); i++) {
    objects[i]->draw();
  }

}