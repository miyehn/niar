#include "GameObject.hpp"

struct Blade{
  Blade(vec3 root);
  vec4 root_w; // v0, width
  vec4 above_h; // v1, height
  vec4 ctrl_s; // v2, stiffness
  vec4 up_o; // a unit vector up, orientation
};
static_assert(sizeof(Blade) == 16 * sizeof(float), "Blade should be packed");

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
};

