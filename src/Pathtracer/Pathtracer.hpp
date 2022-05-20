#pragma once
#include "Utils/myn/Timer.h"
#include "Utils/myn/ThreadSafeQueue.h"
#include "Engine/SceneObject.hpp"
#include "Scene/AABB.hpp"
#include "BVH.hpp"
#include "Render/Renderers/Renderer.h"
#include "Render/Vulkan/DescriptorSet.h"
#include <vulkan/vulkan.h>

struct Scene;
struct Ray;
struct RayTask;
struct Primitive;
struct PathtracerLight;
struct RaytraceThread;
class Texture2D;
class DebugLines;

struct ISPC_Data;

class Pathtracer : public Renderer {
public:
	static Pathtracer* get(uint32_t w = 0, uint32_t h = 0, bool has_window = true);

	static void pathtrace_to_file(uint32_t w, uint32_t h, const std::string& path);
	
	Pathtracer(
			uint32_t _width,
			uint32_t _height,
			bool _has_window = true);
	~Pathtracer() override;

	bool handle_event(SDL_Event event);
	void draw_config_ui() override;
	void render(VkCommandBuffer cmdbuf) override;

	void initialize();
	void on_selected() override;
	void on_unselected() override;

	bool is_enabled() const { return enabled; }

	void raytrace_scene_to_buf(); //trace to main output buffer directly; used for rendering to file
	void output_file(const std::string& path);

private:

	// size for the pathtraced image - could be different from display window.
	bool has_window;
	uint32_t width, height;
	size_t tile_size, tiles_X, tiles_Y;

	// ray tracing state and control
	bool paused;
	bool notified_pause_finish;
	bool finished;
	void pause_trace();
	void continue_trace();
	bool initialized;
	bool enabled;
	void reset();

	myn::TimePoint last_begin_time;
	float cumulative_render_time;
	size_t rendered_tiles;

	// scene
	std::vector<Primitive*> primitives;
	std::vector<PathtracerLight*> lights;
	BVH* bvh;
	void load_scene(SceneObject *scene);
	static BSDF* get_or_create_mesh_bsdf(const std::string& materialName);
	bool use_bvh;

	ISPC_Data* ispc_data;
	void load_ispc_data();

	//---- pathtracing routine ----
	
	// multi-jittered sampling
	std::vector<vec2> pixel_offsets;
	void generate_pixel_offsets();

	// depth of field
	float depth_of_first_hit(int x, int y);

	// routine
	void generate_one_ray(RayTask& task, int x, int y);
	void generate_rays(std::vector<RayTask>& tasks, size_t index);
	vec3 raytrace_pixel(size_t index);
	void raytrace_tile(size_t tid, size_t tile_index);
	void trace_ray(RayTask& task, int ray_depth, bool debug);

	// for debug use
	void raytrace_debug(size_t index);
	void clear_debug_ray();
	std::vector<vec3> logged_rays;

	//---- threading stuff ----

	size_t num_threads;
	std::function<void(int)> raytrace_task;

	std::vector<RaytraceThread*> threads;
	myn::ThreadSafeQueue<size_t> raytrace_tasks;

	//---- buffers & opengl ----

	// an image buffer of size width * height * 3 (since it has rgb channels)
	unsigned char* image_buffer;
	unsigned char** subimage_buffers;

	void upload_rows(int32_t begin, int32_t end);
	void upload_tile(size_t subbuf_index, size_t tile_index);
	void upload_tile(size_t subbuf_index, int32_t begin_x, int32_t begin_y, int32_t w, int32_t h);
	void set_mainbuffer_rgb(size_t i, vec3 rgb);
	void set_subbuffer_rgb(size_t buf_i, size_t i, vec3 rgb);

	// vulkan
	Texture2D* window_surface = nullptr;
	DebugLines* debugLines = nullptr;

	struct {
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

		glm::vec3 CameraPosition;
		float _pad0 = 2.333f;

		glm::vec3 ViewDir;
		float _pad1 = 2.333f;

	} ViewInfo;

	VmaBuffer viewInfoUbo;
	DescriptorSet descriptorSet;

};
