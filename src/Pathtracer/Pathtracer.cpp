#include "Pathtracer.hpp"
#include "Render/Mesh.h"
#include "BSDF.hpp"
#include "PathtracerLight.hpp"
#include "Engine/Config.hpp"
#include "Render/Texture.h"
#include "Render/Materials/GltfMaterial.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/DebugDraw.h"
#include <stack>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <imgui.h>

#define NUM_CHANNELS 4
#define SIZE_PER_CHANNEL 1

// include this generated header to be able to use the kernels
#include "pathtracer_kernel_ispc.h"

struct RaytraceThread {

	enum Status { uninitialized, working, pending_upload, uploaded, ready_for_next, all_done };
	
	RaytraceThread(std::function<void(int)> work, int _tid) : tid(_tid) {
		thread = std::thread(work, _tid);
		finished = true;
		tile_index = -1;
		status = uninitialized;
	}

	int tid;
	std::thread thread;
	std::mutex m{};
	std::condition_variable cv;

	std::atomic<bool> finished;
	std::atomic<Status> status;
	int tile_index; // protected by m just like the tile buffer

};

struct ISPC_Data
{
	std::vector<ispc::Camera> camera;
	std::vector<glm::vec2> pixel_offsets;
	uint32_t num_offsets;
	std::vector<ispc::Triangle> triangles;
	std::vector<ispc::BSDF> bsdfs;
	std::vector<uint32_t> area_light_indices;
	uint32_t num_triangles;
	uint32_t num_area_lights;
	//uint8_t* output;
	uint32_t width;
	uint32_t height;
	uint32_t tile_size;
	uint32_t num_threads;
	uint32_t max_ray_depth;
	float rr_threshold;
	bool use_direct_light;
	uint32_t area_light_samples;
	std::vector<ispc::BVH> bvh_root;
	uint32_t bvh_stack_size;
	bool use_bvh;
	bool use_dof;
	float focal_distance;
	float aperture_radius;
};

namespace
{
static std::unordered_map<std::string, BSDF*> BSDFs;
}

Pathtracer::Pathtracer(
	uint32_t _width,
	uint32_t _height,
	bool _has_window
	) {

	width = _width;
	height = _height;

	initialized = false;
	enabled = false;
	has_window = _has_window;
}

Pathtracer::~Pathtracer() {
	if (!initialized) return;
	if (has_window)
	{
		delete window_surface;
		viewInfoUbo.release();
		delete debugLines;
	}
	delete image_buffer;
	for (uint32_t i=0; i<cached_config.NumThreads; i++) {
		delete subimage_buffers[i];
		// TODO: cleanup the threads in a more elegant way instead of being forced terminated?
	}
	delete subimage_buffers;

	for (auto l : lights) delete l;
	for (auto t : primitives) delete t;

	delete bvh;

	// delete BSDF library
	for (auto& pair : BSDFs) {
		delete pair.second;
	}
	BSDFs.clear();

	delete config;

	TRACE("deleted pathtracer");
}

