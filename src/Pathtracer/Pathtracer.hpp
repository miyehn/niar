#pragma once
#include "Drawable.hpp"

struct Scene;
struct Ray;
struct Primitive;
struct Light;
struct RaytraceThread;

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
	size_t tile_size, tiles_X, tiles_Y;

  // ray tracing state and control
  bool paused;
	bool notified_pause_finish;
	bool finished;
  void pause_trace();
  void continue_trace();
  virtual void enable();
  virtual void disable();
	bool initialized;
	void initialize();
  void reset();

	TimePoint last_begin_time;
	float cumulative_render_time;
	size_t rendered_tiles;

	// scene
  std::vector<Primitive*> primitives;
	std::vector<Light*> lights;
  void load_scene(const Scene& scene);

	//---- pathtracing routine ----
	
	// multi-jittered sampling
	std::vector<vec2> pixel_offsets;
	void generate_pixel_offsets();

	// depth of field
	float depth_of_first_hit(int x, int y);

	// routine
	void generate_one_ray(Ray& ray, int x, int y);
  void generate_rays(std::vector<Ray>& rays, size_t index);
  vec3 raytrace_pixel(size_t index);
	void raytrace_tile(size_t tid, size_t tile_index);
  vec3 trace_ray(Ray& ray, int ray_depth, bool debug);

	// for debug use
	void raytrace_debug(size_t index);
	std::vector<vec3> logged_rays;

	//---- threading stuff ----

	size_t num_threads;
	std::function<void(int)> raytrace_task;

	std::vector<RaytraceThread*> threads;
	TaskQueue<size_t> raytrace_tasks;

  //---- buffers & opengl ----

  // an image buffer of size width * height * 3 (since it has rgb channels)
  unsigned char* image_buffer;
	unsigned char** subimage_buffers;

  void upload_rows(GLint begin, GLint end);
	void upload_tile(size_t subbuf_index, size_t tile_index);
	void upload_tile(size_t subbuf_index, GLint begin_x, GLint begin_y, GLint w, GLint h);
  void set_mainbuffer_rgb(size_t i, vec3 rgb);
	void set_subbuffer_rgb(size_t buf_i, size_t i, vec3 rgb);

  uint texture, vbo, vao;

	// for debug
	Shader loggedrays_shader;
	uint loggedrays_vbo, loggedrays_vao;

};
