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

void ui::usePurpleStyle()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.01f, 0.01f, 0.01f, 0.76f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.08f, 0.15f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.28f, 0.10f, 0.24f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.57f, 0.08f, 0.53f, 0.67f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.05f, 0.03f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.24f, 0.01f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.03f, 0.02f, 0.03f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.19f, 0.05f, 0.16f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.38f, 0.07f, 0.32f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.60f, 0.06f, 0.49f, 1.00f);
	colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 0.07f, 0.53f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.39f, 0.08f, 0.27f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.98f, 0.26f, 0.81f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.86f, 0.10f, 0.46f, 0.40f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.91f, 0.09f, 0.38f, 1.00f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(1.00f, 0.21f, 0.52f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.21f, 0.09f, 0.19f, 0.31f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.23f, 0.10f, 0.25f, 0.80f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.39f, 0.14f, 0.40f, 1.00f);
	colors[ImGuiCol_Separator]              = ImVec4(0.78f, 0.05f, 0.92f, 0.50f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.99f, 0.18f, 0.88f, 0.78f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.99f, 0.31f, 0.63f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.98f, 0.26f, 0.74f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.97f, 0.26f, 0.85f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 0.42f, 0.75f, 0.95f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.08f, 0.03f, 0.07f, 0.86f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.20f, 0.05f, 0.13f, 0.80f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.66f, 0.08f, 0.33f, 1.00f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.12f, 0.07f, 0.15f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.37f, 0.14f, 0.42f, 1.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.35f, 0.11f, 0.29f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.98f, 0.26f, 0.88f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}