void Pathtracer::initialize() {
	TRACE("initializing pathtracer");

	config = new ConfigFile("config/pathtracer.ini", [this](const ConfigFile* cfg) {
		cached_config.ISPC = cfg->lookup<int>("ISPC");
		cached_config.UseBVH = cfg->lookup<int>("UseBVH");

		cached_config.NumThreads = cfg->lookup<int>("NumThreads");
		cached_config.TileSize = cfg->lookup<int>("TileSize");

		cached_config.UseDirectLight = cfg->lookup<int>("UseDirectLight");
		cached_config.AreaLightSamples = cfg->lookup<int>("AreaLightSamples");

		cached_config.UseJitteredSampling = cfg->lookup<int>("UseJitteredSampling");
		cached_config.UseDOF = cfg->lookup<int>("UseDOF");
		cached_config.FocalDistance = cfg->lookup<float>("FocalDistance");
		cached_config.ApertureRadius = cfg->lookup<float>("ApertureRadius");

		cached_config.MaxRayDepth = cfg->lookup<int>("MaxRayDepth");
		cached_config.RussianRouletteThreshold = cfg->lookup<float>("RussianRouletteThreshold");

		cached_config.MinRaysPerPixel = cfg->lookup<int>("MinRaysPerPixel");
	});

	uint32_t tile_size = cached_config.TileSize;
	uint32_t num_threads = cached_config.NumThreads;

	tiles_X = std::ceil(float(width) / tile_size);
	tiles_Y = std::ceil(float(height) / tile_size);

	image_buffer = new unsigned char[width * height * NUM_CHANNELS * SIZE_PER_CHANNEL];
	subimage_buffers = new unsigned char*[num_threads];
	for (int i=0; i<num_threads; i++) {
		subimage_buffers[i] = new unsigned char[tile_size * tile_size * NUM_CHANNELS *SIZE_PER_CHANNEL];
	}

	//-------- load scene --------
	
	bvh = nullptr;
	if (drawable) load_scene(drawable);
	else WARN("Pathtracer scene not loaded - no active scene");

	ispc_data = nullptr;

#if 0
	// the two spheres (classical cornell box)
	BSDF* sphere_bsdf_1 = new Glass();
	primitives.emplace_back(static_cast<Primitive*>(new Sphere(vec3(-40, 430, -45), 30, sphere_bsdf_1)));
	BSDF* sphere_bsdf_2 = new Glass();
	primitives.emplace_back(static_cast<Primitive*>(new Sphere(vec3(40, 390, -45), 30, sphere_bsdf_2)));
#endif

	//-------- graphics api stuff setup --------

	if (has_window) {

		ImageCreator windowSurfaceCreator(
			VK_FORMAT_R8G8B8A8_UNORM,
			{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			"Pathtracer window surface image");
		window_surface = new Texture2D(windowSurfaceCreator);

		viewInfoUbo = VmaBuffer(&Vulkan::Instance->memoryAllocator,
								sizeof(ViewInfo),
								VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								VMA_MEMORY_USAGE_CPU_TO_GPU);

		debugLines = new DebugLines(viewInfoUbo, Vulkan::Instance->getSwapChainRenderPass());

		if (cached_config.Multithreaded) {
			//------------- threading ---------------
			// define work for raytrace threads
			raytrace_task = [this](int tid) {
				while (true) {
					std::unique_lock<std::mutex> lock(threads[tid]->m);

					// wait until main thread says it's okay to keep working
					threads[tid]->cv.wait(lock, [this, tid]{ return threads[tid]->status == RaytraceThread::ready_for_next; });

					// it now owns the lock, and main thread messaged it's okay to start tracing
					uint32_t tile;
					// try to get next tile to work on
					if (raytrace_tasks.dequeue(tile)) {
						threads[tid]->status = RaytraceThread::working;
						threads[tid]->tile_index = tile;
						raytrace_tile(tid, tile);
						threads[tid]->status = RaytraceThread::pending_upload;
					} else {
						threads[tid]->status = RaytraceThread::all_done;
						break;
					}
				}
			};

			for (uint32_t i=0; i<num_threads; i++) {
				threads.push_back(new RaytraceThread(raytrace_task, i));
			}
			//------------------------------------
		}
	}

	reset();

	initialized = true;
}

bool Pathtracer::handle_event(SDL_Event event) {
	if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_SPACE && !finished) {
		if (paused) continue_trace();
		else {
			TRACE("pausing - waiting for pending tiles");
			pause_trace();
		}
		return true;

	} else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) {
		int x, y; // NOTE: y IS INVERSED!!!!!!!!!!!
		SDL_GetMouseState(&x, &y);
		const uint8* state = SDL_GetKeyboardState(nullptr);

		if (state[SDL_SCANCODE_LSHIFT]) {
			uint32_t pixel_index = (height-y) * width + x;
			raytrace_debug(pixel_index);

		} else if (state[SDL_SCANCODE_LALT]) {
			float d = depth_of_first_hit(x, height-y);
			cached_config.FocalDistance = d;
			TRACE("setting focal distance to %f", d);
		}
		return true;
	} else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_RIGHT) {
		clear_debug_ray();
		return true;
	}

	return false;
}

