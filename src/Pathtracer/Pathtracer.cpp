#include "Pathtracer.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "BSDF.hpp"
#include "Primitive.hpp"
#include "PathtracerLight.hpp"
#include "Input.hpp"
#include "Materials.hpp"
#include <stack>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>

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
	std::mutex m;
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

Pathtracer::Pathtracer(
	size_t _width, 
	size_t _height, 
	std::string _name,
	bool _has_window
	) : Drawable(nullptr, _name) {

	if (Cfg.Pathtracer.SmallWindow) {
		width = _width / 2;
		height = _height / 2;
	} else {
		width = _width;
		height = _height;
	}

	initialized = false;
	enabled = false;
	has_window = _has_window;
}

Pathtracer::~Pathtracer() {
	if (!initialized) return;
	if (has_window) {
		glDeleteTextures(1, &texture);
		glDeleteBuffers(1, &loggedrays_vbo);
		glDeleteVertexArrays(1, &loggedrays_vao);
	}
	delete image_buffer;
	for (size_t i=0; i<num_threads; i++) {
		delete subimage_buffers[i];
		// TODO: cleanup the threads in a more elegant way instead of being forced terminated?
	}
	delete subimage_buffers;

	for (auto l : lights) delete l;
	for (auto t : primitives) delete t;

	if (bvh) delete bvh;

	if (loggedrays_mat) delete loggedrays_mat;
	TRACE("deleted pathtracer");
}

Scene* Pathtracer::load_cornellbox_scene(bool init_graphics) {

	/* Classic cornell box
	 *
	 * NOTE: it forces everything to be rendered with basic lighting when pathtracer is disabled
	 * bc this scene only contains area(mesh) lights, which is only supported by pathtracer.
	 *
	 * Also this scene itself doesn't contain the two spheres - they're later added in Pathtracer::initialize()
	 * Can alternatively disable the two spheres over there and add other custom meshes (see below)
	 */
	Camera::Active->position = vec3(0, 0, 0);
	Camera::Active->cutoffFar = 1000.0f;
	Camera::Active->fov = radians(30.0f);

	Scene* scene = new Scene("my scene");

	// cornell box scene
	std::vector<Mesh*> meshes;
#if 1
	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_box.fbx", init_graphics);
	for (int i=0; i<meshes.size(); i++) { // 4 is floor
		Mesh* mesh = meshes[i];
		mesh->bsdf = new Diffuse(vec3(0.6f));
		if (i==1) {// right
			mesh->bsdf->albedo = vec3(0.32f, 0.32f, 0.8f); 
		} else if (i==2) {// left
			mesh->bsdf->albedo = vec3(0.8f, 0.32f, 0.32f); 
		}
		scene->add_child(static_cast<Drawable*>(mesh));
	}

	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_light.fbx", init_graphics);
	Mesh* light = meshes[0];
	light->bsdf = new Diffuse();
	light->name = "light";
	light->bsdf->set_emission(vec3(8.0f));
	scene->add_child(static_cast<Drawable*>(light));
#endif

	// add more items to it
	Mesh* mesh;
#if 1
	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/prism_1.fbx", init_graphics);
	mesh = meshes[0];
	mesh->bsdf = new Mirror();
	mesh->name = "prism 1";
	scene->add_child(static_cast<Drawable*>(mesh));

	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/prism_2.fbx", init_graphics);
	mesh = meshes[0];
	mesh->bsdf = new Diffuse(vec3(0.25f, 0.4f, 0.6f));
	mesh->name = "prism 2";
	scene->add_child(static_cast<Drawable*>(mesh));
#endif

#if 1
	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/poly_sphere.fbx", init_graphics);
	mesh = meshes[0];
	mesh->bsdf = new Glass();
	mesh->name = "sphere1";
	scene->add_child(static_cast<Drawable*>(mesh));
#endif

#if 0
	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/16planes.fbx", init_graphics);
	for (int i=0; i<meshes.size(); i++) { // 4 is floor
		Mesh* mesh = meshes[i];
		mesh->bsdf = new Diffuse(vec3(0.6f));
		scene->add_child(static_cast<Drawable*>(mesh));
	}
#endif

	return scene;
}

