#pragma once
#include "SceneObject.hpp"
#include "Utils/myn/Misc.h"

class Scene;
struct Camera;
struct Mesh;
namespace tinygltf { struct Light; }

#define PBR_WATTS_TO_LUMENS 683.0f

struct Light : public SceneObject {

	enum Type { Point, Directional };
	Type type;
	~Light() override = default;
	glm::vec3 getLumen() { return color * strength; }

protected:
	glm::vec3 color;
	float strength; // power (Watts) updated in blender 3.6 onward: (cd, lx, nt)

};

struct DirectionalLight : public Light {

	explicit DirectionalLight(
			glm::vec3 _color = glm::vec3(1),
			float _intensity = 1.0f,
			glm::vec3 dir = glm::vec3(0, 0, -1),
			const std::string& _name = "[anonymous directional light]");

	explicit DirectionalLight(const std::string& node_name, const tinygltf::Light* in_light);

	~DirectionalLight() override;

	void setDirection(glm::vec3 dir) { setRotation(myn::quat_from_dir(normalize(dir))); }

	glm::vec3 getLightDirection() { return object_to_world_rotation() * glm::vec3(0, 0, -1); }

	glm::vec3 getIrradianceLx() { return getLumen() / (2 * PI) / PBR_WATTS_TO_LUMENS; }

	static DirectionalLight* getSun() { return sun; }

	// scene object functions

	void setRotation(glm::quat newRot) override;

#if GRAPHICS_DISPLAY
	void update(float elapsed) override;

	void drawConfigUI() override;
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

	glm::vec3 getLuminousIntensityCd() { return getLumen() / (4 * PI) / PBR_WATTS_TO_LUMENS; }

};