BSDF *Pathtracer::get_or_create_mesh_bsdf(const std::string &materialName)
{
	auto iter = BSDFs.find(materialName);
	if (iter != BSDFs.end()) return iter->second;

	// not found; create a new one
	GltfMaterialInfo* info = GltfMaterial::getInfo(materialName);
	EXPECT(info != nullptr, true)

	auto bsdf = new Diffuse(info->BaseColorFactor);
	bsdf->set_emission(info->EmissiveFactor);
	BSDFs[info->name] = bsdf;
	TRACE("created new BSDF (%f, %f, %f)", bsdf->albedo.r, bsdf->albedo.g, bsdf->albedo.b)

	return bsdf;
}

// TODO: load scene recursively
void Pathtracer::load_scene(SceneObject *scene) {

	primitives.clear();
	lights.clear();

	bvh = new BVH(&primitives, 0);

	int meshes_count = 0;
	scene->foreach_descendent_bfs([&](SceneObject* drawable)
	{
		Mesh* mesh = dynamic_cast<Mesh*>(drawable);
		if (mesh) {

			BSDF* bsdf = get_or_create_mesh_bsdf(mesh->materialName);
			mesh->bsdf = bsdf;
			meshes_count++;

			bool emissive = mesh->bsdf->is_emissive;
			for (int i=0; i<mesh->faces.size(); i+=3) {
				// loop and load triangles
				Vertex v1 = mesh->vertices[mesh->faces[i]];
				Vertex v2 = mesh->vertices[mesh->faces[i + 1]];
				Vertex v3 = mesh->vertices[mesh->faces[i + 2]];
				Triangle* T = new Triangle(mesh->object_to_world(), v1, v2, v3, mesh->bsdf);
				Primitive* P = static_cast<Primitive*>(T);
				primitives.push_back(P);

				// also load as light if emissive
				if (emissive) {
					lights.push_back(static_cast<PathtracerLight*>(new AreaLight(T)));
				}
			}
		}
	});

	bvh->primitives_start = 0;
	bvh->primitives_count = primitives.size();
	bvh->update_extents();
	bvh->expand_bvh();

	TRACE("loaded a scene with %d meshes, %lu triangles, %lu lights",
		  meshes_count, primitives.size(), lights.size());
}

void Pathtracer::on_selected() {
	if (!initialized) initialize();
	enabled = true;
	TRACE("pathtracer enabled");
	camera->lock();

	// update view info to buffer
	ViewInfo.ViewMatrix = camera->world_to_object();
	ViewInfo.ProjectionMatrix = camera->camera_to_clip();
	ViewInfo.ProjectionMatrix[1][1] *= -1; // so it's not upside down

	ViewInfo.CameraPosition = camera->world_position();
	ViewInfo.ViewDir = camera->forward();

	viewInfoUbo.writeData(&ViewInfo);
}

void Pathtracer::on_unselected() {
	enabled = false;
	TRACE("pathtracer disabled");
	camera->unlock();
}

void Pathtracer::pause_trace() {
	myn::TimePoint end_time = std::chrono::high_resolution_clock::now();
	cumulative_render_time += std::chrono::duration<float>(end_time - last_begin_time).count();
	TRACE("rendered %f seconds so far.", cumulative_render_time);
	notified_pause_finish = false;
	paused = true;
}

void Pathtracer::continue_trace() {
	TRACE("continue trace");
	last_begin_time = std::chrono::high_resolution_clock::now();
	if (cached_config.ISPC) {
		load_ispc_data();
	}
	paused = false;
}

