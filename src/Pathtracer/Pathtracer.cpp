#include "Pathtracer.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "BSDF.hpp"
#include "Triangle.hpp"
#include "Light.hpp"
#include <chrono>

#define DEBUG 1

Pathtracer::Pathtracer(
  size_t _width, 
  size_t _height, 
  std::string _name
  ) : width(_width), 
      height(_height), 
      Drawable(nullptr, _name) {

  float min_x = -0.96f; float min_y = -0.96f;
  float max_x = -0.1f; float max_y = -0.1f;

	num_threads = 1;
  pixels_per_frame = 3000;
	tile_size = 32;

	tiles_X = std::ceil(float(width) / float(tile_size));
	tiles_Y = std::ceil(float(height) / float(tile_size));

  image_buffer = new unsigned char[width * height * 3]; 
	subimage_buffers = new unsigned char*[num_threads];
	for (int i=0; i<num_threads; i++) {
		subimage_buffers[i] = new unsigned char[tile_size * tile_size * 3];
	}

  enabled = false;

  refresh_timer = 0.0f;
  refresh_interval = 0.0f;

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
	for (auto t : triangles) delete t;
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

			bool emissive = mesh->bsdf->is_emissive();
      for (int i=0; i<mesh->faces.size(); i+=3) {
				// loop and load triangles
        Vertex v1 = mesh->vertices[mesh->faces[i]];
        Vertex v2 = mesh->vertices[mesh->faces[i + 1]];
        Vertex v3 = mesh->vertices[mesh->faces[i + 2]];
				Triangle* T = new Triangle(mesh->object_to_world(), v1, v2, v3, mesh->bsdf);
        triangles.push_back(T);
				
				// also load as light if emissive
				if (emissive) {
					lights.push_back(static_cast<Light*>(new AreaLight(T)));
				}
      }
    }
  }

  TRACEF("loaded a scene with %d meshes, %d triangles, %d lights", 
			scene.children.size(), triangles.size(), lights.size());
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
  progress = 0;
	cumulative_render_time = 0.0f;
  uploaded_rows = 0;
  upload_rows(0, height);
}

void Pathtracer::set_mainbuffer_rgb(size_t i, vec3 rgb) {
#if DEBUG
  if (i >= width * height * 3) ERR("set_rgb indexing out of range!!");
#endif
  image_buffer[3 * i] = int(rgb.r * 255.0f);
  image_buffer[3 * i + 1] = int(rgb.g * 255.0f);
  image_buffer[3 * i + 2] = int(rgb.b * 255.0f);
}

void Pathtracer::set_subbuffer_rgb(size_t buf_i, size_t i, vec3 rgb) {
	unsigned char* buf = subimage_buffers[0];
  buf[3 * i] = int(rgb.r * 255.0f);
  buf[3 * i + 1] = int(rgb.g * 255.0f);
  buf[3 * i + 2] = int(rgb.b * 255.0f);
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

void Pathtracer::update(float elapsed) {
  if (!paused) {

		/*
    // trace N pixels and update progress (capped at num pixels total)
    size_t end = std::min(width * height, progress + pixels_per_frame);
    for (size_t i=progress; i<end; i++) {
      raytrace_pixel(i);
    }
    progress = end;
		*/
		for (size_t Y = 0; Y < tiles_Y; Y++) {
			for (size_t X = 0; X < tiles_X; X++) {
				raytrace_tile(X, Y);
			}
		}

		TRACE("(tracing tiles) done.");
		pause_trace();
		//upload_rows(0, height);

		/*
    // re-upload data each interval, or if pathtracing finished
    refresh_timer = std::min(refresh_interval, refresh_timer + elapsed);
    if (refresh_timer >= refresh_interval || progress == width * height) {

      size_t num_rows_to_upload = progress / width - uploaded_rows;
      if (num_rows_to_upload > 0) upload_rows(uploaded_rows, num_rows_to_upload);

      uploaded_rows += num_rows_to_upload;

      refresh_timer = 0.0f;
    }

    // determine if finished
    if (progress == width * height) {
      TRACE("Done!");
      pause_trace();
    }
		*/
    
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
