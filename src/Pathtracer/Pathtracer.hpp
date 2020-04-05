#pragma once
#include "Drawable.hpp"

struct Scene;
struct Ray;
struct Triangle;
struct Light;

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

	size_t num_threads;

  // size for the pathtraced image - could be different from display window.
  size_t width, height;
	size_t tile_size, tiles_X, tiles_Y;
	size_t rendered_tiles;

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

	TimePoint last_begin_time;
	float cumulative_render_time;

  std::vector<Triangle*> triangles;
	std::vector<Light*> lights;
  void load_scene(const Scene& scene);

  std::vector<Ray> generate_rays(size_t index);
  vec3 raytrace_pixel(size_t index);
	void raytrace_tile(size_t X, size_t Y);
  vec3 trace_ray(Ray& ray, int ray_depth);

  //---- buffer & opengl stuff ----

  // an image buffer of size width * height * 3 (since it has rgb channels)
  unsigned char* image_buffer;
	unsigned char** subimage_buffers;

  void upload_rows(GLint begin, GLint end);
	void upload_tile(size_t subbuf_index, GLint begin_x, GLint begin_y, GLint w, GLint h);
  void set_mainbuffer_rgb(size_t i, vec3 rgb);
	void set_subbuffer_rgb(size_t buf_i, size_t i, vec3 rgb);

  uint texture, vbo, vao;

};