void Pathtracer::reset() {
	TRACE("reset pathtracer");
	
	if (cached_config.Multithreaded) {
		//-------- threading stuff --------
		if (finished) {
			threads.clear();
			for (uint32_t i=0; i<cached_config.NumThreads; i++) {
				threads.push_back(new RaytraceThread(raytrace_task, i));
			}
		}
		raytrace_tasks.clear();
		// reset thread status
		for (uint32_t i=0; i < threads.size(); i++) {
			threads[i]->status = RaytraceThread::uninitialized;
		}
		// enqueue all tiles
		for (uint32_t i=0; i < tiles_X * tiles_Y; i++) {
			raytrace_tasks.enqueue(i);
		}
		//---------------------------------
	}

	paused = true;
	finished = false;
	rendered_tiles = 0;
	cumulative_render_time = 0.0f;
	generate_pixel_offsets();

	memset(image_buffer, 40, width * height * NUM_CHANNELS * SIZE_PER_CHANNEL);
	if (has_window) upload_rows(0, height);
}

void Pathtracer::set_mainbuffer_rgb(uint32_t i, vec3 rgb) {
	uint32_t pixel_size = NUM_CHANNELS * SIZE_PER_CHANNEL;
	image_buffer[pixel_size * i] = char(rgb.r * 255.0f);
	image_buffer[pixel_size * i + 1] = char(rgb.g * 255.0f);
	image_buffer[pixel_size * i + 2] = char(rgb.b * 255.0f);
	image_buffer[pixel_size * i + 3] = 255;
}

void Pathtracer::set_subbuffer_rgb(uint32_t buf_i, uint32_t i, vec3 rgb) {
	uint32_t pixel_size = NUM_CHANNELS * SIZE_PER_CHANNEL;
	unsigned char* buf = subimage_buffers[buf_i];
	buf[pixel_size * i] = char(rgb.r * 255.0f);
	buf[pixel_size * i + 1] = char(rgb.g * 255.0f);
	buf[pixel_size * i + 2] = char(rgb.b * 255.0f);
	buf[pixel_size * i + 3] = 255;
}

void Pathtracer::upload_rows(uint32_t begin, uint32_t rows)
{
	uint32_t subimage_offset = width * begin * NUM_CHANNELS * SIZE_PER_CHANNEL;
	uint8_t* data = image_buffer + subimage_offset;
	vk::uploadPixelsToImage(
		data,
		0, begin,
		width, rows,
		NUM_CHANNELS *SIZE_PER_CHANNEL,
		window_surface->resource
		);

	int percentage = int(float(begin + rows) / float(height) * 100.0f);
	TRACE("refresh! updated %d rows, %d%% done.", rows, percentage);
}

void Pathtracer::upload_tile(uint32_t subbuf_index, uint32_t begin_x, uint32_t begin_y, uint32_t w, uint32_t h)
{
	unsigned char* buffer = subimage_buffers[subbuf_index];
	vk::uploadPixelsToImage(
		buffer,
		begin_x, begin_y,
		w, h,
		NUM_CHANNELS *SIZE_PER_CHANNEL,
		window_surface->resource
	);
}

void Pathtracer::upload_tile(uint32_t subbuf_index, uint32_t tile_index) {
	uint32_t X = tile_index % tiles_X;
	uint32_t Y = tile_index / tiles_X;
	uint32_t tile_size = cached_config.TileSize;

	uint32_t tile_w = std::min(tile_size, width - X * tile_size);
	uint32_t tile_h = std::min(tile_size, height - Y * tile_size);

	uint32_t x_offset = X * tile_size;
	uint32_t y_offset = Y * tile_size;
	
	upload_tile(subbuf_index, x_offset, y_offset, tile_w, tile_h);
}

vec3 gamma_correct(vec3 in) {
	const vec3 gamma(0.455f);
	return pow(in, gamma);
}

