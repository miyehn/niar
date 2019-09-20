#include "GameObject.hpp"

struct Blade{
  Blade(vec3 root);
  string str() {
    return
      "root_w: " + to_string(root_w) + "\n" +
      "above_h: " + to_string(above_h) + "\n" +
      "ctrl_s: " + to_string(ctrl_s) + "\n" +
      "up_o: " + to_string(up_o) + "\n";
  }

  vec4 root_w; // v0, width
  vec4 above_h; // v1, height
  vec4 ctrl_s; // v2, stiffness
  vec4 up_o; // a unit vector up, orientation
};
static_assert(sizeof(Blade) == 16 * sizeof(float), "Blade should be packed");

struct Compute : GameObject {

  Compute(Camera* camera);
  ~Compute();

  // inherited
  void update(float time_elapsed);
  void draw();
  bool handle_event(SDL_Event event);

  // properties, data...
  vector<Blade> blades = vector<Blade>();
  vector<float> read_back;

  // a few constants
  GLsizei work_group_size = 4; // IMPORTANT: check this with shader to make sure in sync!!!
  GLsizei img_buffer_pixels = 0; // number of pixels in image buffer: blades.size() * pixels needed (4) for each
  GLsizei num_workgroups = 0; // number of workgroups: number of invocations (blades.size()) / work group size
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
