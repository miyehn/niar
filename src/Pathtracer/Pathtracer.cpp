#include "Pathtracer.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "BSDF.hpp"
#include "Primitive.hpp"
#include "Light.hpp"
#include <chrono>

#define DEBUG 0
#define NUM_THREADS 8
#define MULTITHREADED 0

Pathtracer::Pathtracer(
  size_t _width, 
  size_t _height, 
  std::string _name
  ) : width(_width), 
      height(_height), 
      Drawable(nullptr, _name) {

#if 0 // small window
  float min_x = -0.96f; float min_y = -0.96f;
  float max_x = 0.0f; float max_y = 0.0f;
#else // full window
  float min_x = -1.0f; float min_y = -1.0f;
  float max_x = 1.0f; float max_y = 1.0f;
#endif

	num_threads = NUM_THREADS;
	tile_size = 16;

	tiles_X = std::ceil(float(width) / tile_size);
	tiles_Y = std::ceil(float(height) / tile_size);

  image_buffer = new unsigned char[width * height * 3]; 
	subimage_buffers = new unsigned char*[num_threads];
	for (int i=0; i<num_threads; i++) {
		subimage_buffers[i] = new unsigned char[tile_size * tile_size * 3];
	}

  enabled = false;

  //-------- opengl stuff setup --------

  shader = Shader("../shaders/quad.vert", "../shaders/quad.frag");
  shader.set_parameters = [this]() {
    shader.set_tex2D(0, texture);
  };

  float quad_vertices[24] = {
    min_x, max_y, 0.0f, 1.0f, // tl
    min_x, min_y, 0.0f, 0.0f, // bl
    max_x, min_y, 1.0f, 0.0f, // br
    min_x, max_y, 0.0f, 1.0f, // tl
    max_x, min_y, 1.0f, 0.0f, // br
    max_x, max_y, 1.0f, 1.0f, // tr
  };

  glGenBuffers(1, &vbo);
  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0, // atrib index
        2, // num of data elems
        GL_FLOAT, // data type
        GL_FALSE, // normalized
        4 * sizeof(float), // stride size
        (void*)0); // offset in bytes since stride start
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, // atrib index
        2, // num of data elems
        GL_FLOAT, // data type
        GL_FALSE, // normalized
        4 * sizeof(float), // stride size
        (void*)(2 * sizeof(float))); // offset in bytes since stride start
    glEnableVertexAttribArray(1);
  }
  glBindVertexArray(0);

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

  //------------------------------------

  reset();
}

Pathtracer::~Pathtracer() {
  glDeleteTextures(1, &texture);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  delete image_buffer;
	for (size_t i=0; i<num_threads; i++) delete subimage_buffers[i];
	delete subimage_buffers;

	for (auto l : lights) delete l;
	for (auto t : primitives) delete t;
#if DEBUG
  GL_ERRORS();
#endif
  TRACE("deleted pathtracer");
}

bool Pathtracer::handle_event(SDL_Event event) {
  if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_p) {
    if (paused) continue_trace();
    else pause_trace();
    return true;
  } else if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_0) {
    reset();
  } else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		size_t pixel_index = (height-y) * width + x;
		raytrace_debug(pixel_index);
	}

  return Drawable::handle_event(event);
}

// TODO: load scene recursively
void Pathtracer::load_scene(const Scene& scene) {

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
					lights.push_back(static_cast<Light*>(new AreaLight(T)));
				}
      }
    }
  }

  TRACEF("loaded a scene with %d meshes, %d triangles, %d lights", 
			scene.children.size(), primitives.size(), lights.size());
}

void Pathtracer::enable() {
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
  TRACE("pause trace");
	TimePoint end_time = std::chrono::high_resolution_clock::now();
	cumulative_render_time += std::chrono::duration<float>(end_time - last_begin_time).count();
	TRACEF("rendered %f seconds so far.", cumulative_render_time);
  paused = true;
}

void Pathtracer::continue_trace() {
  TRACE("continue trace");
	last_begin_time = std::chrono::high_resolution_clock::now();
  paused = false;
}

void Pathtracer::reset() {
  TRACE("reset pathtracer");
  memset(image_buffer, 40, width * height * 3);
  paused = true;
	rendered_tiles = 0;
	cumulative_render_time = 0.0f;
	raytrace_tasks.clear();
	
	//-------- threading stuff --------
	
	// enqueue all tiles
	for (size_t i=0; i < tiles_X * tiles_Y; i++) {
		raytrace_tasks.enqueue(i);
	}
	
	// define work for raytrace threads
	auto task = [this](int tid) {
		while (true) {
			std::unique_lock<std::mutex> lock(threads[tid]->m);

			// wait until main thread says it's okay to keep working
			threads[tid]->cv.wait(lock, [this, tid]{ return threads[tid]->status == RaytraceThread::ready_for_next; });

			// try to get next tile to work on
			size_t tile;
			if (raytrace_tasks.dequeue(tile)) {
				// it now owns the lock, and main thread messaged it's okay to start tracing
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

	// start threads
	threads.clear(); // TODO: what if not first time reset? (join them at some point before this?
	for (size_t i=0; i<num_threads; i++) {
		threads.push_back(new RaytraceThread(task, i));
	}

	//---------------------------------
	
  upload_rows(0, height);
}

void Pathtracer::set_mainbuffer_rgb(size_t i, vec3 rgb) {
#if DEBUG
  if (i >= width * height * 3) ERR("set_rgb indexing out of range!!");
#endif
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
#if DEBUG
  GL_ERRORS();
#endif
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
#if DEBUG
  GL_ERRORS();
#endif
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

void Pathtracer::update(float elapsed) {
  if (!paused) {

#if MULTITHREADED
		int finished_threads = 0;

		for (size_t i=0; i<threads.size(); i++) {

			if (threads[i]->status == RaytraceThread::all_done) {
				finished_threads++;
				if (threads[i]->thread.joinable()) threads[i]->thread.join();

			} else if (threads[i]->status == RaytraceThread::pending_upload) {
				{// block things when uploading data
					std::lock_guard<std::mutex> lock(threads[i]->m);
					upload_tile(i, threads[i]->tile_index);
					threads[i]->status = RaytraceThread::ready_for_next;
				}
				threads[i]->cv.notify_one();
				
			} else if (threads[i]->status == RaytraceThread::uninitialized) {
				threads[i]->status = RaytraceThread::ready_for_next;
				threads[i]->cv.notify_one();
			}

		}

		if (finished_threads == threads.size()) {
			TRACE("Done!");
			pause_trace();
		}

#else
		if (rendered_tiles == tiles_X * tiles_Y) {
      TRACE("Done!");
      pause_trace();
			//upload_rows(0, height);

		} else {
			size_t X = rendered_tiles % tiles_X;
			size_t Y = rendered_tiles / tiles_X;

			raytrace_tile(0, rendered_tiles);
			upload_tile(0, rendered_tiles);

			rendered_tiles++;
		}

#endif

  }
  Drawable::update(elapsed);
}

void Pathtracer::draw() {
	
	// set fill
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // set shader
  glUseProgram(shader.id);

  // pass uniforms
  shader.set_parameters();

  // draw stuff
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  glUseProgram(0);
#if DEBUG
  GL_ERRORS();
#endif

  Drawable::draw();
}


// file that contains the actual pathtracing meat
#include "Pathtracer.inl"
