#include "Pathtracer.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "BSDF.hpp"
#include "Primitive.hpp"
#include "PathtracerLight.hpp"
#include "Input.hpp"
#include "Materials.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>

CVar<float>* FocalDistance = new CVar<float>("FocalDistance", 500);
CVar<float>* ApertureRadius = new CVar<float>("ApertureRadius", 8);

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

	if (loggedrays_mat) delete loggedrays_mat;
	TRACE("deleted pathtracer");
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
	
	if (Scene::Active) load_scene(*Scene::Active);
	else WARN("Pathtracer scene not loaded - no active scene");

#if 1
	// the two spheres (classical cornell box)
	BSDF* sphere_bsdf_1 = new Mirror();
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
		loggedrays_mat = new MatGeneric("yellow");//Shader("../shaders/yellow.vert", "../shaders/yellow.frag");

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
			//------- -- threading ---------------
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
			FocalDistance->set(d);
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
				primitives.push_back(static_cast<Primitive*>(T));
				
				// also load as light if emissive
				if (emissive) {
					lights.push_back(static_cast<PathtracerLight*>(new AreaLight(T)));
				}
			}
		}
	}

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

void Pathtracer::raytrace_tile(size_t tid, size_t tile_index) {
	size_t X = tile_index % tiles_X;
	size_t Y = tile_index / tiles_X;

	size_t tile_w = std::min(tile_size, width - X * tile_size);
	size_t tile_h = std::min(tile_size, height - Y * tile_size);

	size_t x_offset = X * tile_size;
	size_t y_offset = Y * tile_size;

	for (size_t y = 0; y < tile_h; y++) {
		for (size_t x = 0; x < tile_w; x++) {

			size_t px_index_main = width * (y_offset + y) + (x_offset + x);
			vec3 color = raytrace_pixel(px_index_main);
			set_mainbuffer_rgb(px_index_main, color);

			size_t px_index_sub = y * tile_w + x;
			set_subbuffer_rgb(tid, px_index_sub, color);

		}
	}

}

void Pathtracer::raytrace_scene() {
	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++) {

			size_t px_index = width * y + x;
			vec3 color = raytrace_pixel(px_index);
			set_mainbuffer_rgb(px_index, color);

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
		for (int i = 0; i < width * height; ++i) { 
			r = image_buffer[3 * i];//static_cast(std::min(1.f, img.pixels[i].r) * 255); 
			g = image_buffer[3 * i + 1];//static_cast(std::min(1.f, img.pixels[i].g) * 255); 
			b = image_buffer[3 * i + 2];//static_cast(std::min(1.f, img.pixels[i].b) * 255); 
			ofs << r << g << b; 
		} 
		ofs.close(); 
	} 
	catch (const char *err) { 
		fprintf(stderr, "%s\n", err); 
		ofs.close(); 
	} 
}

void Pathtracer::update(float elapsed) {

	if (Cfg.Pathtracer.Multithreaded) {
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
	
	} else {
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

	glViewport(0, 0, width*2, height*2);

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
