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

	Shader(const std::string& frag_path);

  std::function<void()> set_parameters = []() {
    WARN("using a shader without parameter function set!");
  };
  std::string name;
  uint id = 0;

  //--------

  void set_bool(const std::string &name, bool value) const;

  void set_int(const std::string &name, int value) const;

  void set_float(const std::string &name, float value) const;

  void set_vec2(const std::string &name, vec2 v) const;

  void set_vec3(const std::string &name, vec3 v) const;

  void set_vec4(const std::string &name, vec4 v) const;

  void set_mat3(const std::string &name, mat3 mat) const;

  void set_mat4(const std::string &name, mat4 mat) const;

  void set_tex2D(uint texture_unit, uint texture_id);

private:
  uint uniform_loc(const std::string& uniformName) const;

	uint quad_vert = 0;

};

struct Blit {

	Blit(const std::string& frag_path);

	Shader shader;

	void draw();

private:
	static uint vao;
	static uint vbo;

};
