#pragma once
#include "Drawable.hpp"

struct Scene;
struct Camera;
struct Mesh;

struct Light : public Drawable {

	static void set_directional_shadowpass_params_for_mesh(Mesh* mesh, int shader_index);
	static void set_point_shadowpass_params_for_mesh(Mesh* mesh, int shader_index);

	enum Type { Point, Directional };
	Type type;
	virtual ~Light();
	vec3 get_emission() { return color * intensity; }
	virtual void render_shadow_map() = 0;
	virtual mat4 world_to_light_clip() = 0;

	virtual void set_cast_shadow(bool cast) = 0;
	bool get_cast_shadow() { return cast_shadow; }

	uint get_shadow_map(){ return shadow_map_tex; }
	uint get_shadow_mask(){ return shadow_mask_tex; }

protected:
	vec3 color;
	float intensity;

	bool cast_shadow = false;
	bool shadow_map_initialized = false;
	uint shadow_map_dim;

	float effective_radius;

	//-------- opengl stuff --------
	uint shadow_map_tex = 0; // GL_TEXTURE_2D || GL_TEXTURE_CUBE_MAP
	Camera* shadow_map_cam = nullptr;
	uint shadow_mask_tex = 0;

};

struct DirectionalLight : public Light {

	DirectionalLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 dir = vec3(0, 0, -1));

	virtual ~DirectionalLight();

	virtual void set_cast_shadow(bool cast);

	void set_direction(vec3 dir) { rotation = quat_from_dir(normalize(dir)); }

	vec3 get_direction() { return object_to_world_rotation() * vec3(0, 0, -1); }

	virtual void render_shadow_map();

	virtual mat4 world_to_light_clip();

private:

	uint shadow_map_fbo = 0;

	Blit* shadow_map_blit = nullptr;

};

struct PointLight: public Light {

	PointLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 _local_pos = vec3(0));

	virtual ~PointLight();

	virtual void render_shadow_map();

	virtual void set_cast_shadow(bool cast);

	// since point lights don't need this at all
	virtual mat4 world_to_light_clip() { return mat4(1); }

private:

	uint shadow_map_fbos[6];
	vec3 shadow_map_normals[6] = {
		vec3(1, 0, 0), vec3(-1, 0, 0),
		vec3(0, 1, 0), vec3(0, -1, 0),
		vec3(0, 0, 1), vec3(0, 0, -1)
	};
	uint shadow_map_depth_rbo = 0;

	void set_location_to_all_shaders(Scene* scene, int shader_index);

};
