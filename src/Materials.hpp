#pragma once
#include <functional>
#include "Shader.hpp"
#include "Texture.hpp"

/* Two types of materials:
 *
 * - generic (object-dependent uniforms are just the transformations),
 *   therefore don't need to be stored for each object (but may still have multiple copies)
 *
 * - standard (with non-transform object-dependent properties)
 *   stored at each object. Each object may have multiple of these (material sets)
 */

struct Drawable;
struct MatGeneric;

struct Material {

	CONST_PTR(MatGeneric, mat_depth);
	
	Material (const std::string& shader_name) {
		shader = Shader::get(shader_name);
	}
	virtual ~Material() {}

	virtual void use(const Drawable* obj);

	std::function<void()> set_parameters = [](){};

	static void cleanup();

	Shader* shader = nullptr;

};

// generic: materials whose object-dependent uniforms are just the transformations; can be used with any shader
struct MatGeneric : Material {
	MatGeneric(const std::string& shader_name) : Material(shader_name) {}
	virtual ~MatGeneric() {}
	virtual void use(const Drawable* obj);
};

// standard: materials tied to a specific shader
struct MatBasic : Material {
	MatBasic() : Material("basic") {
		base_color = Texture::white();
		tint = vec3(1);
	}
	virtual ~MatBasic() {}
	virtual void use(const Drawable* obj);
	Texture* base_color;
	vec3 tint;
};

struct MatDeferredGeometryBasic : Material {
	MatDeferredGeometryBasic() : Material("geometry_basic") {
		base_color = Texture::white();
		tint = vec3(1);
	}
	virtual ~MatDeferredGeometryBasic() {}
	virtual void use(const Drawable* obj);
	Texture* base_color;
	vec3 tint;
};

struct MatDeferredGeometry : Material {
	MatDeferredGeometry() : Material("geometry") {
		base_color = Texture::white();
		normal_map = Texture::default_normal();
		metallic_map = Texture::black();
		roughness_map = Texture::white();
		ao_map = Texture::white();
		tint = vec3(1);
	}
	virtual ~MatDeferredGeometry() {}
	virtual void use(const Drawable* obj);
	Texture* base_color;
	Texture* normal_map;
	Texture* metallic_map;
	Texture* roughness_map;
	Texture* ao_map;
	vec3 tint;
};

struct MatGrass : Material {
	MatGrass() : Material("grass") {}
	virtual ~MatGrass(){}
	virtual void use(const Drawable* obj);
};
