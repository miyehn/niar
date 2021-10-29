#include "DebugUI.h"
#include <unordered_map>
#include <imgui.h>

std::unordered_map<std::string, std::vector<std::function<void()>>> UIMap;

void ui::elem(const std::function<void()>& fn, const std::string &category)
{
	if (!UIMap.contains(category))
		UIMap[category] = std::vector<std::function<void()>>();

	auto& categoryList = UIMap[category];
	categoryList.emplace_back(fn);
}

void ui::text(
	const std::string &content,
	const std::string &category)
{
	if (!UIMap.contains(category))
		UIMap[category] = std::vector<std::function<void()>>();

	auto& categoryList = UIMap[category];

	categoryList.emplace_back([content](){
		ImGui::Text("%s", content.c_str());
	});
}

void ui::button(const std::string &name, std::function<void()> fn, const std::string &category)
{
	if (!UIMap.contains(category))
		UIMap[category] = std::vector<std::function<void()>>();

	auto& categoryList = UIMap[category];

	categoryList.emplace_back([name, fn](){
		if (ImGui::Button(name.c_str())) {
			fn();
		}
	});
}

void ui::checkBox(
	const std::string &name,
	bool *value,
	const std::string &category)
{
	if (!UIMap.contains(category))
		UIMap[category] = std::vector<std::function<void()>>();

	auto& categoryList = UIMap[category];

	categoryList.emplace_back([name, value](){
		ImGui::CheckboxFlags(name.c_str(), (unsigned int*)value, 1);
	});
}

void ui::sliderFloat(
	const std::string &name,
	float* value,
	float min,
	float max,
	const std::string &formatStr,
	const std::string &category)
{
	if (!UIMap.contains(category))
		UIMap[category] = std::vector<std::function<void()>>();

	auto& categoryList = UIMap[category];

	categoryList.emplace_back([name, value, min, max, formatStr](){
		ImGui::SliderFloat(name.c_str(), value, min, max, formatStr.c_str());
	});
}

void ui::drawUI()
{
	for (const auto& category : UIMap)
	{
		if (ImGui::CollapsingHeader(category.first.c_str()))
		{
			if (category.first == "Default")
			{
			}
			for (const auto& makeElemFn : category.second)
			{
				makeElemFn();
			}
		}
	}
}