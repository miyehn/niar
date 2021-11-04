#pragma once
#include <string>
#include <functional>
#include <SDL_events.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Scene;

class SceneObject {
public:

	explicit SceneObject(
			SceneObject* _parent = nullptr,
			std::string _name = "[unnamed scene object]");
	virtual ~SceneObject();

	// inherited
	virtual bool handle_event(SDL_Event event);
	virtual void update(float elapsed);

	// draw function
	virtual void draw();

	// hierarchy operations
	SceneObject* parent;
	std::vector<SceneObject*> children = std::vector<SceneObject*>();
	void foreach_descendent_bfs(const std::function<void(SceneObject*)>& fn);
	virtual bool add_child(SceneObject* child);

	// other operations

	virtual void set_local_position(glm::vec3 _local_position) { local_position_value = _local_position; }
	virtual void set_rotation(glm::quat _rotation) { rotation_value = _rotation; }
	virtual void set_scale(glm::vec3 _scale) { scale_value = _scale; }

	void draw_transform_ui(bool global) const;

	// transformation
	glm::mat4 object_to_parent() const;
	glm::mat4 object_to_world() const;
	glm::mat4 parent_to_object() const;
	glm::mat4 world_to_object() const;
	
	glm::mat3 object_to_world_rotation() const;
	glm::mat3 world_to_object_rotation() const;

	glm::vec3 world_position() const;
	glm::vec3 local_position() const { return local_position_value; }
	glm::quat rotation() const { return rotation_value; }
	glm::vec3 scale() const { return scale_value; }

	std::string name;
	bool enabled = true;

protected:

	glm::vec3 local_position_value;
	glm::quat rotation_value;
	glm::vec3 scale_value;

};