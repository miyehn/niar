#pragma once
#include "Updatable.hpp"

#include "utils.hpp"

struct Camera;

struct Drawable: public Updatable {

  Drawable(Drawable* _parent = nullptr, std::string _name = "[unnamed drawable]") {
    parent = _parent; 
    name = _name;
    if (parent) parent->children.push_back(this);
  }

  // inherited
  virtual bool handle_event(SDL_Event event) {
    bool handled = false;
    for (uint i=0; i<children.size(); i++) {
      handled = handled | children[i]->handle_event(event);
    }
    return handled;
  }
  virtual void update(float elapsed) {
    for (uint i=0; i<children.size(); i++) children[i]->update(elapsed);
  }

  // game object can be shown on screen
  virtual void draw() {
    for (uint i=0; i<children.size(); i++) children[i]->draw();
  }


  virtual ~Drawable() {
    for (uint i=0; i<children.size(); i++) delete children[i];
  }

  //---- hierarchy ----

  Drawable* parent;

  std::vector<Drawable*> children = std::vector<Drawable*>();

  glm::mat4 transformation = glm::mat4(1.0f);
};
