#include "GameObject.hpp"

struct Compute : GameObject {

  Compute(Camera* camera);
  ~Compute();

  // inherited
  void update(float time_elapsed);
  void draw();
  bool handle_event(SDL_Event event);

  // a few constants
  GLsizei num_floats = 16;
  GLsizei work_group_size = 8; // IMPORTANT: check this with shader to make sure in sync!!!
  GLsizei num_vertices = 3; // just draw a triangle for demo purpose.

  // buffers, properties...
  uint vao;
  uint vbo;
  uint comp_shader_prog;
  uint draw_shader_prog;
  // each texture gets associated with a buffer (generated at initialization),
  // and just remains associated with that buffer.
  uint tex_r, tex_w;

};
