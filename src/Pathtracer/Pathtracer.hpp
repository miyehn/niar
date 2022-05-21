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
class ConfigFile;

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
	uint32_t tiles_X, tiles_Y;

	// store some frequently-accessed configs here to alleviate config lookup cost
	struct {
		int ISPC = 0;
		int UseBVH = 1;
		int Multithreaded = 0; // initially 0 so if set to >0 by config file, will create the threads
		int NumThreads = 0;
		int TileSize = 16;
		int UseDirectLight = 1;
		int AreaLightSamples = 2;
		int UseJitteredSampling = 1;
		int UseDOF = 1;
		float FocalDistance = 5.0f;
		float ApertureRadius = 0.25f;
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
		int MinRaysPerPixel = 4;
	} cached_config;
	ConfigFile* config = nullptr;

	// ray tracing state and control
	bool paused = true;
	bool notified_pause_finish = true;
	bool finished = true;
	void pause_trace();
	void continue_trace();
	bool initialized = false;
	bool enabled = false;
	void reset();

	myn::TimePoint last_begin_time;
	float cumulative_render_time;
	uint32_t rendered_tiles;

	// scene
	std::vector<Primitive*> primitives;
	std::vector<PathtracerLight*> lights;
	BVH* bvh = nullptr;
	void load_scene(SceneObject *scene);
	static BSDF* get_or_create_mesh_bsdf(const std::string& materialName);

	ISPC_Data* ispc_data = nullptr;
	void load_ispc_data();

	//---- pathtracing routine ----
	
	// multi-jittered sampling
	std::vector<vec2> pixel_offsets;
	void generate_pixel_offsets();

	// depth of field
	float depth_of_first_hit(int x, int y);

	// routine
	void generate_one_ray(RayTask& task, int x, int y);
	void generate_rays(std::vector<RayTask>& tasks, uint32_t index);
	vec3 raytrace_pixel(uint32_t index);
	void raytrace_tile(uint32_t tid, uint32_t tile_index);
	void trace_ray(RayTask& task, int ray_depth, bool debug);

	// for debug use
	void raytrace_debug(uint32_t index);
	void clear_debug_ray();
	std::vector<vec3> logged_rays;

	//---- threading stuff ----

	std::function<void(int)> raytrace_task;

	std::vector<RaytraceThread*> threads;
	myn::ThreadSafeQueue<uint32_t> raytrace_tasks;

	void clear_tasks_and_threads_begin();
	void clear_tasks_and_threads_wait();

	//---- buffers & opengl ----

	// an image buffer of size width * height * 3 (since it has rgb channels)
	unsigned char* image_buffer = nullptr;
	unsigned char** subimage_buffers = nullptr;

	void upload_rows(uint32_t begin, uint32_t end);
	void upload_tile(uint32_t subbuf_index, uint32_t tile_index);
	void upload_tile(uint32_t subbuf_index, uint32_t begin_x, uint32_t begin_y, uint32_t w, uint32_t h);
	void set_mainbuffer_rgb(uint32_t i, vec3 rgb);
	void set_subbuffer_rgb(uint32_t buf_i, uint32_t i, vec3 rgb);

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