void Pathtracer::raytrace_tile(uint32_t tid, uint32_t tile_index) {
	uint32_t X = tile_index % tiles_X;
	uint32_t Y = tile_index / tiles_X;

	uint32_t tile_size = cached_config.TileSize;

	uint32_t tile_w = std::min(tile_size, width - X * tile_size);
	uint32_t tile_h = std::min(tile_size, height - Y * tile_size);

	uint32_t x_offset = X * tile_size;
	uint32_t y_offset = Y * tile_size;

	if (cached_config.ISPC)
	{
		// dispatch task to ispc
		ispc::raytrace_scene_ispc(
			ispc_data->camera.data(), 
			(float*)ispc_data->pixel_offsets.data(),
			ispc_data->num_offsets,
			ispc_data->triangles.data(),
			ispc_data->bsdfs.data(),
			ispc_data->area_light_indices.data(),
			ispc_data->num_triangles,
			ispc_data->num_area_lights,
			subimage_buffers[tid],
			ispc_data->width,
			ispc_data->height,
			true,
			ispc_data->tile_size,
			tile_w, tile_h,
			X, Y,
			ispc_data->num_threads,
			ispc_data->max_ray_depth,
			ispc_data->rr_threshold,
			ispc_data->use_direct_light,
			ispc_data->area_light_samples,
			ispc_data->bvh_root.data(),
			ispc_data->bvh_stack_size,
			ispc_data->use_bvh,
			ispc_data->use_dof,
			ispc_data->focal_distance,
			ispc_data->aperture_radius);
	}
	else
	{
		for (uint32_t y = 0; y < tile_h; y++) {
			for (uint32_t x = 0; x < tile_w; x++) {

				uint32_t px_index_main = width * (y_offset + y) + (x_offset + x);
				vec3 color = raytrace_pixel(px_index_main);
				if (!has_window) color = gamma_correct(color);
				set_mainbuffer_rgb(px_index_main, color);

				uint32_t px_index_sub = y * tile_w + x;
				set_subbuffer_rgb(tid, px_index_sub, color);

			}
		}
	}

}

