#pragma once

#include "lib.h"

// took from: https://learnopengl.com/Getting-started/Shaders
struct Shader {

  static Shader Basic;
	static Shader DeferredBasePass;
	static Shader DepthOnly;
	static Shader ShadowPass;

  Shader() { 
		id = 0; 
		name = "[unamed shader]";
	}
  Shader(
      const std::string& vert_path, 
      const std::string& frag_path, 
      const std::string& tesc_path = "",
      const std::string& tese_path = "");

	// should likely only be called from Blit; uses a singleton vertex shader to draw full-screen quad
	Shader(const std::string& frag_path);

  std::function<void()> set_parameters = [this]() {
    WARNF("%s: using shader without parameter function set!", name.c_str());
  };
  std::string name;
  uint id;

  //--------

  void set_bool(const std::string &name, bool value) const;

  void set_int(const std::string &name, int value) const;

  void set_float(const std::string &name, float value) const;

  void set_vec2(const std::string &name, vec2 v) const;

  void set_vec3(const std::string &name, vec3 v) const;

  void set_vec4(const std::string &name, vec4 v) const;

  void set_mat3(const std::string &name, mat3 mat) const;

  void set_mat4(const std::string &name, mat4 mat) const;

  void set_tex2D(const std::string &name, uint texture_unit, uint texture_id) const;

  void set_texCube(const std::string &name, uint texture_unit, uint texture_id) const;

private:
  int uniform_loc(const std::string& uniformName) const;

	static uint quad_vert; // vertex shader for drawing a quad

};

struct Blit {

	static Blit* CopyDebug;

	Blit(const std::string& frag_path);

	Shader shader;

	void begin_pass();
	void end_pass();

private:
	static uint vao;
	static uint vbo;
	GLboolean cached_depth_test;

};
