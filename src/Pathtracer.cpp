#include "Pathtracer.hpp"

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

  image_buffer = new unsigned char[width * height * 3]; 

  enabled = false;

  pixels_per_frame = 2000;
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

  //------------------------------------

  reset();
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
  paused = true;
}

void Pathtracer::continue_trace() {
  LOG("continue trace");
  paused = false;
}

void Pathtracer::reset() {
  LOG("reset pathtracer");
  memset(image_buffer, 40, width * height * 3);
  paused = true;
  progress = 0;
  uploaded_rows = 0;
  upload_rows(0, height);
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

void Pathtracer::upload_rows(GLint begin, GLsizei rows) {
  LOGF("begin, num rows: %d, %d", begin, rows);
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
  WARNF("refresh! updated %d rows, %d%% done.", rows, percentage);
  GL_ERRORS();
}

void Pathtracer::update(float elapsed) {
  if (!paused) {

    // trace N pixels and update progress (capped at num pixels total)
    size_t end = std::min(width * height, progress + pixels_per_frame);
    for (size_t i=progress; i<end; i++) {
      set_rgb(i, vec3(0.4f, 0.5f, 0.7f));
    }
    progress = end;

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
      WARN("Done!");
      pause_trace();
      progress = 0;
    }
    
  }
  Drawable::update(elapsed);
}

void Pathtracer::draw() {

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
