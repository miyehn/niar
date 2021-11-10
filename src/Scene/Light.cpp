#include "Light.hpp"
#include "Engine/Config.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
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

DirectionalLight::DirectionalLight(aiLight *light)
{
	name = light->mName.C_Str();
	auto pos = light->mPosition;
	local_position_value = vec3(pos.x, pos.y, pos.z);
	auto dir = light->mDirection;
	rotation_value = myn::quat_from_dir(vec3(dir.x, dir.y, dir.z));
	scale_value = vec3(1);
	auto col = light->mColorDiffuse;
	color = vec3(col.r, col.g, col.b);
	intensity = 1;
}

DirectionalLight::DirectionalLight(const tinygltf::Light *in_light)
{
	name = in_light->name;
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

	local_position_value = _local_pos;

	name = "[unnamed point light]";
}

PointLight::PointLight(aiLight *light)
{
	name = light->mName.C_Str();
	auto lcol = light->mColorDiffuse;
	color = vec3(lcol.r, lcol.g, lcol.b);
	intensity = 1.0f;

	auto pos = light->mPosition;
	local_position_value = vec3(pos.x, pos.y, pos.z);

	scale_value = vec3(1);
}

PointLight::PointLight(const tinygltf::Light *in_light)
{
	name = in_light->name;
	auto c = in_light->color;
	color = vec3(c[0], c[1], c[2]);
	intensity = in_light->intensity;
}

