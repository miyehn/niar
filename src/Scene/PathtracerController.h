//
// Created by miyehn on 5/19/22.
//

#include "Engine/SceneObject.hpp"

class PathtracerController : public SceneObject {
public:
	PathtracerController();
	bool handle_event(SDL_Event event) override;
	void draw_config_ui() override;

protected:
	void on_enable() override;
	void on_disable() override;

private:

	void set_local_position(glm::vec3 _local_position) override {}
	void set_rotation(glm::quat _rotation) override {}
	void set_scale(glm::vec3 _scale) override {}

};
