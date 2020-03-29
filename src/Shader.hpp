#pragma once

#include "lib.h"

// took from: https://learnopengl.com/Getting-started/Shaders
struct Shader {

  static Shader Basic;

  Shader() {}
  Shader(
      const std::string& vert_path, 
      const std::string& frag_path, 
      const std::string& tesc_path = "",
      const std::string& tese_path = "");

  std::function<void()> set_parameters = []() {
    WARN("using a shader without parameter function set!");
  };
  std::string name;
  uint id = 0;

  //--------

  void set_bool(const std::string &name, bool value) const {       
    glUniform1i(uniform_loc(name), (int)value); 
  }

  void set_int(const std::string &name, int value) const {
    glUniform1i(uniform_loc(name), value); 
  }

  void set_float(const std::string &name, float value) const { 
    glUniform1f(uniform_loc(name), value); 
  }

  void set_vec2(const std::string &name, vec2 v) const { 
    glUniform2f(uniform_loc(name), v.x, v.y); 
  }

  void set_vec3(const std::string &name, vec3 v) const { 
    glUniform3f(uniform_loc(name), v.x, v.y, v.z); 
  }

  void set_vec4(const std::string &name, vec4 v) const { 
    glUniform4f(uniform_loc(name), v.x, v.y, v.z, v.w); 
  }

  void set_mat4(const std::string &name, mat4 mat) const {
    glUniformMatrix4fv(uniform_loc(name), 1, GL_FALSE, value_ptr(mat));
  }

private:
  uint uniform_loc(const std::string& uniformName) const;

};

