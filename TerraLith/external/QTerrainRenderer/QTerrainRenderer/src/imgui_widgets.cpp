/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#include "qtr/windows_patch.hpp"

#include "qtr/imgui_widgets.hpp"
#include "qtr/logger.hpp"
#include "qtr/render_widget.hpp"

namespace qtr
{

bool imgui_enum_selector(const std::string              &label,
                         int                            &value,
                         const std::vector<std::string> &options)
{
  // ImGui expects const char*[]
  std::vector<const char *> cstrs;
  cstrs.reserve(options.size());
  for (auto &s : options)
    cstrs.push_back(s.c_str());

  return ImGui::Combo(label.c_str(),
                      &value,
                      cstrs.data(),
                      static_cast<int>(cstrs.size()));
}

bool imgui_viewer_main_menubar(RenderWidget &render_widget)
{
  bool changed = false;

  ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.f, 0.f, 0.f, 0.1f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.1f));

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Viewer Type"))
    {
      if (ImGui::MenuItem("2D viewer"))
        render_widget.set_render_type(RenderType::RENDER_2D);
      if (ImGui::MenuItem("3D renderer"))
        render_widget.set_render_type(RenderType::RENDER_3D);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  ImGui::PopStyleColor(2);

  return changed;
}

void imgui_set_blender_style()
{
  ImGuiStyle &style = ImGui::GetStyle();

  // ===== Colors =====
  ImVec4 blender_blue = ImVec4(0.369f, 0.506f, 0.675f, 1.f);
  ImVec4 blender_blue_hover = ImVec4(0.357f, 0.525f, 0.780f, 1.0f);
  ImVec4 blender_blue_active = ImVec4(0.200f, 0.369f, 0.624f, 1.0f);

  ImVec4 bg_dark = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);
  ImVec4 bg_light = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
  ImVec4 text_color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
  ImVec4 text_disabled = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

  style.Colors[ImGuiCol_Text] = text_color;
  style.Colors[ImGuiCol_TextDisabled] = text_disabled;
  style.Colors[ImGuiCol_WindowBg] = bg_dark;
  style.Colors[ImGuiCol_ChildBg] = bg_light;
  style.Colors[ImGuiCol_PopupBg] = bg_light;
  style.Colors[ImGuiCol_Border] = bg_light;
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
  style.Colors[ImGuiCol_FrameBg] = bg_light;
  style.Colors[ImGuiCol_FrameBgHovered] = blender_blue_hover;
  style.Colors[ImGuiCol_FrameBgActive] = blender_blue_active;
  style.Colors[ImGuiCol_TitleBg] = bg_dark;
  style.Colors[ImGuiCol_TitleBgActive] = bg_dark;
  style.Colors[ImGuiCol_TitleBgCollapsed] = bg_dark;

  // Buttons
  style.Colors[ImGuiCol_Button] = blender_blue;
  style.Colors[ImGuiCol_ButtonHovered] = blender_blue_hover;
  style.Colors[ImGuiCol_ButtonActive] = blender_blue_active;

  // Headers / collapsing menus
  style.Colors[ImGuiCol_Header] = blender_blue;
  style.Colors[ImGuiCol_HeaderHovered] = blender_blue_hover;
  style.Colors[ImGuiCol_HeaderActive] = blender_blue_active;

  // Tabs
  style.Colors[ImGuiCol_Tab] = blender_blue;
  style.Colors[ImGuiCol_TabHovered] = blender_blue_hover;
  style.Colors[ImGuiCol_TabActive] = blender_blue_active;
  style.Colors[ImGuiCol_TabUnfocused] = bg_light;
  style.Colors[ImGuiCol_TabUnfocusedActive] = blender_blue;

  // Sliders / Progress Bars
  style.Colors[ImGuiCol_SliderGrab] = blender_blue;
  style.Colors[ImGuiCol_SliderGrabActive] = blender_blue_active;
  style.Colors[ImGuiCol_PlotHistogram] = blender_blue;
  style.Colors[ImGuiCol_PlotHistogramHovered] = blender_blue_hover;

  // Checkboxes / Radio buttons
  style.Colors[ImGuiCol_CheckMark] = blender_blue;

  // ===== Style metrics =====
  style.WindowRounding = 4.0f;
  style.FrameRounding = 3.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 3.0f;
  style.ScrollbarRounding = 3.0f;
  style.WindowPadding = ImVec2(8, 8);
  style.FramePadding = ImVec2(6, 4);
  style.ItemSpacing = ImVec2(6, 4);
  style.ItemInnerSpacing = ImVec2(4, 4);
  style.IndentSpacing = 16.0f;
  style.ScrollbarSize = 12.0f;
  style.GrabMinSize = 8.0f;

  // Tweak other colors for consistent Blender look
  style.Colors[ImGuiCol_CheckMark] = blender_blue;
  style.Colors[ImGuiCol_Separator] = bg_light;
  style.Colors[ImGuiCol_SeparatorHovered] = blender_blue_hover;
  style.Colors[ImGuiCol_SeparatorActive] = blender_blue_active;
}

bool imgui_show_water_preset_selector(glm::vec3 &shallow, glm::vec3 &deep)
{
  bool ret = false;

  // Build list of preset names
  static std::vector<const char *> preset_names;
  if (preset_names.empty())
  {
    for (auto &kv : water_colors)
      preset_names.push_back(kv.first.c_str());
  }

  // Find current index
  int current_index = 0;
  for (size_t i = 0; i < preset_names.size(); i++)
  {
    if (current_water_preset == preset_names[i])
    {
      current_index = static_cast<int>(i);
      break;
    }
  }

  // Combo box
  if (ImGui::Combo("Water Preset",
                   &current_index,
                   preset_names.data(),
                   (int)preset_names.size()))
  {
    // Update selected preset
    current_water_preset = preset_names[current_index];
    ret = true;
  }

  // Display colors of the selected preset
  auto &preset = water_colors[current_water_preset];
  shallow = preset.first;
  deep = preset.second;

  ImGui::Indent();

  ImGui::Text("Shallow color:");
  ImGui::ColorButton("##shallow_color",
                     ImVec4(shallow.r, shallow.g, shallow.b, 1.0f),
                     0,
                     ImVec2(50, 20));
  ImGui::SameLine();
  ImGui::Text("(%.2f, %.2f, %.2f)", shallow.r, shallow.g, shallow.b);

  ImGui::Text("Deep color:");
  ImGui::ColorButton("##deep_color",
                     ImVec4(deep.r, deep.g, deep.b, 1.0f),
                     0,
                     ImVec2(50, 20));
  ImGui::SameLine();
  ImGui::Text("(%.2f, %.2f, %.2f)", deep.r, deep.g, deep.b);

  ImGui::Unindent();

  return ret;
}

} // namespace qtr
