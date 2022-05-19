//
// Created by miyehn on 5/19/22.
//

#include "PathtracerController.h"
#include "Pathtracer/Pathtracer.hpp"

#include <imgui.h>

PathtracerController::PathtracerController() {
	name = "Pathtracer controller";
	show_transform = false;
	_enabled = false;
}

bool PathtracerController::handle_event(SDL_Event event) {

	auto pathtracer = Pathtracer::get();
	if (!pathtracer || !pathtracer->is_enabled()) return false;

	return pathtracer->handle_event(event) | SceneObject::handle_event(event);
}

void PathtracerController::draw_config_ui() {
	ImGui::Text("This is just a proxy object that forwards \nKB+M control to the pathtracer.");
	if (enabled()) {

		auto pathtracer = Pathtracer::get();
		if (!pathtracer || !pathtracer->is_enabled()) {
			auto yellow = ImVec4(1.0f, 0.7f, 0.1f, 1.0f);
			ImGui::TextColored(yellow, "Pathtracer renderer is not selected.");
		} else {
			auto green = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
			ImGui::TextColored(green, "Controller enabled.");
			ImGui::TextColored(green, " - [space]: continue/pause tracing");
			ImGui::TextColored(green, " - [opt]+LMB: set focal length");
			ImGui::TextColored(green, " - [shift]+LMB: trace a debug ray");
			ImGui::TextColored(green, " - RMB: clear the debug ray");
		}
	}
}
