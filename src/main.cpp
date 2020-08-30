#include "Program.hpp"
#include "Drawable.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "Pathtracer.hpp"
#include "Input.hpp"

Program* Program::Instance;
#ifdef WIN32
#ifdef main
#undef main
#endif
#endif
int main(int argc, const char * argv[]) {

  std::srand(time(nullptr));

  uint w = 800;
  uint h = 600;

	Program::Instance = new Program("niar", w, h);
	Program::Instance->run();
  delete Program::Instance;
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

	// store drawable sizes
	SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
  
  // create context
  this->context = SDL_GL_CreateContext(window);
  if (!context) {
    SDL_DestroyWindow(window);
    std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
    exit(1);
  }
  
  // glew setup
  // https://open.gl/context#Onemorething
  glewExperimental = GL_TRUE;
  glewInit();

  this->previous_time = std::chrono::high_resolution_clock::now();

}

Program::~Program() {
  release_resources();
}

void Program::run() {

  load_resources();
  setup();

  while (true) {

    SDL_Event event;
    bool quit = false;

    // currently everything handles everything (except quit)
    while (SDL_PollEvent(&event)==1 && !quit) {

			// termination
      if (event.type == SDL_QUIT) { quit=true; break; }
      else if (event.type==SDL_KEYUP && 
          event.key.keysym.sym==SDLK_ESCAPE) { quit=true; break; }

			// console input
			else if (event.type==SDL_KEYUP && !receiving_text && event.key.keysym.sym==SDLK_SLASH) {
				input_str = "";
				receiving_text = true;
				std::cout << "> " << std::flush;
			}
			else if (event.type == SDL_TEXTINPUT && receiving_text) {
				input_str += event.text.text;
				std::cout << event.text.text << std::flush;
			}
			else if (event.type == SDL_KEYUP && receiving_text && event.key.keysym.sym==SDLK_BACKSPACE) {
				input_str = input_str.substr(0, input_str.length()-1);
				std::cout << "\r> " << input_str << std::flush;
			}
			else if (event.type == SDL_KEYUP && receiving_text && event.key.keysym.sym==SDLK_RETURN) {
				receiving_text = false;
				std::cout << std::endl;
				process_input();
			}

      else if (!receiving_text) {
        // toggle between rasterizer & pathtracer
        if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_TAB) {
          if (Pathtracer::Instance->enabled) Pathtracer::Instance->disable();
          else Pathtracer::Instance->enable();
        }
        // update singletons
        if (Pathtracer::Instance->enabled) 
          Pathtracer::Instance->handle_event(event);
        // let all scene(s) handle the input
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
#ifndef WIN32 // for whatever reason, on Windows including any of these lines makes SDL unable to close the window and quit properly
  // tear down
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
#endif
}

void Program::update(float elapsed) {
  
  // camera
  Camera::Active->update_control(elapsed);
  // pathtracer
  if (Pathtracer::Instance && Pathtracer::Instance->enabled)
    Pathtracer::Instance->update(elapsed);
  // scenes (only the active one updates)
  if (Scene::Active && Scene::Active->enabled) {
		Scene::Active->update(elapsed);
  }
  
}

void Program::draw() {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glClearColor(0.1f, 0.1f, 0.1f, 1);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // scenes
  if (Scene::Active && Scene::Active->enabled) {
		Scene::Active->draw();
  }
  // pathtracer
  if (Pathtracer::Instance && Pathtracer::Instance->enabled) 
		Pathtracer::Instance->draw();
    
}