void Pathtracer::load_ispc_data() {

	if (ispc_data != nullptr) {
		delete(ispc_data);
	}
	ispc_data = new ISPC_Data();

	auto ispc_vec3 = [](const vec3& v) {
		ispc::vec3 res;
		res.x = v.x; res.y = v.y; res.z = v.z;
		return res;
	};
	
	// construct scene representation (triangles + materials list)
	ispc_data->bsdfs.resize(primitives.size());
	ispc_data->triangles.resize(primitives.size());
	for (int i=0; i<primitives.size(); i++)
	{
		ispc::Triangle &T = ispc_data->triangles[i];
		Triangle* T0 = dynamic_cast<Triangle*>(primitives[i]);
		if (!T0) ERR("failed to cast primitive to triangle?");
		// construct its corresponding material
		T.bsdf_index = i;
		ispc_data->bsdfs[i].albedo = ispc_vec3(T0->bsdf->albedo);
		ispc_data->bsdfs[i].Le = ispc_vec3(T0->bsdf->get_emission());
		ispc_data->bsdfs[i].is_delta = T0->bsdf->is_delta;
		ispc_data->bsdfs[i].is_emissive = T0->bsdf->is_emissive;
		if (T0->bsdf->type == BSDF::Mirror) {
			ispc_data->bsdfs[i].type = ispc::Mirror;
		} else if (T0->bsdf->type == BSDF::Glass) {
			ispc_data->bsdfs[i].type = ispc::Glass;
		} else {
			ispc_data->bsdfs[i].type = ispc::Diffuse;
		}
		// construct the ispc triangle object
		for (int j=0; j<3; j++) {
			T.vertices[j] = ispc_vec3(T0->vertices[j]);
			T.enormals[j] = ispc_vec3(T0->enormals[j]);
		}
		T.plane_n = ispc_vec3(T0->plane_n);
		T.plane_k = T0->plane_k;
		T.area = T0->area;
	}
	ispc_data->num_triangles = primitives.size();

	ispc_data->area_light_indices.resize(lights.size());
	uint light_count = 0;
	for (int i=0; i<lights.size(); i++) {
		if (lights[i]->type == PathtracerLight::AreaLight) {
			Triangle* T = dynamic_cast<AreaLight*>(lights[i])->triangle;
			auto it = find(primitives.begin(), primitives.end(), T);
			if (it != primitives.end()) { // found
				ispc_data->area_light_indices[light_count] = it - primitives.begin();
			}
			light_count++;
		}
	}
	ispc_data->num_area_lights = light_count;

	// construct camera
	ispc_data->camera.resize(1);
	mat3 c2wr = mat3(camera->object_to_world());
	ispc_data->camera[0].camera_to_world_rotation.colx = ispc_vec3(c2wr[0]);
	ispc_data->camera[0].camera_to_world_rotation.coly = ispc_vec3(c2wr[1]);
	ispc_data->camera[0].camera_to_world_rotation.colz = ispc_vec3(c2wr[2]);
	ispc_data->camera[0].position = ispc_vec3(camera->world_position());
	ispc_data->camera[0].fov = camera->fov;
	ispc_data->camera[0].aspect_ratio = camera->aspect_ratio;

	// pixel offsets
	ispc_data->pixel_offsets = pixel_offsets;
	ispc_data->num_offsets = pixel_offsets.size();

	// BVH
	uint max_depth = 0;
	std::stack<BVH*> st;
	std::unordered_map<BVH*, int> m;
	// first iteration: make the structs, and map from node to index
	st.push(bvh);
	while (!st.empty()) {
		BVH* ptr = st.top(); st.pop();
		int self_index = ispc_data->bvh_root.size();
		m[ptr] = self_index;
		max_depth = glm::max(max_depth, ptr->depth);
		// make the node (except children indices)
		ispc::BVH node;
		node.min = ispc_vec3(ptr->min);
		node.max = ispc_vec3(ptr->max);
		node.triangles_start = ptr->primitives_start;
		node.triangles_count = ptr->primitives_count;
		node.self_index = self_index;
		ispc_data->bvh_root.push_back(node);
		// push children
		if (ptr->left) st.push(ptr->right);
		if (ptr->right) st.push(ptr->left);
	}
	// second iteration: set children indices
	st.push(bvh);
	while (!st.empty()) {
		BVH* ptr = st.top(); st.pop();
		bool is_leaf = !ptr->left || !ptr->right;
		int self_index = m[ptr];
		int left_index = is_leaf ? -1 : m[ptr->left];
		int right_index = is_leaf ? -1 : m[ptr->right];
		ispc_data->bvh_root[self_index].left_index = left_index;
		ispc_data->bvh_root[self_index].right_index = right_index;

		// push children
		if (ptr->left) st.push(ptr->right);
		if (ptr->right) st.push(ptr->left);
	}

	// and the rest of the inputs
	ispc_data->width = width;
	ispc_data->height = height;
	ispc_data->tile_size = cached_config.TileSize;
	ispc_data->num_threads = cached_config.Multithreaded ? cached_config.NumThreads : 1;
	ispc_data->max_ray_depth = cached_config.MaxRayDepth;
	ispc_data->rr_threshold = cached_config.RussianRouletteThreshold;
	ispc_data->use_direct_light = cached_config.UseDirectLight;
	ispc_data->area_light_samples = cached_config.AreaLightSamples;
	ispc_data->bvh_stack_size = (1 + max_depth) * 2;
	ispc_data->use_bvh = cached_config.UseBVH;
	ispc_data->use_dof = cached_config.UseDOF;
	ispc_data->focal_distance = cached_config.FocalDistance;
	ispc_data->aperture_radius = cached_config.ApertureRadius;

	TRACE("reloaded ISPC data");
}

