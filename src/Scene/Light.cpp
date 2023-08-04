#include "Light.hpp"
#include "Assets/ConfigAsset.hpp"
#if GRAPHICS_DISPLAY
#include "imgui.h"
#endif

#include <tinygltf/tiny_gltf.h>

using namespace glm;

DirectionalLight* DirectionalLight::sun = nullptr;

DirectionalLight::DirectionalLight(vec3 _color, float _intensity, vec3 dir, const std::string& _name)
{
	type = Directional;

	color = _color;
	strength = _intensity;
	setDirection(dir);

	name = _name;
#if GRAPHICS_DISPLAY
	ui_show_transform = false;
#endif

	if (myn::lower(name) == "sun") {
		if (!sun) {
			sun = this;
		} else {
			WARN("there are multiple directional lights called 'sun' in the scene. Only the first one created is set as sun")
		}
	}
}

DirectionalLight::DirectionalLight(const std::string& node_name, const tinygltf::Light *in_light) {
	type = Directional;
	name = node_name + " | " + in_light->name;
	auto c = in_light->color;
	color = vec3(c[0], c[1], c[2]);
	strength = in_light->intensity;

#if GRAPHICS_DISPLAY
	ui_show_transform = false;
#endif

	if (myn::lower(in_light->name) == "sun") {
		if (!sun) {
			sun = this;
		} else {
			WARN("there are multiple directional lights called 'sun' in the scene. Only the first one created is set as sun")
		}
	}
}

DirectionalLight::~DirectionalLight() noexcept {
	if (sun == this) sun = nullptr;
}

#if GRAPHICS_DISPLAY
void DirectionalLight::update(float elapsed) {
	//vec3
	const float deg2rad = 3.14159265f / 180;
	float theta = angleTheta * deg2rad;
	float phi = anglePhi * deg2rad;
	vec3 newDir2sun = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));

	// not modifying phi and theta
	SceneObject::setRotation(myn::quat_from_dir(-newDir2sun));

	SceneObject::update(elapsed);
}
#endif

// SceneObject interface
void DirectionalLight::setRotation(glm::quat newRot) {
	Light::setRotation(newRot);

	// update theta and phi here, since for most of the time they're the rotation ground truth
	const float rad2deg = 180.0f / 3.14159265f;
	vec3 dir2sun = -getLightDirection();
	anglePhi = 180.0f - glm::acos(-dir2sun.z) * rad2deg;

	dir2sun.z = 0;
	dir2sun = glm::normalize(dir2sun);
	angleTheta = glm::atan(dir2sun.y, dir2sun.x) * rad2deg;

}

#if GRAPHICS_DISPLAY
void DirectionalLight::drawConfigUI() {
	ImGui::SliderFloat("angle theta", &angleTheta, -180.0f, 180.0f);
	ImGui::SliderFloat("angle phi", &anglePhi, 0.0f, 180.0f);
}
#endif

//-------- point light --------

PointLight::PointLight(vec3 _color, float _intensity, vec3 _local_pos)
{
	type = Point;

	color = _color;
	strength = _intensity;

	if (color.r > 1 || color.g > 1 || color.b > 1) {
		float factor = max(color.r, max(color.g, color.b));
		color *= (1.0f / factor);
		strength = factor;
	}

	_local_position = _local_pos;

	name = "[unnamed point light]";
}

PointLight::PointLight(const std::string& node_name, const tinygltf::Light *in_light)
{
	name = node_name + " | " + in_light->name;
	auto c = in_light->color;
	color = vec3(c[0], c[1], c[2]);
	strength = in_light->intensity;
}

