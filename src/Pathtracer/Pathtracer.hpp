#pragma once
#include "Drawable.hpp"
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

struct Scene;
struct Ray;
struct Primitive;
struct Light;

template <typename T>
struct TaskQueue {
	
	size_t size() {
		std::lock_guard<std::mutex> lock(queue_mutex);
		return queue.size();
	}

	bool dequeue(T& out_task) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (queue.size() == 0) return false;
		out_task = queue.back();
		queue.pop_back();
		return true;
	}

	void enqueue(T task) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		queue.insert(queue.begin(), task);
	}

	void clear() {
		std::lock_guard<std::mutex> lock(queue_mutex);
		queue.clear();
	}

private:
	std::vector<T> queue;
	std::mutex queue_mutex;

};

struct RaytraceThread {

	enum Status { uninitialized, working, pending_upload, ready_for_next, all_done };
	
	RaytraceThread(std::function<void(int)> work, int _tid) : tid(_tid) {
		thread = std::thread(work, _tid);
		finished = true;
		tile_index = -1;
		status = uninitialized;
	}

	int tid;
	std::thread thread;
	std::mutex m;
	std::condition_variable cv;

	std::atomic<bool> finished;
	std::atomic<Status> status;
	int tile_index; // protected by m just like the tile buffer

};

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

	TimePoint last_begin_time;
	float cumulative_render_time;

  std::vector<Primitive*> primitives;
	std::vector<Light*> lights;
  void load_scene(const Scene& scene);

	std::vector<vec2> pixel_offsets;
	void generate_pixel_offsets();

  void generate_rays(std::vector<Ray>& rays, size_t index);
  vec3 raytrace_pixel(size_t index);
	void raytrace_debug(size_t index);
	void raytrace_tile(size_t tid, size_t tile_index);
  vec3 trace_ray(Ray& ray, int ray_depth, bool debug);

	// for debug
	std::vector<vec3> logged_rays;

	//---- threading stuff ----

	std::vector<RaytraceThread*> threads;
	TaskQueue<size_t> raytrace_tasks;

  //---- buffer & opengl stuff ----

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

