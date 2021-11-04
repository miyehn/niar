#pragma once
#include "Engine/SceneObject.hpp"
#include "Utils/myn/Misc.h"

struct Scene;
struct Camera;
struct Mesh;
struct aiLight;
struct aiNode;
struct MatGeneric;

struct Light : public SceneObject {

	enum Type { Point, Directional };
	Type type;
	~Light() override = default;
	glm::vec3 get_emission() { return color * intensity; }

	void set_local_position(glm::vec3 _local_position) override { local_position_value = _local_position; }
	void set_rotation(glm::quat _rotation) override { rotation_value = _rotation; }
	void set_scale(glm::vec3 _scale) override { scale_value = _scale; }

protected:
	glm::vec3 color;
	float intensity;

};

struct DirectionalLight : public Light {

	explicit DirectionalLight(
			glm::vec3 _color = glm::vec3(1),
			float _intensity = 1.0f, 
			glm::vec3 dir = glm::vec3(0, 0, -1));

	explicit DirectionalLight(aiLight* light);

	void set_direction(glm::vec3 dir) { set_rotation(myn::quat_from_dir(normalize(dir))); }

	glm::vec3 get_direction() { return object_to_world_rotation() * glm::vec3(0, 0, -1); }

};

struct PointLight: public Light {

	explicit PointLight(
			glm::vec3 _color = glm::vec3(1),
			float _intensity = 1.0f, 
			glm::vec3 _local_pos = glm::vec3(0));

	explicit PointLight(aiLight* light);

};
