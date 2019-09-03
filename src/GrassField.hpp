#include "GameObject.hpp"
#include "Blade.hpp"

struct GrassField : GameObject {
  
  GrassField(Camera* camera, uint num_blades);
  ~GrassField();

  // inherited
  void update(float time_elapsed);
  bool handle_event(SDL_Event event);
  void draw();

  // properties and methods
  vector<Blade> blades;

  uint shader;
  uint vbo;
  uint vao;
  float* render_buffer;
  void assemble_buffer();

};
