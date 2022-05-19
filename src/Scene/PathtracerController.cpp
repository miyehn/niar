//
// Created by miyehn on 5/19/22.
//

#include "PathtracerController.h"
#include "Utils/myn/Log.h"

#include <imgui.h>

PathtracerController::PathtracerController() {
	name = "Pathtracer controller";
	show_transform = false;
	_enabled = false;
}

bool PathtracerController::handle_event(SDL_Event event) {
	bool handled = false;

	/*
	if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_SPACE && !finished) {
		if (paused) continue_trace();
		else {
			TRACE("pausing - waiting for pending tiles");
			pause_trace();
		}
		return true;

	} else if (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_0) {
		reset();

	} else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) {
		int x, y; // NOTE: y IS INVERSED!!!!!!!!!!!
		SDL_GetMouseState(&x, &y);
		const uint8* state = SDL_GetKeyboardState(nullptr);

		if (state[SDL_SCANCODE_LSHIFT]) {
			size_t pixel_index = (height-y) * width + x;
			raytrace_debug(pixel_index);

		} else if (state[SDL_SCANCODE_LALT]) {
			float d = depth_of_first_hit(x, height-y);
			Cfg.Pathtracer.FocalDistance->set(d);
			TRACE("setting focal distance to %f", d);
		}
	} else if (event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_RIGHT) {
		logged_rays.clear();
	}
	 */

	return handled | SceneObject::handle_event(event);
}

void PathtracerController::draw_config_ui() {
	ImGui::Text("Controller :D");
	if (enabled()) {

		auto green = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
		ImGui::TextColored(green, "Controller enabled.");
		ImGui::TextColored(green, " - [space]: continue/pause tracing");
		ImGui::TextColored(green, " - LMB: trace a debug ray");
		ImGui::TextColored(green, " - [opt]+LMB: set focal length");

		if (ImGui::Button("reset")) {
			LOG("reset")
		}
	}
}

void PathtracerController::on_enable() {
	SceneObject::on_enable();
}

void PathtracerController::on_disable() {
	SceneObject::on_disable();
}