void Pathtracer::raytrace_scene_to_buf() {

	if (cached_config.ISPC)
	{
		load_ispc_data();
		LOG("ispc max depth: %u", ispc_data->bvh_stack_size);

		// dispatch task to ispc
		ispc::raytrace_scene_ispc(
			ispc_data->camera.data(), 
			(float*)ispc_data->pixel_offsets.data(),
			ispc_data->num_offsets,
			ispc_data->triangles.data(),
			ispc_data->bsdfs.data(),
			ispc_data->area_light_indices.data(),
			ispc_data->num_triangles,
			ispc_data->num_area_lights,
			image_buffer,
			ispc_data->width,
			ispc_data->height,
			false,
			ispc_data->tile_size,
			0, 0, // tile_width, tile_height
			0, 0, // tile_indexX, tile_indexY
			ispc_data->num_threads,
			ispc_data->max_ray_depth,
			ispc_data->rr_threshold,
			ispc_data->use_direct_light,
			ispc_data->area_light_samples,
			ispc_data->bvh_root.data(),
			ispc_data->bvh_stack_size,
			ispc_data->use_bvh,
			ispc_data->use_dof,
			ispc_data->focal_distance,
			ispc_data->aperture_radius);
	}
	else
	{
		if (cached_config.Multithreaded)
		{
			myn::ThreadSafeQueue<uint> tasks;
			uint task_size = cached_config.TileSize * cached_config.TileSize;
			uint image_size = width * height;
			for (uint i = 0; i < image_size; i += task_size) {
				tasks.enqueue(i);
			}
			std::function<void(int)> raytrace_task = [&](int tid){
				uint task_begin;
				while (tasks.dequeue(task_begin))
				{
					uint task_end = glm::min(image_size, task_begin + task_size);
					for (uint task = task_begin; task < task_end; task++)
					{
						vec3 color = raytrace_pixel(task);
						if (!has_window) color = gamma_correct(color);
						set_mainbuffer_rgb(task, color);
					}
				}
			};
			LOG("enqueued %zu tasks", tasks.size());
			// create the threads and execute
			std::vector<std::thread> threads_tmp;
			for (uint tid = 0; tid < cached_config.NumThreads; tid++) {
				threads_tmp.emplace_back(raytrace_task, tid);
			}
			LOG("created %d threads", cached_config.NumThreads);
			for (uint tid = 0; tid < cached_config.NumThreads; tid++) {
				threads_tmp[tid].join();
			}
			LOG("joined threads");
		}
		else
		{
			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {

					uint32_t px_index = width * y + x;
					vec3 color = raytrace_pixel(px_index);
					set_mainbuffer_rgb(px_index, color);

				}
			}
		}

	}

}

// https://www.scratchapixel.com/lessons/digital-imaging/simple-image-manipulations
void Pathtracer::output_file(const std::string& path) {

	if (width == 0 || height == 0) { fprintf(stderr, "Can't save an empty image\n"); return; } 
	std::ofstream ofs; 
	try { 
		ofs.open(path.c_str(), std::ios::binary); // need to spec. binary mode for Windows users 
		if (ofs.fail()) throw("Can't open output file"); 
		ofs << "P6\n" << width << " " << height << "\n255\n"; 
		unsigned char r, g, b; 
		// loop over each pixel in the image, clamp and convert to byte format
		uint32_t pixel_size = NUM_CHANNELS * SIZE_PER_CHANNEL;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int i = y * width + x;
				r = image_buffer[pixel_size * i];
				g = image_buffer[pixel_size * i + 1];
				b = image_buffer[pixel_size * i + 2];
				ofs << r << g << b; 
			}
		}
		ofs.close(); 
	} 
	catch (const char *err) { 
		fprintf(stderr, "%s\n", err); 
		ofs.close(); 
	} 
}

