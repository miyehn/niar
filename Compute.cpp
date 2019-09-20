#include "Compute.hpp"
#include "data_path.hpp"

Compute::Compute(Camera* camera): GameObject(camera) {

  { // init for data
    for (int i=0; i<8; i++) {
      blades.emplace_back(vec3(-10.0f + i*3.0f, 0.0f, 0.0f));
    }
    img_buffer_len = (GLsizei)(blades.size() * sizeof(Blade));
    num_workgroups = (GLsizei)(blades.size() / work_group_size);
  }

  { // init for gl compute shader

    // initialize both textures with initial blades data
    tex_r = newTexture1D((void*)blades.data(), img_buffer_len);
    tex_w = newTexture1D((void*)blades.data(), img_buffer_len);

    comp_shader_prog = newComputeShaderProgram(data_path("../shaders/test.comp"));
  }

  { // init for gl draw

    // create shader program
    draw_shader_prog = newVFShaderProgram(data_path("../shaders/test.vert"), data_path("../shaders/test.frag"));

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
  // work group size = 4, dispatch 2 work groups total to draw 8 blades.
  glDispatchCompute(num_workgroups, 1, 1); // arguments: X, Y, Z (how many work groups in each dimension)
  GL_ERRORS();

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

  // what gets read in the shader is an array of length img_buffer_len == blades.size() * sizeof(Blade)
  // each is a vec4, with only its R component contains value.
  glBindImageTexture(0, tex_w, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

  // draw!
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, num_vertices);

  GL_ERRORS();
}

Blade::Blade(vec3 root) {
  this->up_o = vec4(0.0f, 0.0f, 1.0f, radians(65.0f));

  this->root_w = vec4(root.x, root.y, root.z, 0.65f);

  this->above_h = this->up_o * 4.0f;
  this->above_h.w = 1.0f;

  this->ctrl_s = above_h + vec4(-0.8f, 0.0f, 0.0f, 0.0f);
  this->ctrl_s.w = 1.0f;
}
