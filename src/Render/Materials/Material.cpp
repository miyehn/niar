#include "Material.h"
#include "Engine/Drawable.hpp"
#include "Render/Renderers/Renderer.h"

std::unordered_map<std::string, Material*> Material::pool;

void Material::add(Material* material)
{
	auto mat_iter = Material::pool.find(material->name);
	if (mat_iter != Material::pool.end())
	{
		WARN("Adding material of duplicate names. Overriding..")
		delete mat_iter->second;
	}
	Material::pool[material->name] = material;
}

Material *Material::find(const std::string &name)
{
	auto mat_iter = pool.find(name);
	if (mat_iter != pool.end()) return mat_iter->second;
	return nullptr;
}

void Material::cleanup()
{
	for (auto it : pool)
	{
		delete it.second;
	}
}
