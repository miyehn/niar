#include "Light.hpp"
#include "Engine/ConfigAsset.hpp"

#include <tinygltf/tiny_gltf.h>

using namespace glm;

DirectionalLight::DirectionalLight(vec3 _color, float _intensity, vec3 dir)
{
	type = Directional;

	color = _color;
	intensity = _intensity;
	set_direction(dir);

	name = "[unnamed directional light]";
}

DirectionalLight::DirectionalLight(const std::string& node_name, const tinygltf::Light *in_light)
{
	name = node_name + " | " + in_light->name;
	auto c = in_light->color;
	color = vec3(c[0], c[1], c[2]);
	intensity = in_light->intensity;
}

//-------- point light --------

PointLight::PointLight(vec3 _color, float _intensity, vec3 _local_pos)
{
	type = Point;

	color = _color;
	intensity = _intensity;

	if (color.r > 1 || color.g > 1 || color.b > 1) {
		float factor = max(color.r, max(color.g, color.b));
		color *= (1.0f / factor);
		intensity = factor;
	}

	_local_position = _local_pos;

	name = "[unnamed point light]";
}

PointLight::PointLight(const std::string& node_name, const tinygltf::Light *in_light)
{
	name = node_name + " | " + in_light->name;
	auto c = in_light->color;
	color = vec3(c[0], c[1], c[2]);
	intensity = in_light->intensity;
}

