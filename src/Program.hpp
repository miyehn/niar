#include <chrono>
#include "Camera.hpp"
#include "Drawable.hpp"

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

struct Program {
  
  Program(std::string name, int width, int height);
  ~Program();
  void run();
    
private:
    
  // program properties
  std::string name;
  size_t width;
  size_t height;
  
  SDL_Window* window;
  SDL_GLContext context;
  TimePoint previous_time;

  Camera camera;
  std::vector<Drawable*> objects;
  
  // implementation of these decides game behavior.
  void setup();
  void update(float elapsed);
  void draw();

};
