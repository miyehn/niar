#include "GrassField.hpp"
#include "data_path.hpp"

GrassField::GrassField(Camera* camera): GameObject(camera) {

  { // init data
    for (int i=0; i<num_blades; i++) {
      blades.emplace_back();
    }
    img_buffer_pixels = (GLsizei)(blades.size() * sizeof(Blade) / 4);
    read_back = vector<float>(img_buffer_pixels, 0.0f);
    num_workgroups = (GLsizei)(blades.size() / work_group_size);
  }

  { // init for gl compute shader

    // initialize both textures with initial blades data
    tex_r = newTexture1D((void*)blades.data(), img_buffer_pixels);
    tex_w = newTexture1D((void*)blades.data(), img_buffer_pixels);
    GL_ERRORS();

    comp_shader_prog = newComputeShaderProgram(data_path("../shaders/grass.comp"));
  }

  { // init for gl draw

    // create shader program
    draw_shader_prog = newShaderProgram(
        data_path("../shaders/grass.vert"),
        data_path("../shaders/grass.tesc"),
        data_path("../shaders/grass.tese"),
        data_path("../shaders/grass.frag"));

    // create vao and use it
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // create vbo for this vao
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // set vertex attrib pointer and use it
    glVertexAttribPointer(
        0, // input position within shader
        4, // size of that input position (1, 2, 3 or 4)
        GL_FLOAT, // type of input
        GL_FALSE, // normalized
        sizeof(vec4), // stride size, could be 0 if tightly packed
        (GLbyte*)0 // offset
        );
    glEnableVertexAttribArray(0);

    // specify patch size for tessellation
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    // done referring to vbo and vao
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    GL_ERRORS();

  }
}

GrassField::~GrassField(){
  // delete any used buffers and textures
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteTextures(1, &tex_r);
  glDeleteTextures(1, &tex_w);
  GL_ERRORS();
}

void GrassField::update(float time_elapsed) {

  time += time_elapsed;

  // swap the two textures
  uint textmp = tex_r;
  tex_r = tex_w;
  tex_w = textmp;

  // re-bind them to shader input locations
  glBindImageTexture(0, tex_r, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
  glBindImageTexture(1, tex_w, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

  // use compute shader
  glUseProgram(comp_shader_prog);
  // set time uniform
  glUniform2f(unifLoc(comp_shader_prog, "time"), time, time_elapsed);
  // suppose want to draw 8 blades: work group size = 4, dispatch 2 work groups total to draw them.
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
  glDispatchCompute(num_workgroups, 1, 1); // arguments: X, Y, Z (how many work groups in each dimension)
  GL_ERRORS();

  // read the results back into blades
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
  glBindTexture(GL_TEXTURE_1D, tex_w);
  glGetTexImage(GL_TEXTURE_1D, 0, GL_RED, GL_FLOAT, blades.data());
  glBindTexture(GL_TEXTURE_1D, 0);

  // uncomment below to debug
  // for (auto b : blades) { cout << b.str() << endl; }
  // cout << "------------" << endl << endl;

}

bool GrassField::handle_event(SDL_Event event) {
  if (event.type==SDL_MOUSEMOTION && (event.motion.state&SDL_BUTTON_LMASK)) {
    float dx = (float)event.motion.xrel;
    this->transformation = glm::rotate(this->transformation, radians(dx), vec3(0, 0, 1));
    return true;
  }
  return false;
}

void GrassField::draw() {

  // upload buffer data
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, num_blades * sizeof(Blade), blades.data(), GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // set draw shader
  glUseProgram(draw_shader_prog);

  // upload transformation to shader
  mat4 transformation = camera->obj_to_screen() * this->transformation;
  glUniformMatrix4fv(unifLoc(draw_shader_prog, "transformation"), 1, GL_FALSE, value_ptr(transformation));

  // use vao to interpret uploaded data
  glBindVertexArray(vao);

  // draw!
  glDrawArrays(GL_PATCHES, 0, 4 * (GLsizei)num_blades);

  // unbind vao
  glBindVertexArray(0);

  // reset shader
  glUseProgram(0);

  GL_ERRORS();

}

Blade::Blade(float x, float y) {
  vec3 root = vec3(x, y, 0);

  float angle = rand01() * 360;
  this->up_o = vec4(0.0f, 0.0f, 1.0f, radians(angle));

  float width = 0.4f + rand01() * 0.35f;
  this->root_w = vec4(root.x, root.y, root.z, width);

  float height = 3.0f + rand01() * 2.0f;
  this->above_h = this->up_o * 4.0f;
  this->above_h.w = height;

  float stiffness = 0.7f + rand01() * 0.2f;
  this->ctrl_s = above_h + vec4(0.0f, 0.0f, 0.0f, stiffness);

}

