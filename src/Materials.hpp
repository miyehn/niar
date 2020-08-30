#pragma once
#include <functional>
#include "Shader.hpp"

/* Two types of materials:
 *
 * - generic (object-dependent uniforms are just the transformations),
 *   therefore don't need to be stored for each object (but may still have multiple copies)
 *
 * - standard (with non-transform object-dependent properties)
 *   stored at each object. Each object may have multiple of these (material sets)
 */

struct Drawable;

struct Material {
	
	Material (const std::string& shader_name) {
		shader = Shader::get(shader_name);
		shader->set_static_parameters = [](){};
	}

	virtual void use(const Drawable* obj);

	// material constants
	static Material* mat_depth(); // generic

	Shader* shader = nullptr;

protected:
	static Material* mat_depth_value;
};

// materials whose object-dependent uniforms are just the transformations
struct MatGeneric : Material {
	MatGeneric(const std::string& shader_name) : Material(shader_name) {}
	virtual void use(const Drawable* obj);
};

struct MatBasic : Material {
};

struct MatDeferredGeometry : Material {
};

struct MatDeferredLightingDirectional : Material {
};

struct MatDeferredLightingPoint : Material {
};
