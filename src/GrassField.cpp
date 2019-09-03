#include "GrassField.hpp"

GrassField::GrassField(Camera* camera, uint num_blades): GameObject(camera) {

  // create blades
  blades = vector<Blade>();
  for (uint i=0; i<num_blades; i++) {
    Blade b = Blade(vec3(-10.0f + i*3.0f, 0.0f, 0.0f));
    blades.push_back(b);
  }  

  // allocate render buffer
  render_buffer = new float[16 * num_blades];

  { // create vao and shaders
    // generate 1 vbo
    glGenBuffers(1, &vbo);
    // generate 1 vao, bind it (start refering to it)
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // bind vbo to current vao (start refering to it)
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // vertex attrib pointers:
    glVertexAttribPointer(
        0, // index of ptr (attribute)
        4, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized
        4*sizeof(float), // stride size
        (void*)0 // offset
    );
    glEnableVertexAttribArray(0); // enable vertex attrib ptr at index 0

    // specify patch size for tesselation
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    // done refering to vbo and vao, unbind them
    glBindBuffer(vbo, 0);
    glBindVertexArray(0);

    // shaders
    shader = newShaderProgram(
        "../shaders/grass.vert",
        "../shaders/grass.tesc",
        "../shaders/grass.tese",
        "../shaders/grass.frag");
  }

}

GrassField::~GrassField() {
  delete(render_buffer); 
  glDeleteBuffers(1, &vbo);
  vbo = 0;
  glDeleteVertexArrays(1, &vao);
  vao = 0;
}

void GrassField::update(float time_elapsed) {
}

bool GrassField::handle_event(SDL_Event event) {
}

void GrassField::draw() {

  // assemble buffer to upload into render_buffer
  assemble_buffer();

  // upload render_buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 16 * blades.size() * sizeof(float), render_buffer, GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // set shader
  glUseProgram(shader);

  // upload transformation to shader
  mat4 transformation = camera->obj_to_screen() * this->transformation;
  glUniformMatrix4fv(unifLoc(shader, "transformation"), 1, GL_FALSE, value_ptr(transformation));

  // use vao
  glBindVertexArray(vao);

  // draw.
  glDrawArrays(GL_PATCHES, 0, 4 * blades.size());

  // unbind vao
  glBindVertexArray(0);

  // reset shader
  glUseProgram(0);
  
}

void GrassField::assemble_buffer() {
  size_t patch_size = 16;
  for (size_t i=0; i<blades.size(); i++) {
    render_buffer[i*patch_size + 0] = blades[i].root.x;
    render_buffer[i*patch_size + 1] = blades[i].root.y;
    render_buffer[i*patch_size + 2] = blades[i].root.z;
    render_buffer[i*patch_size + 3] = blades[i].width;

    render_buffer[i*patch_size + 4] = blades[i].above.x;
    render_buffer[i*patch_size + 5] = blades[i].above.y;
    render_buffer[i*patch_size + 6] = blades[i].above.z;
    render_buffer[i*patch_size + 7] = blades[i].height;

    render_buffer[i*patch_size + 8] = blades[i].ctrl.x;
    render_buffer[i*patch_size + 9] = blades[i].ctrl.y;
    render_buffer[i*patch_size + 10] = blades[i].ctrl.z;
    render_buffer[i*patch_size + 11] = blades[i].stiffness;

    render_buffer[i*patch_size + 12] = blades[i].up.x;
    render_buffer[i*patch_size + 13] = blades[i].up.y;
    render_buffer[i*patch_size + 14] = blades[i].up.z;
    render_buffer[i*patch_size + 15] = blades[i].orientation;
  }
}
