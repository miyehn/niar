#include "Compute.hpp"

Compute::Compute(Camera* camera): GameObject(camera) {
  { // init for compute shader
    vector<float> buf1 = vector<float>(num_floats, 0.1f);
    vector<float> buf2 = vector<float>(num_floats, 1.0f);

    tex_r = newTexture1D(buf1.data(), num_floats);
    tex_w = newTexture1D(buf2.data(), num_floats);

    comp_shader_prog = newComputeShaderProgram("shaders/test.comp");
  }

  { // init for draw

    // create shader program
    draw_shader_prog = newVFShaderProgram("shaders/test.vert", "shaders/test.frag");

    // create vao and use it
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // create vbo for this vao
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // create and upload data to this vbo.
    // IMPORTANT: if data gets updated from CPU, need to upload again in draw().
    vector<vec3> vertices = vector<vec3>(num_vertices * 20);
    vertices[0] = vec3(-1.0f, 0.0f, 0.0f);
    vertices[1] = vec3(1.0f, 0.0f, 0.0f);
    vertices[2] = vec3(0.0f, 0.0f, 2.0f);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);

    // set vertex attrib pointer and use it
    glVertexAttribPointer(
        0, // input position within shader
        3, // size of that input position (1, 2, 3 or 4)
        GL_FLOAT, // type of input
        GL_FALSE, // normalized
        sizeof(vec3), // stride size, could be 0 if tightly packed
        (GLbyte*)0 // offset
        );
    glEnableVertexAttribArray(0);

    // done referring to vbo and vao
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GL_ERRORS();
  }
}

Compute::~Compute() {
  // delete any used buffers and textures
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteTextures(1, &tex_r);
  glDeleteTextures(1, &tex_w);
  GL_ERRORS();
}

void Compute::update(float time_elapsed) {

  // swap the two textures
  uint textmp = tex_r;
  tex_r = tex_w;
  tex_w = textmp;

  // re-bind them to shader input locations
  glBindImageTexture(0, tex_r, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
  glBindImageTexture(1, tex_w, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

  // run compute shader
  glUseProgram(comp_shader_prog);
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
  glDispatchCompute(num_floats / work_group_size, 1, 1); // arguments: X, Y, Z (how many work groups in each dimension)

}

bool Compute::handle_event(SDL_Event event) {
  return false;
}

void Compute::draw() {
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(draw_shader_prog);
  // upload transformation to shader
  mat4 transformation = camera->obj_to_screen() * this->transformation;
  glUniformMatrix4fv(unifLoc(draw_shader_prog, "transformation"), 1, GL_FALSE, value_ptr(transformation));

  glBindVertexArray(vao);

  // what gets read in the shader is an array of length num_floats
  // each is a vec4, with only its R component contains value.
  glBindImageTexture(0, tex_w, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

  // draw!
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, num_vertices);

  GL_ERRORS();
}
