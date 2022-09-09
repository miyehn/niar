#include "SceneObject.hpp"
#include "Utils/myn/Log.h"
#include <queue>
#if GRAPHICS_DISPLAY
#include <imgui.h>
#endif

using namespace glm;

SceneObject::SceneObject(SceneObject* _parent, std::string _name) {

	parent = _parent;
	name = _name;
	if (parent) {
		parent->children.push_back(this);
	}

	_local_position = vec3(0, 0, 0);
	_rotation = quat(1, 0, 0, 0);
	_scale = vec3(1, 1, 1);
}

SceneObject::~SceneObject() {
	for (auto & child : children) delete child;
	children.clear();
}

#if GRAPHICS_DISPLAY
bool SceneObject::handle_event(SDL_Event event) {
	bool handled = false;
	for (auto & child : children) {
		if (child->enabled())
			handled = handled || child->handle_event(event);
	}
	return handled;
}

void SceneObject::update(float elapsed) {
	for (auto & child : children)
		if (child->enabled()) child->update(elapsed);
}

void SceneObject::draw(VkCommandBuffer cmdbuf) {
	for (auto & child : children)
		if (child->enabled()) child->draw(cmdbuf);
}
#endif

bool SceneObject::add_child(SceneObject* child) {
	if (!child) {
		WARN("trying to add a null child to %s, skipping..", name.c_str());
		return false;
	}
	if (child->parent) {
		for (auto p = child->parent->children.begin(); p != child->parent->children.end(); p++) {
			if (*p == child) {
				child->parent->children.erase(p);
				break;
			}
		}
	}
	children.push_back(child);
	child->parent = this;
	return true;
}

void SceneObject::set_parent(SceneObject *in_parent)
{
	EXPECT(in_parent == nullptr, false)
	if (parent) {
		parent->try_remove_child(this);
	}
	EXPECT(parent == nullptr, true)
	parent->add_child(this);
}

bool SceneObject::try_remove_child(SceneObject *child)
{
	for (auto p = children.begin(); p != children.end(); p++) {
		if (*p == child) {
			children.erase(p);
			return true;
		}
	}
	return false;
}

mat4 SceneObject::object_to_parent() const {
	vec3 sc = _scale;
	return mat4( // translate
		vec4(1, 0, 0, 0),
		vec4(0, 1, 0, 0),
		vec4(0, 0, 1, 0),
		vec4(local_position(), 1)
	) * mat4_cast(rotation()) // rotate
		* mat4 ( // scale
		vec4(sc.x, 0, 0, 0),
		vec4(0, sc.y, 0, 0),
		vec4(0, 0, sc.z, 0),
		vec4(0, 0, 0, 1)
	);
}

mat4 SceneObject::object_to_world() const {
	if (parent) return parent->object_to_world() * object_to_parent();
	else return object_to_parent();
}

mat4 SceneObject::parent_to_object() const {
	vec3 sc = scale();
	return mat4( // inv scale
		vec4(1.0f / sc.x, 0, 0, 0),
		vec4(0, 1.0f / sc.y, 0, 0),
		vec4(0, 0, 1.0f / sc.z, 0),
		vec4(0, 0, 0, 1)
	) * mat4_cast(inverse(rotation())) // inv rotate
		* mat4( // un-translate
		vec4(1, 0, 0, 0),
		vec4(0, 1, 0, 0),
		vec4(0, 0, 1, 0),
		vec4(-local_position(), 1)
	);
}

mat3 SceneObject::object_to_world_rotation() const {
	if (!parent) return mat3_cast(rotation());
	return parent->object_to_world_rotation() * mat3_cast(rotation());
}

mat3 SceneObject::world_to_object_rotation() const {
	return transpose(object_to_world_rotation());
}

mat4 SceneObject::world_to_object() const {
	if (parent) return parent_to_object() * parent->world_to_object();
	else return parent_to_object();
}

vec3 SceneObject::world_position() const {
	vec4 position = object_to_world() * vec4(0, 0, 0, 1);
	return vec3(position.x, position.y, position.z) / position.w;
}

void SceneObject::foreach_descendent_bfs(
	const std::function<void(SceneObject*)>& fn,
	const std::function<bool(SceneObject*)>& filter_condition)
{
	std::queue<SceneObject*> Q;
	Q.push(this);
	while (!Q.empty())
	{
		auto node = Q.front();
		Q.pop();
		if (filter_condition(node))
		{
			fn(node);
			for (auto child : node->children)
			{
				Q.push(child);
			}
		}
	}
}

#if GRAPHICS_DISPLAY
void SceneObject::draw_transform_ui(bool global) const
{
	vec3 pos = global ? world_position() : local_position();
	quat qrot = rotation();
	vec3 scl = scale();
	if (global)
	{
		auto ptr = parent;
		while (ptr != nullptr)
		{
			qrot = ptr->rotation() * qrot;
			scl = ptr->scale() * scl;
			ptr = ptr->parent;
		}
	}

	vec3 rot = eulerAngles(qrot);

	ImGui::DragFloat3("pos", (float*)&_local_position, 0.05f);
	ImGui::DragFloat3("scl", (float*)&_scale, 0.05f);
	//ImGui::DragFloat3("rot", (float*)&_rotation, 15.0f);
	ImGui::Text("%.3f, %.3f, %.3f [rotation]", degrees(rot.x), degrees(rot.y), degrees(rot.z));

	//ImGui::TextDisabled("[pos] %.3f, %.3f, %.3f", pos.x, pos.y, pos.z);
	//ImGui::TextDisabled("[rot] %.3f, %.3f, %.3f", degrees(rot.x), degrees(rot.y), degrees(rot.z));
	//ImGui::TextDisabled("[scl] %.3f, %.3f, %.3f", scl.x, scl.y, scl.z);
}
#endif

void SceneObject::toggle_enabled() {
	_enabled = !_enabled;
	if (_enabled) on_enable();
	else on_disable();
}

void SceneObject::rotate_around_axis(glm::vec3 ws_axis_unitvec, float theta) {
	float cos_half_theta = cos(theta / 2);
	float sin_half_theta = sin(theta / 2);
	auto qrot = quat(
		cos_half_theta,
		ws_axis_unitvec.x * sin_half_theta,
		ws_axis_unitvec.y * sin_half_theta,
		ws_axis_unitvec.z * sin_half_theta);
	_rotation = qrot * _rotation;
}