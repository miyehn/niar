#pragma once

#include "lib.h"
#include <functional>
#include <unordered_map>

// took from: https://learnopengl.com/Getting-started/Shaders
struct Shader {

  static Shader Basic;
	static Shader DeferredBasePass;
	static Shader ShadowPassDirectional;
	static Shader ShadowPassPoint;

	//------

  Shader(
      const std::string& vert_path, 
      const std::string& frag_path, 
      const std::string& tesc_path = "",
      const std::string& tese_path = "");
	virtual ~Shader() {} // a dummy, just to make this class polymorphic..
	static Shader* get(const std::string& name);
	static void add(const std::string& name, Shader* shader);
	static void cleanup();

  std::function<void()> set_static_parameters = [this]() {
    WARNF("%s: using shader without static parameter function set!", name.c_str());
  };

private:
	static std::unordered_map<std::string, Shader*> shader_pool;

protected:
	// should likely only be called from Blit; uses a singleton vertex shader to draw full-screen quad
	Shader(const std::string& frag_path);

public:

  Shader() {
		id = 0; 
		name = "[unamed shader]";
	}

  std::function<void()> set_parameters = [this]() {
    WARNF("%s: using shader without parameter function set!", name.c_str());
  };
  std::string name;
  uint id;

  //--------

  void set_bool(const std::string &name, bool value, bool optional=false) const;

  void set_int(const std::string &name, int value, bool optional=false) const;

  void set_float(const std::string &name, float value, bool optional=false) const;

  void set_vec2(const std::string &name, vec2 v, bool optional=false) const;

  void set_vec3(const std::string &name, vec3 v, bool optional=false) const;

  void set_vec4(const std::string &name, vec4 v, bool optional=false) const;

  void set_mat3(const std::string &name, mat3 mat, bool optional=false) const;

  void set_mat4(const std::string &name, mat4 mat, bool optional=false) const;

  void set_tex2D(const std::string &name, uint texture_unit, uint texture_id, bool optional=false) const;

  void set_texCube(const std::string &name, uint texture_unit, uint texture_id, bool optional=false) const;

private:
  int uniform_loc(const std::string& uniformName, bool optional=false) const;

	static uint quad_vert; // vertex shader for drawing a quad

};

struct Blit : Shader {

	static Blit* copy_debug();
	static Blit* shadow_mask_directional();
	static Blit* shadow_mask_point();

	Blit(const std::string& shader_name);

	void begin_pass();
	void end_pass();

private:
	static uint vao;
	static uint vbo;
	GLboolean cached_depth_test;

	static Blit* copy_debug_value;
	static Blit* shadow_mask_directional_value;
	static Blit* shadow_mask_point_value;

};
