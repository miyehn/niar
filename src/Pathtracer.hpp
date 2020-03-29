#pragma once
#include "Drawable.hpp"

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

  float min_x, min_y, max_x, max_y; // in range [-1, 1]

  // ray tracing state
  bool paused;
  void pause_trace();
  void continue_trace();
  virtual void enable();
  virtual void disable();

  size_t pixels_per_frame;
  size_t progress;

  bool refresh;
  float refresh_timer;
  float refresh_interval;

  //---- buffer & opengl stuff ----

  // an image buffer of size width * height * 3 (since it has rgb channels)
  unsigned char* image_buffer;

  void set_rgb(size_t w, size_t h, vec3 rgb);
  void set_rgb(size_t i, vec3 rgb);

  uint texture, vbo, vao;

};

