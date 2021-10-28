#pragma once
#include <string>
#include <functional>

namespace ui
{

void button(
	const std::string &name,
	std::function<void()> fn,
	const std::string &category="Default");

void checkBox(
	const std::string &name,
	bool* value,
	const std::string &category="Default");

// from ImGui:
// - CTRL+Click on any slider to turn them into an input box.
//   Manually input values aren't clamped by default and can go off-bounds.
//   Use ImGuiSliderFlags_AlwaysClamp to always clamp.
// - Adjust format string to decorate the value with a prefix, a suffix,
//   or adapt the editing and display precision
//   e.g. "%.3f" -> 1.234; "%5.2f secs" -> 01.23 secs; "Biscuit: %.0f" -> Biscuit: 1; etc.
void sliderFloat(
	const std::string &name,
	float* value,
	float min,
	float max,
	const std::string &formatStr="%.3f",
	const std::string &category="Default");

void drawUI();

};