void Pathtracer::initialize() {
	TRACE("initializing pathtracer");

	num_threads = Cfg.Pathtracer.NumThreads;
	tile_size = Cfg.Pathtracer.TileSize;

	tiles_X = std::ceil(float(width) / tile_size);
	tiles_Y = std::ceil(float(height) / tile_size);

	image_buffer = new unsigned char[width * height * 3]; 
	subimage_buffers = new unsigned char*[num_threads];
	for (int i=0; i<num_threads; i++) {
		subimage_buffers[i] = new unsigned char[tile_size * tile_size * 3];
	}

	//-------- load scene --------
	
	bvh = nullptr;
	use_bvh = true;
	if (Scene::Active) load_scene(*Scene::Active);
	else WARN("Pathtracer scene not loaded - no active scene");

	ispc_data = nullptr;

#if 0
	// the two spheres (classical cornell box)
	BSDF* sphere_bsdf_1 = new Glass();
	primitives.emplace_back(static_cast<Primitive*>(new Sphere(vec3(-40, 430, -45), 30, sphere_bsdf_1)));
	BSDF* sphere_bsdf_2 = new Glass();
	primitives.emplace_back(static_cast<Primitive*>(new Sphere(vec3(40, 390, -45), 30, sphere_bsdf_2)));
#endif

	//-------- opengl stuff setup --------

	if (has_window) {

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_buffer);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// and for debug draw
		loggedrays_mat = new MatGeneric("yellow");

		glGenBuffers(1, &loggedrays_vbo);
		glGenVertexArrays(1, &loggedrays_vao);
		glBindVertexArray(loggedrays_vao);
		{
			glBindBuffer(GL_ARRAY_BUFFER, loggedrays_vbo);
			glVertexAttribPointer(
					0, // atrib index
					3, // num of data elems
					GL_FLOAT, // data type
					GL_FALSE, // normalized
					3 * sizeof(float), // stride size
					(void*)0); // offset in bytes since stride start
			glEnableVertexAttribArray(0);
		}
		glBindVertexArray(0);

		if (Cfg.Pathtracer.Multithreaded) {
			//------------- threading ---------------
			// define work for raytrace threads
			raytrace_task = [this](int tid) {
				while (true) {
					std::unique_lock<std::mutex> lock(threads[tid]->m);

					// wait until main thread says it's okay to keep working
					threads[tid]->cv.wait(lock, [this, tid]{ return threads[tid]->status == RaytraceThread::ready_for_next; });

					// it now owns the lock, and main thread messaged it's okay to start tracing
					size_t tile;
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

			for (size_t i=0; i<num_threads; i++) {
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

	} else if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_0) {
		reset();

	} else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) {
		int x, y; // NOTE: y IS INVERSED!!!!!!!!!!!
		SDL_GetMouseState(&x, &y);
		const uint8* state = SDL_GetKeyboardState(nullptr);

		if (state[SDL_SCANCODE_LSHIFT]) {
			size_t pixel_index = (height-y) * width + x;
			raytrace_debug(pixel_index);

		} else if (state[SDL_SCANCODE_LALT]) {
			float d = depth_of_first_hit(x, height-y);
			Cfg.Pathtracer.FocalDistance->set(d);
			TRACEF("setting focal distance to %f", d);
		}
	} else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_RIGHT) {
		logged_rays.clear();
	}

	return Drawable::handle_event(event);
}

