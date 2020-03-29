#include "Program.hpp"
#include "Drawable.hpp"
#include "Scene.hpp"
#include "Camera.hpp"

int main(int argc, const char * argv[]) {

  uint w = 800;
  uint h = 600;

  Program* program = new Program("my program", w, h);
  program->run();
  delete program;
  return 0;
}

Program::Program(std::string name, int width, int height) {
  
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
    std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
    exit(1);
  }
  
  // create context
  this->context = SDL_GL_CreateContext(window);
  if (!context) {
    SDL_DestroyWindow(window);
    std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
    exit(1);
  }
  
  #ifdef MACOS
  // glew setup (not sure why necessary)
  // https://open.gl/context#Onemorething
  glewExperimental = GL_TRUE;
  glewInit();
  #endif

  this->previous_time = std::chrono::high_resolution_clock::now();

}

Program::~Program() {
  LOG("cleaning up...");
  for (uint i=0; i<scenes.size(); i++) {
    delete scenes[i];
  }
}

void Program::run() {

  load_resources();
  setup();

  while (true) {

    SDL_Event event;
    bool quit = false;

    // currently everything handles everything (except quit)
    while (SDL_PollEvent(&event)==1 && !quit) {
      if (event.type == SDL_QUIT) { quit=true; break; }

      else if (event.type==SDL_KEYUP && 
          event.key.keysym.sym==SDLK_ESCAPE) { quit=true; break; }

      else if (event.type==SDL_KEYUP || event.type==SDL_KEYDOWN) {
        Camera::Active->handle_event(event);
        for (uint i=0; i<scenes.size(); i++) {
          scenes[i]->handle_event(event);
        }
      }
    }
    if (quit) break;
    
    TimePoint current_time = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float>(current_time - previous_time).count();
    elapsed = std::min(0.1f, elapsed);
    previous_time = current_time;

    update(elapsed);

    draw();

    SDL_GL_SwapWindow(window);
  }
  
  // tear down
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Program::update(float elapsed) {
  
  Camera::Active->update(elapsed);
  for (uint i=0; i<scenes.size(); i++) {
    scenes[i]->update(elapsed);
  }
  
}

void Program::draw() {

  glClearColor(0.1f, 0.1f, 0.1f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);

  for (uint i=0; i<scenes.size(); i++) {
    scenes[i]->draw();
  }
    
}
