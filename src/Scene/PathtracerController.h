//
// Created by miyehn on 5/19/22.
//

#include "Engine/SceneObject.hpp"

/*
 * just a proxy object to forward some keyboard shortcuts to the pathtracer renderer
 */

class PathtracerController : public SceneObject {
public:
	PathtracerController();
	bool handle_event(SDL_Event event) override;
	void draw_config_ui() override;
};