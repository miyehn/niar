#include "GameObject.hpp"

struct Compute : GameObject {

  Compute(Camera* camera);
  ~Compute();

  // inherited
  void update(float time_elapsed);
  void draw();
  bool handle_event(SDL_Event event);

  // buffers, properties...

  vector<vec3> vertices;
  uint vao;
  uint vbo;

  uint comp_shader_prog;
  uint draw_shader_prog;

  vector<float> read_buf, write_buf;
  uint read_tex, write_tex;

};
