// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <imgui/imgui.h>

static inline ImVec2 operator*(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x*rhs, lhs.y*rhs); }
static inline ImVec2 operator/(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x/rhs, lhs.y/rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x*rhs.x, lhs.y*rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x/rhs.x, lhs.y/rhs.y); }
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs)                  { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2& operator/=(ImVec2& lhs, const float rhs)                  { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline ImVec4 operator+(const ImVec4& lhs, const ImVec4& rhs)            { return ImVec4(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, lhs.w+rhs.w); }
static inline ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs)            { return ImVec4(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w); }
static inline ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs)            { return ImVec4(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z, lhs.w*rhs.w); }

namespace imgui_helpers {

using GLuint = uint32_t;

/* Examples:
    if (ImGui::BeginMainMenuBar()) {
        imgui_helpers::show_im_gui_menu();
        ImGui::EndMainMenuBar();
    }
*/
void show_im_gui_menu();

/// Pick a size that maintains the aspect ration of the given image.
ImVec2 aspect_correct_image_size(const ImVec2& desired_size, const ImVec2& canvas_size, const ImVec2& minimum_size);

void gl_texture(GLuint tex_id, const ImVec2& size);

} // namespace imgui_helpers

// ----------------------------------------------------------------------------

/// Helper C++ bindings for ImGui
namespace ImGuiPP {

bool SliderSize(const std::string& label, size_t* v, size_t v_min, size_t v_max, float power = 1.0f);

bool InputText(const std::string& label, std::string& text, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
void Text(const std::string& text);
void LabelText(const std::string& label, const std::string& text);
bool Button(const std::string& text);

bool ListBox(const std::string& label, std::string* current_item, const std::vector<std::string>& items, int height_in_items = -1);
bool Combo(const std::string& label, std::string* current_item, const std::vector<std::string>& items, int height_in_items = -1);

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
