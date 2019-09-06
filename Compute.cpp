#include "Compute.hpp"

GLsizei num_floats = 16;

Compute::Compute(Camera* camera): GameObject(camera) {
  { // init for comp shader
    read_buf = vector<float>(num_floats, 0.1f);
    read_buf[3] = 1.0f;
    write_buf = vector<float>(num_floats, 0.7f);

    read_tex = newTexture1D(read_buf.data(), num_floats);
    write_tex = newTexture1D(write_buf.data(), num_floats);

    comp_shader_prog = newComputeShaderProgram("shaders/test.comp");
  }

  draw_shader_prog = newVFShaderProgram("shaders/test.vert", "shaders/test.frag");

  vertices = vector<vec3>();
  vertices.push_back(vec3(-1.0f, 0.0f, 0.0f));
  vertices.push_back(vec3(1.0f, 0.0f, 0.0f));
  vertices.push_back(vec3(0.0f, 0.0f, 2.0f));

  { // init for draw
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
        0, // position?
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(vec3),
        (GLbyte*)0 );
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    GL_ERRORS();
  }
}

Compute::~Compute() {
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteTextures(1, &read_tex);
  glDeleteTextures(1, &write_tex);
  GL_ERRORS();
}

void Compute::update(float time_elapsed) {

  /*
  // swap read and write buffers
  vector<float> tmp = read_buf;
  read_buf = write_buf;
  write_buf = tmp;
   */

  glBindImageTexture(0, read_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
  glBindImageTexture(1, write_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

  glUseProgram(comp_shader_prog);
  glDispatchCompute(16/4, 1, 1);

}

bool Compute::handle_event(SDL_Event event) {
  return false;
}

void Compute::draw() {
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glUseProgram(draw_shader_prog);
  // upload transformation to shader
  mat4 transformation = camera->obj_to_screen() * this->transformation;
  glUniformMatrix4fv(unifLoc(draw_shader_prog, "transformation"), 1, GL_FALSE, value_ptr(transformation));

  glBindVertexArray(vao);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glBindImageTexture(0, write_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

  glMemoryBarrier(GL_ALL_BARRIER_BITS);
  glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

  GL_ERRORS();
}