// TODO: load scene recursively
void Pathtracer::load_scene(const Scene& scene) {

	primitives.clear();
	lights.clear();

	bvh = new BVH(&primitives, 0);

	for (Drawable* drawable : scene.children) {
		Mesh* mesh = dynamic_cast<Mesh*>(drawable);
		if (mesh) {
			if (!mesh->bsdf) {
				WARN("trying to load a mesh without bsdf. skipping...");
				continue;
			}

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
	}
	bvh->primitives_start = 0;
	bvh->primitives_count = primitives.size();
	bvh->update_extents();
	bvh->expand_bvh();

	TRACEF("loaded a scene with %d meshes, %d triangles, %d lights", 
			scene.children.size(), primitives.size(), lights.size());
}

void Pathtracer::enable() {
	if (!initialized) initialize();
	TRACE("pathtracer enabled");
	Camera::Active->lock();
	Drawable::enable();
}

void Pathtracer::disable() {
	TRACE("pathtracer disabled");
	Camera::Active->unlock();
	Drawable::disable();
}

void Pathtracer::pause_trace() {
	TimePoint end_time = std::chrono::high_resolution_clock::now();
	cumulative_render_time += std::chrono::duration<float>(end_time - last_begin_time).count();
	TRACEF("rendered %f seconds so far.", cumulative_render_time);
	notified_pause_finish = false;
	paused = true;
}

void Pathtracer::continue_trace() {
	TRACE("continue trace");
	last_begin_time = std::chrono::high_resolution_clock::now();
	use_bvh = Cfg.Pathtracer.UseBVH->get();
	if (Cfg.Pathtracer.ISPC) {
		load_ispc_data();
	}
	paused = false;
}

void Pathtracer::reset() {
	TRACE("reset pathtracer");
	
	if (Cfg.Pathtracer.Multithreaded) {
		//-------- threading stuff --------
		if (finished) {
			threads.clear();
			for (size_t i=0; i<num_threads; i++) {
				threads.push_back(new RaytraceThread(raytrace_task, i));
			}
		}
		raytrace_tasks.clear();
		// reset thread status
		for (size_t i=0; i < threads.size(); i++) {
			threads[i]->status = RaytraceThread::uninitialized;
		}
		// enqueue all tiles
		for (size_t i=0; i < tiles_X * tiles_Y; i++) {
			raytrace_tasks.enqueue(i);
		}
		//---------------------------------
	}

	paused = true;
	finished = false;
	rendered_tiles = 0;
	cumulative_render_time = 0.0f;
	generate_pixel_offsets();

	memset(image_buffer, 40, width * height * 3);
	if (has_window) upload_rows(0, height);
}

void Pathtracer::set_mainbuffer_rgb(size_t i, vec3 rgb) {
	image_buffer[3 * i] = char(rgb.r * 255.0f);
	image_buffer[3 * i + 1] = char(rgb.g * 255.0f);
	image_buffer[3 * i + 2] = char(rgb.b * 255.0f);
}

void Pathtracer::set_subbuffer_rgb(size_t buf_i, size_t i, vec3 rgb) {
	unsigned char* buf = subimage_buffers[buf_i];
	buf[3 * i] = char(rgb.r * 255.0f);
	buf[3 * i + 1] = char(rgb.g * 255.0f);
	buf[3 * i + 2] = char(rgb.b * 255.0f);
}

void Pathtracer::upload_rows(GLint begin, GLsizei rows) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	GLsizei subimage_offset = width * begin * 3;
	glTexSubImage2D(GL_TEXTURE_2D, 0, 
			0, begin, // min x, min y
			width, rows, // subimage width, subimage height
			GL_RGB, GL_UNSIGNED_BYTE, 
			image_buffer + subimage_offset);
	glBindTexture(GL_TEXTURE_2D, 0);

	int percentage = int(float(begin + rows) / float(height) * 100.0f);
	TRACEF("refresh! updated %d rows, %d%% done.", rows, percentage);
}

