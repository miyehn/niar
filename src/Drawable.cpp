#include "Drawable.hpp"
#include "utils.hpp"


Drawable::Drawable(Drawable* _parent, std::string _name) {
  parent = _parent;
  name = _name;
  if (parent) {
    parent->children.push_back(this);
  }

  local_position = vec3(0, 0, 0);
  rotation = quat(1, 0, 0, 0);
  scale = vec3(1, 1, 1);
}

Drawable::~Drawable() {
  for (uint i=0; i<children.size(); i++) delete children[i];
}

bool Drawable::handle_event(SDL_Event event) {
  bool handled = false;
  for (uint i=0; i<children.size(); i++) {
    handled = handled | children[i]->handle_event(event);
  }
  return handled;
}

void Drawable::update(float elapsed) {
  for (uint i=0; i<children.size(); i++) children[i]->update(elapsed);
}

void Drawable::draw() {
  for (uint i=0; i<children.size(); i++) children[i]->draw();
}

bool Drawable::add_child(Drawable* child) {
  if (!child) {
    WARNF("trying to add a null child %s, skipping..", name.c_str());
    return false;
  }
  children.push_back(child);
  child->parent = this;
  return true;
}

mat4 Drawable::object_to_parent() {
  return mat4( // translate
    vec4(1, 0, 0, 0),
    vec4(0, 1, 0, 0),
    vec4(0, 0, 1, 0),
    vec4(local_position, 1)
  ) * mat4_cast(rotation) // rotate
    * mat4 ( // scale
    vec4(scale.x, 0, 0, 0),
    vec4(0, scale.y, 0, 0),
    vec4(0, 0, scale.z, 0),
    vec4(0, 0, 0, 1)
  );
}

mat4 Drawable::object_to_world() {
  if (parent) return parent->object_to_world() * object_to_parent();
  else return object_to_parent();
}

mat4 Drawable::parent_to_object() {
  return mat4( // inv scale
    vec4(1.0f / scale.x, 0, 0, 0),
    vec4(0, 1.0f / scale.y, 0, 0),
    vec4(0, 0, 1.0f / scale.z, 0),
    vec4(0, 0, 0, 1)
  ) * mat4_cast(inverse(rotation)) // inv rotate
    * mat4( // un-translate
    vec4(1, 0, 0, 0),
    vec4(0, 1, 0, 0),
    vec4(0, 0, 1, 0),
    vec4(-local_position, 1)
  );
}

mat4 Drawable::world_to_object() {
  if (parent) return parent_to_object() * parent->world_to_object();
  else return parent_to_object();
}

vec3 Drawable::world_position() {
  vec4 position = object_to_world() * vec4(local_position, 1);
  return vec3(position.x, position.y, position.z) / position.w;
}
