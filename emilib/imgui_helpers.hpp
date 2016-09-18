// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <imgui/imgui.h>

using GLuint = uint32_t;

namespace imgui_helpers {

// Pick a size that maintains the aspect ration of the given image.
ImVec2 aspect_correct_image_size(const ImVec2& desired_size, const ImVec2& canvas_size, const ImVec2& minimum_size);

void gl_texture(GLuint tex_id, const ImVec2& size);

} // namespace imgui_helpers

// ----------------------------------------------------------------------------

// Helper C++ bindings for ImGui
namespace ImGuiPP {

bool InputText(const std::string& label, std::string& text, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);
void Text(const std::string& text);
void LabelText(const std::string& label, const std::string& text);
bool Button(const std::string& text);

bool ListBox(const std::string& label, std::string& current_item, const std::vector<std::string>& items, int height_in_items = -1);
bool Combo(const std::string& label, std::string& current_item, const std::vector<std::string>& items, int height_in_items = -1);

} // namespace ImGuiPP