void Pathtracer::upload_tile(size_t subbuf_index, GLint begin_x, GLint begin_y, GLint w, GLint h) {
	unsigned char* buffer = subimage_buffers[subbuf_index];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			begin_x, begin_y,
			w, h,
			GL_RGB, GL_UNSIGNED_BYTE,
			buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Pathtracer::upload_tile(size_t subbuf_index, size_t tile_index) {
	size_t X = tile_index % tiles_X;
	size_t Y = tile_index / tiles_X;

	size_t tile_w = std::min(tile_size, width - X * tile_size);
	size_t tile_h = std::min(tile_size, height - Y * tile_size);

	size_t x_offset = X * tile_size;
	size_t y_offset = Y * tile_size;
	
	upload_tile(subbuf_index, x_offset, y_offset, tile_w, tile_h);
}

vec3 gamma_correct(vec3 in) {
	const vec3 gamma(0.455f);
	return pow(in, gamma);
}

void Pathtracer::raytrace_tile(size_t tid, size_t tile_index) {
	size_t X = tile_index % tiles_X;
	size_t Y = tile_index / tiles_X;

	size_t tile_w = std::min(tile_size, width - X * tile_size);
	size_t tile_h = std::min(tile_size, height - Y * tile_size);

	size_t x_offset = X * tile_size;
	size_t y_offset = Y * tile_size;

	if (Cfg.Pathtracer.ISPC)
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
		for (size_t y = 0; y < tile_h; y++) {
			for (size_t x = 0; x < tile_w; x++) {

				size_t px_index_main = width * (y_offset + y) + (x_offset + x);
				vec3 color = gamma_correct(raytrace_pixel(px_index_main));
				set_mainbuffer_rgb(px_index_main, color);

				size_t px_index_sub = y * tile_w + x;
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
	mat3 c2wr = Camera::Active->camera_to_world_rotation();
	ispc_data->camera[0].camera_to_world_rotation.colx = ispc_vec3(c2wr[0]);
	ispc_data->camera[0].camera_to_world_rotation.coly = ispc_vec3(c2wr[1]);
	ispc_data->camera[0].camera_to_world_rotation.colz = ispc_vec3(c2wr[2]);
	ispc_data->camera[0].position = ispc_vec3(Camera::Active->position);
	ispc_data->camera[0].fov = Camera::Active->fov;
	ispc_data->camera[0].aspect_ratio = Camera::Active->aspect_ratio;

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
	ispc_data->tile_size = tile_size;
	ispc_data->num_threads = Cfg.Pathtracer.Multithreaded ? Cfg.Pathtracer.NumThreads : 1;
	ispc_data->max_ray_depth = Cfg.Pathtracer.MaxRayDepth;
	ispc_data->rr_threshold = Cfg.Pathtracer.RussianRouletteThreshold;
	ispc_data->use_direct_light = Cfg.Pathtracer.UseDirectLight;
	ispc_data->area_light_samples = Cfg.Pathtracer.AreaLightSamples;
	ispc_data->bvh_stack_size = (1 + max_depth) * 2;
	ispc_data->use_bvh = Cfg.Pathtracer.UseBVH->get();
	ispc_data->use_dof = Cfg.Pathtracer.UseDOF->get();
	ispc_data->focal_distance = Cfg.Pathtracer.FocalDistance->get();
	ispc_data->aperture_radius = Cfg.Pathtracer.ApertureRadius->get();

	TRACE("reloaded ISPC data");
}

void Pathtracer::raytrace_scene_to_buf() {

	if (Cfg.Pathtracer.ISPC)
	{
		load_ispc_data();
		LOGF("ispc max depth: %u", ispc_data->bvh_stack_size);

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
		use_bvh = Cfg.Pathtracer.UseBVH->get();
		if (Cfg.Pathtracer.Multithreaded)
		{
			TaskQueue<uint> tasks;
			uint task_size = tile_size * tile_size;
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
						vec3 color = gamma_correct(raytrace_pixel(task));
						set_mainbuffer_rgb(task, color);
					}
				}
			};
			LOGF("enqueued %u tasks", tasks.size());
			// create the threads and execute
			std::vector<std::thread> threads_tmp;
			for (uint tid = 0; tid < num_threads; tid++) {
				threads_tmp.push_back(std::thread(raytrace_task, tid));
			}
			LOGF("created %u threads", num_threads);
			for (uint tid = 0; tid < num_threads; tid++) {
				threads_tmp[tid].join();
			}
			LOG("joined threads");
		}
		else
		{
			for (size_t y = 0; y < height; y++) {
				for (size_t x = 0; x < width; x++) {

					size_t px_index = width * y + x;
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
		for (int y = height-1; y >= 0; y--) {
			for (int x = 0; x < width; x++) {
				int i = y * width + x;
				r = image_buffer[3 * i];
				g = image_buffer[3 * i + 1];
				b = image_buffer[3 * i + 2];
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

void Pathtracer::update(float elapsed) {

	if (Cfg.Pathtracer.Multithreaded && !Cfg.Pathtracer.ISPC)
	{
		if (!finished) {
			int uploaded_threads = 0;
			int finished_threads = 0;

			for (size_t i=0; i<threads.size(); i++) {

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
				size_t X = rendered_tiles % tiles_X;
				size_t Y = rendered_tiles / tiles_X;

				raytrace_tile(0, rendered_tiles);
				upload_tile(0, rendered_tiles);

				rendered_tiles++;
			}
		}
	}

	Drawable::update(elapsed);
}

void Pathtracer::draw() {
	
	//---- draw the image buffer first ----

	glViewport(0, 0, width, height);

	Blit* blit = Blit::blit();
	blit->begin_pass();
	blit->set_tex2D("TEX", 0, texture);
	blit->end_pass();

	//---- then draw the logged rays ----
	glDisable(GL_DEPTH_TEST); // TODO: make this a state push & pop
	loggedrays_mat->use(this);
	glBindVertexArray(loggedrays_vao);
	glDrawArrays(GL_LINE_STRIP, 0, logged_rays.size());

	glBindVertexArray(0);
	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);

	Drawable::draw();
}

// file that contains the actual pathtracing meat
#include "Pathtracer.inl"
