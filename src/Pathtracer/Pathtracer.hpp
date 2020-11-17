#pragma once
#include "Drawable.hpp"
#include "Utils.hpp"

struct Scene;
struct Ray;
struct RayTask;
struct Primitive;
struct PathtracerLight;
struct RaytraceThread;
struct MatGeneric;

struct Pathtracer : public Drawable {

	static Pathtracer* Instance;
	
	Pathtracer(
			size_t _width,
			size_t _height,
			std::string _name = "[unamed pathtracer]",
			bool _has_window = true);
	virtual ~Pathtracer();

	virtual bool handle_event(SDL_Event event);
	virtual void update(float elapsed);
	virtual void draw();

	void initialize();
	void enable();
	void disable();

	void raytrace_scene_to_buf(); //trace to main output buffer directly; used for rendering to file
	void output_file(const std::string& path);

private:

	// size for the pathtraced image - could be different from display window.
	bool has_window;
	size_t width, height;
	size_t tile_size, tiles_X, tiles_Y;

	// ray tracing state and control
	bool paused;
	bool notified_pause_finish;
	bool finished;
	void pause_trace();
	void continue_trace();
	bool initialized;
	void reset();

	TimePoint last_begin_time;
	float cumulative_render_time;
	size_t rendered_tiles;

	// scene
	std::vector<Primitive*> primitives;
	std::vector<PathtracerLight*> lights;
	void load_scene(const Scene& scene);

	//---- pathtracing routine ----
	
	// multi-jittered sampling
	std::vector<vec2> pixel_offsets;
	void generate_pixel_offsets();

	// depth of field
	float depth_of_first_hit(int x, int y);

	// routine
	void generate_one_ray(RayTask& task, int x, int y);
	void generate_rays(std::vector<RayTask>& tasks, size_t index);
	vec3 raytrace_pixel(size_t index, bool ispc = false);
	void raytrace_tile(size_t tid, size_t tile_index);
	void trace_ray(RayTask& task, int ray_depth, bool debug);
	// (for 418 project: ISPC)
	void trace_ray_ispc(RayTask& task, int ray_depth);
	//---- END 418 ----

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

	uint texture;

	// for debug
	MatGeneric* loggedrays_mat = nullptr;
	uint loggedrays_vbo, loggedrays_vao;

	virtual void set_local_position(vec3 _local_position) {}
	virtual void set_rotation(quat _rotation) {}
	virtual void set_scale(vec3 _scale) {}

};
