#include "Blade.hpp"

Blade::Blade(vec3 root) {
  this->up = vec3(0.0f, 0.0f, 1.0f);
  this->root = root;
  this->above = up * 4.0f;
  this->ctrl = above + vec3(-0.8f, 0.0f, 0.0f);

  this->orientation = radians(65.0f);
  this->height = 1.0f;
  this->width = 0.65f;
  this->stiffness = 1.0f;

}
