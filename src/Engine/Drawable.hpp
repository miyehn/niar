#pragma once
#include "Updatable.hpp"
#include "Asset/Shader.h"
#include "Asset/GlBlit.h"

struct Scene;

struct Drawable: public Updatable {

	Drawable(
			Drawable* _parent = nullptr, 
			std::string _name = "[unnamed drawable]");
	virtual ~Drawable();

	// inherited
	virtual bool handle_event(SDL_Event event);
	virtual void update(float elapsed);

	// draw function
	virtual void draw();

	// hierarchy
	Drawable* parent;
	std::vector<Drawable*> children = std::vector<Drawable*>();
	void foreach_descendent_bfs(const std::function<void(Drawable*)>& fn);
	virtual bool add_child(Drawable* child);

	// transformation
	mat4 object_to_parent() const;
	mat4 object_to_world() const;
	mat4 parent_to_object() const;
	mat4 world_to_object() const;
	
	mat3 object_to_world_rotation() const;
	mat3 world_to_object_rotation() const;

	vec3 world_position() const;
	vec3 local_position() const { return local_position_value; }
	quat rotation() const { return rotation_value; }
	vec3 scale() const { return scale_value; }

	virtual void set_local_position(vec3 _local_position) = 0;
	virtual void set_rotation(quat _rotation) = 0;
	virtual void set_scale(vec3 _scale) = 0;

protected:
	vec3 local_position_value;
	quat rotation_value;
	vec3 scale_value;

};

struct SceneObject : public Drawable
{
	explicit SceneObject(
		Drawable* _parent = nullptr,
		std::string _name = "[unnamed drawable]") : Drawable(_parent, _name){}
	void set_local_position(vec3 _local_position) override { local_position_value = _local_position; }
	void set_rotation(quat _rotation) override { rotation_value = _rotation; }
	void set_scale(vec3 _scale) override { scale_value = _scale; }
};