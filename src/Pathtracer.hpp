#pragma once
#include "Drawable.hpp"

struct Scene;
struct Ray;
struct Triangle;

struct Pathtracer : public Drawable {

  static Pathtracer* Instance;
  
  Pathtracer(
      size_t _width,
      size_t _height,
      std::string _name = "[unamed pathtracer]");
  virtual ~Pathtracer();

  virtual bool handle_event(SDL_Event event);
  virtual void update(float elapsed);
  virtual void draw();

  // size for the pathtraced image - could be different from display window.
  size_t width, height;

  // ray tracing state
  bool paused;
  void pause_trace();
  void continue_trace();
  virtual void enable();
  virtual void disable();

  void reset();

  size_t pixels_per_frame;
  size_t progress;

  float refresh_timer;
  float refresh_interval;
  size_t uploaded_rows;

  std::vector<Triangle> triangles;
  void load_scene(const Scene& scene);

  std::vector<Ray> generate_rays(size_t index);
  vec3 trace_ray(Ray& ray, int ray_depth);
  vec3 raytrace_pixel(size_t index);

  //---- buffer & opengl stuff ----

  // an image buffer of size width * height * 3 (since it has rgb channels)
  unsigned char* image_buffer;

  void upload_rows(GLint begin, GLint end);
  void set_rgb(size_t w, size_t h, vec3 rgb);
  void set_rgb(size_t i, vec3 rgb);

  uint texture, vbo, vao;

};

