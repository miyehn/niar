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

struct Material {
	
	Material (const std::string& shader_name) {
		shader = Shader::get(shader_name);
		shader->set_static_parameters = [](){};
	}
	virtual ~Material() {}

	virtual void use(const Drawable* obj);

	// material constants (owned by mateiral class)
	static Material* mat_depth(); // generic
	static void cleanup();

	Shader* shader = nullptr;

protected:
	static Material* mat_depth_value;
};

// materials whose object-dependent uniforms are just the transformations
struct MatGeneric : Material {
	MatGeneric(const std::string& shader_name) : Material(shader_name) {}
	virtual ~MatGeneric() {}
	virtual void use(const Drawable* obj);
};

struct MatBasic : Material {
	MatBasic() : Material("basic") {
		base_color = Texture::get("white");
		tint = vec3(1);
	}
	virtual ~MatBasic() {}
	virtual void use(const Drawable* obj);
	Texture* base_color;
	vec3 tint;
};

struct MatDeferredGeometry : Material {
	MatDeferredGeometry() : Material("geometry") {
		base_color = Texture::get("white");
		tint = vec3(1);
	}
	virtual ~MatDeferredGeometry() {}
	virtual void use(const Drawable* obj);
	Texture* base_color;
	vec3 tint;
};

struct MatGrass : Material {
	MatGrass() : Material("grass") {}
	virtual ~MatGrass(){}
	virtual void use(const Drawable* obj);
};
