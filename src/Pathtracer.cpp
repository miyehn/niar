#include "Pathtracer.hpp"

#define DEBUG 1

Pathtracer::Pathtracer(
  size_t _width, 
  size_t _height, 
  std::string _name
  ) : width(_width), 
      height(_height), 
      Drawable(nullptr, _name) {

  min_x = -0.96f; min_y = -0.96f;
  max_x = -0.1f; max_y = -0.1f;

  size_t num_bytes = width * height * 3;
  image_buffer = new unsigned char[num_bytes]; 
  memset(image_buffer, 40, num_bytes);

  enabled = false;
  paused = true;

  pixels_per_frame = 2000;
  progress = 0;

  refresh = false;
  refresh_timer = 0.0f;
  refresh_interval = 0.5f;

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

  LOG("initialized pathtracer");
}

Pathtracer::~Pathtracer() {
  glDeleteTextures(1, &texture);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  delete image_buffer;
#if DEBUG
  GL_ERRORS();
#endif
  LOG("deleted pathtracer");
}

bool Pathtracer::handle_event(SDL_Event event) {
  if (event.type==SDL_KEYUP && 
      event.key.keysym.sym==SDLK_p) {
    if (paused) continue_trace();
    else pause_trace();
    return true;
  }

  return Drawable::handle_event(event);
}

void Pathtracer::enable() {
  LOG("pathtracer enabled");
  Drawable::enable();
}

void Pathtracer::disable() {
  LOG("pathtracer disabled");
  Drawable::disable();
}

void Pathtracer::pause_trace() {
  LOG("pause trace");
  refresh = true;
  paused = true;
}

void Pathtracer::continue_trace() {
  LOG("continue trace");
  paused = false;
}

void Pathtracer::set_rgb(size_t w, size_t h, vec3 rgb) {
}

void Pathtracer::set_rgb(size_t i, vec3 rgb) {
#if DEBUG
  if (i >= width * height * 3) ERR("set_rgb index out of range!!");
#endif
  image_buffer[3 * i] = int(rgb.r * 255.0f);
  image_buffer[3 * i + 1] = int(rgb.g * 255.0f);
  image_buffer[3 * i + 2] = int(rgb.b * 255.0f);
}

void Pathtracer::update(float elapsed) {
  if (!paused) {

    // trace N pixels
    size_t end = std::min(width * height, progress + pixels_per_frame);
    for (size_t i=progress; i<end; i++) {
      set_rgb(i, vec3(0.4f, 0.5f, 0.6f));
    }
    progress = end;

    // determine if finished
    if (progress == width * height) {
      pause_trace();
      progress = 0;
    }
    
    refresh_timer = std::min(refresh_interval, refresh_timer + elapsed);
    if (refresh_timer >= refresh_interval) {
      refresh = true;
      refresh_timer = 0.0f;
    }

  }
  Drawable::update(elapsed);
}

void Pathtracer::draw() {

  // set shader
  glUseProgram(shader.id);

  // upload uniforms
  shader.set_parameters();
  // re-upload data if things are updated
  if (refresh) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_buffer);
    glBindTexture(GL_TEXTURE_2D, 0);

    WARNF("refresh! progress: %d", progress);
    refresh = false;
  }

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
