#pragma once
#include <functional>
#include "Shader.h"
#include "GlBlit.h"
#include "GlTexture.h"

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

struct GlMaterial {

	CONST_PTR(MatGeneric, mat_depth);
	
	GlMaterial (const std::string& shader_name) {
		shader = Shader::get(shader_name);
	}
	virtual ~GlMaterial() {}

	virtual void use(const Drawable* obj);

	std::function<void()> set_parameters = [](){};

	static void cleanup();

	Shader* shader = nullptr;

	// for managing the pool
	static void add_to_pool(const std::string& name, GlMaterial* mat);

	static GlMaterial* get(const std::string& name);

private:
	static std::unordered_map<std::string, GlMaterial*> material_pool;

};

// generic: materials whose object-dependent uniforms are just the transformations; can be used with any shader
struct MatGeneric : GlMaterial {
	MatGeneric(const std::string& shader_name) : GlMaterial(shader_name) {}
	virtual ~MatGeneric() {}
	virtual void use(const Drawable* obj);
};

// standard: materials tied to a specific shader
struct MatBasic : GlMaterial {
	MatBasic() : GlMaterial("basic") {
		base_color = GlTexture::white();
		tint = vec3(1);
	}
	virtual ~MatBasic() {}
	virtual void use(const Drawable* obj);
	GlTexture* base_color;
	vec3 tint;
};

struct MatDeferredGeometryBasic : GlMaterial {
	MatDeferredGeometryBasic() : GlMaterial("geometry_basic") {
		albedo_map = GlTexture::white();
		tint = vec3(1);
	}
	virtual ~MatDeferredGeometryBasic() {}
	virtual void use(const Drawable* obj);
	GlTexture* albedo_map;
	vec3 tint;
};

struct MatDeferredGeometry : GlMaterial {
	MatDeferredGeometry() : GlMaterial("geometry") {
		albedo_map = GlTexture::white();
		normal_map = GlTexture::default_normal();
		metallic_map = GlTexture::black();
		roughness_map = GlTexture::white();
		ao_map = GlTexture::white();
		tint = vec3(1);
	}
	virtual ~MatDeferredGeometry() {}
	virtual void use(const Drawable* obj);
	GlTexture* albedo_map;
	GlTexture* normal_map;
	GlTexture* metallic_map;
	GlTexture* roughness_map;
	GlTexture* ao_map;
	vec3 tint;
};

struct MatGrass : GlMaterial {
	MatGrass() : GlMaterial("grass") {}
	virtual ~MatGrass(){}
	virtual void use(const Drawable* obj);
};
