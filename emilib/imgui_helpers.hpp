// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define IM_VEC2_CLASS_EXTRA                                                                        \
    friend ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return {a.x + b.x, a.y + b.y}; }   \
    friend ImVec2 operator-(const ImVec2& a, const ImVec2& b) { return {a.x - b.x, a.y - b.y}; }   \
    friend ImVec2 operator*(const ImVec2& v, float s) { return {v.x * s, v.y * s}; }               \
    friend ImVec2 operator*(float s, const ImVec2& v) { return {v.x * s, v.y * s}; }               \
    friend bool operator==(const ImVec2& a, const ImVec2& b) { return a.x == b.x && a.y == b.y; }  \
    void operator*=(float s) { x *= s; y *= s; }                                                   \
    void operator/=(float s) { x /= s; y /= s; }

#define IM_VEC4_CLASS_EXTRA                                                        \
    friend bool operator==(const ImVec4& a, const ImVec4& b) {                     \
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }             \

#include <imgui/imgui.h>

using GLuint = uint32_t;

namespace imgui_helpers {

/// Pick a size that maintains the aspect ration of the given image.
ImVec2 aspect_correct_image_size(const ImVec2& desired_size, const ImVec2& canvas_size, const ImVec2& minimum_size);

void gl_texture(GLuint tex_id, const ImVec2& size);

} // namespace imgui_helpers

// ----------------------------------------------------------------------------

/// Helper C++ bindings for ImGui
namespace ImGuiPP {

bool SliderSize(const std::string& label, size_t* v, size_t v_min, size_t v_max, float power = 1.0f);

bool InputText(const std::string& label, std::string& text, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);
void Text(const std::string& text);
void LabelText(const std::string& label, const std::string& text);
bool Button(const std::string& text);

bool ListBox(const std::string& label, std::string& current_item, const std::vector<std::string>& items, int height_in_items = -1);
bool Combo(const std::string& label, std::string& current_item, const std::vector<std::string>& items, int height_in_items = -1);

/// Convenience for enums
template<typename Enum>
bool RadioButtonEnum(const char* label, Enum* v, Enum v_button)
{
	if (ImGui::RadioButton(label, *v == v_button)) {
		*v = v_button;
		return true;
	} else {
		return false;
	}
}

} // namespace ImGuiPP