void Pathtracer::render(VkCommandBuffer cmdbuf)
{
	if (!initialized) initialize();

	// update
	if (cached_config.Multithreaded && !cached_config.ISPC) // multithreaded c++
	{
		if (!finished) {
			int uploaded_threads = 0;
			int finished_threads = 0;

			for (uint32_t i=0; i<threads.size(); i++) {

				if (threads[i]->status == RaytraceThread::all_done) {
					finished_threads++;
					if (threads[i]->thread.joinable()) threads[i]->thread.join();

				} else if (threads[i]->status == RaytraceThread::pending_upload) {
					std::lock_guard<std::mutex> lock(threads[i]->m);
					upload_tile(i, threads[i]->tile_index);
					threads[i]->status = RaytraceThread::uploaded;

				} else if (threads[i]->status == RaytraceThread::uploaded) {
					uploaded_threads++;
					if (!paused) {
						threads[i]->status = RaytraceThread::ready_for_next;
						threads[i]->cv.notify_one();
					}

				} else if (threads[i]->status == RaytraceThread::uninitialized) {
					if (!paused) {
						threads[i]->status = RaytraceThread::ready_for_next;
						threads[i]->cv.notify_one();
					}
				}

			}

			if (finished_threads == threads.size()) {
				TRACE("Done!");
				finished = true;
				pause_trace();
			} else if (paused && uploaded_threads == threads.size() && !notified_pause_finish) {
				TRACE("pending tiles finished");
				notified_pause_finish = true;
			}
		}

	}
	else
	{
		if (!paused) {
			if (rendered_tiles == tiles_X * tiles_Y) {
				TRACE("Done!");
				pause_trace();

			} else {

				// TODO: spawn a task to do this instead
				raytrace_tile(0, rendered_tiles);
				upload_tile(0, rendered_tiles);

				rendered_tiles++;
			}
		}
	}

	///////////////////// DISPLAY ////////////////////////

	// barrier source into trasfer source
	vk::insertImageBarrier(cmdbuf, window_surface->resource.image,
						   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_ACCESS_TRANSFER_WRITE_BIT,
						   VK_ACCESS_TRANSFER_WRITE_BIT,
						   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vk::blitToScreen(
		cmdbuf,
		window_surface->resource.image,
		{0, 0, 0},
		{(int32_t)width, (int32_t)height, 1});
	vk::insertImageBarrier(cmdbuf, window_surface->resource.image,
						   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_ACCESS_TRANSFER_WRITE_BIT,
						   VK_ACCESS_TRANSFER_WRITE_BIT,
						   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	if (debugLines && debugLines->numSegments() > 0) {
		Vulkan::Instance->beginSwapChainRenderPass(cmdbuf);
		debugLines->bindAndDraw(cmdbuf);
		Vulkan::Instance->endSwapChainRenderPass(cmdbuf);
	}
}

void Pathtracer::pathtrace_to_file(uint32_t w, uint32_t h, const std::string &path)
{
	TRACE("TODO: rewrite this")
	return;

	/*
	initialize_pathtracer_config();

	Pathtracer::Instance = new Pathtracer(w, h, "command line pathtracer", false);

	Camera::Active = new Camera(w, h);

	//Scene::Active = Pathtracer::load_cornellbox_scene();

	Pathtracer::Instance->initialize();
	TRACE("starting..");

	if (Cfg.Pathtracer.ISPC) {
		TRACE("---- ISPC ON ----");
	} else {
		TRACE("---- ISPC OFF ----");
	}

	TIMER_BEGIN();
	Pathtracer::Instance->raytrace_scene_to_buf();
	TIMER_END();
	TRACE("done! took %f seconds", execution_time);
	Pathtracer::Instance->output_file(relative_path);

	// delete Scene::Active; // TODO: pull out graphics tear down from Scene::~Scene()
	delete Camera::Active;
	delete Pathtracer::Instance;
	 */
}

Pathtracer *Pathtracer::get(uint32_t _w, uint32_t _h, bool _has_window)
{
	static Pathtracer* renderer = nullptr;
	static uint32_t w, h;
	static bool has_window;

	if (_w == 0 || _h == 0) {
		// just get whatever is already created
		return renderer;
	}

	if (renderer == nullptr || _w != w || _h != h || _has_window != has_window) {
		delete renderer;

		w = _w; h = _h; has_window = _has_window;
		renderer = new Pathtracer(w, h, has_window);

		if (has_window) Vulkan::Instance->destructionQueue.emplace_back([](){ delete renderer; });
	}
	return renderer;
}

void Pathtracer::draw_config_ui()
{
	// reset
	bool reset_condition = paused && notified_pause_finish;
	ImGui::BeginDisabled(!reset_condition);
	if (ImGui::Button("clear buffer")) {
		reset();
	}
	ImGui::EndDisabled();
}

// file that contains the actual pathtracing meat
#include "Pathtracer.inl"
