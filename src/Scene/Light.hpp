#pragma once
#include "SceneObject.hpp"
#include "Utils/myn/Misc.h"

class Scene;
struct Camera;
struct Mesh;
namespace tinygltf { struct Light; }

struct Light : public SceneObject {

	enum Type { Point, Directional };
	Type type;
	~Light() override = default;
	glm::vec3 get_emission() { return color * intensity; }

protected:
	glm::vec3 color;
	float intensity; // power (Watts) updated in blender 3.6 onward: (cd, lx, nt)

};

struct DirectionalLight : public Light {

	explicit DirectionalLight(
			glm::vec3 _color = glm::vec3(1),
			float _intensity = 1.0f,
			glm::vec3 dir = glm::vec3(0, 0, -1),
			const std::string& _name = "[anonymous directional light]");

	explicit DirectionalLight(const std::string& node_name, const tinygltf::Light* in_light);

	~DirectionalLight() override;

	void set_direction(glm::vec3 dir) { set_rotation(myn::quat_from_dir(normalize(dir))); }

	glm::vec3 get_light_direction() { return object_to_world_rotation() * glm::vec3(0, 0, -1); }

	static DirectionalLight* get_sun() { return sun; }

	// scene object functions

	void set_rotation(glm::quat newRot) override;

#if GRAPHICS_DISPLAY
	void update(float elapsed) override;

	void draw_config_ui() override;
#endif

private:

	float angleTheta = 0.0f;
	float anglePhi = 90.0f;

	static DirectionalLight* sun;
};

struct PointLight: public Light {

	explicit PointLight(
			glm::vec3 _color = glm::vec3(1),
			float _intensity = 1.0f, 
			glm::vec3 _local_pos = glm::vec3(0));

	explicit PointLight(const std::string& node_name, const tinygltf::Light* in_light);

};
