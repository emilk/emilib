// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "imgui_helpers.hpp"

namespace imgui_helpers {

void show_im_gui_menu()
{
    static bool s_show_imgui_demo_window  = false;
    static bool s_show_imgui_metrics      = false;
    static bool s_show_imgui_style_editor = false;
    static bool s_show_imgui_user_guide   = false;

    if (ImGui::BeginMenu("ImGui")) {
        const double fps = ImGui::GetIO().Framerate;
        ImGui::Text("%.1f FPS (%.1f ms/frame) rolling average", fps, 1000.0 / fps);
        ImGui::Checkbox("Show ImGui example",      &s_show_imgui_demo_window);
        ImGui::Checkbox("Show ImGui metrics",      &s_show_imgui_metrics);
        ImGui::Checkbox("Show ImGui style editor", &s_show_imgui_style_editor);
        ImGui::Checkbox("Show ImGui user guide",   &s_show_imgui_user_guide);

        ImGui::SliderInt("Log stderr verbosity", &loguru::g_stderr_verbosity, 0, 10);

        if (ImGui::Button("Classic Style")) { ImGui::StyleColorsClassic(); }
        if (ImGui::Button("Dark Style"))    { ImGui::StyleColorsDark();    }
        if (ImGui::Button("Light Style"))   { ImGui::StyleColorsLight();   }

        ImGui::EndMenu();
    }

    if (s_show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&s_show_imgui_demo_window);
    }

    if (s_show_imgui_metrics) {
        ImGui::ShowMetricsWindow(&s_show_imgui_metrics);
    }

    if (s_show_imgui_style_editor) {
        if (ImGui::Begin("ImGui Style Editor", &s_show_imgui_style_editor)) {
            ImGui::ShowStyleEditor();
        }
        ImGui::End();
    }

    if (s_show_imgui_user_guide) {
        if (ImGui::Begin("ImGui User Guide", &s_show_imgui_user_guide)) {
            ImGui::ShowUserGuide();
        }
        ImGui::End();
    }
}

ImVec2 aspect_correct_image_size(const ImVec2& desired_size, const ImVec2& canvas_size, const ImVec2& minimum_size)
{
    const auto desired_width  = std::max(1.0f, desired_size.x);
    const auto desired_height = std::max(1.0f, desired_size.y);
    const float desired_aspect_ratio = desired_width / desired_height;

    auto result_width  = std::max(minimum_size.x, canvas_size.x);
    auto result_height = std::max(minimum_size.y, canvas_size.y);
    const float canvas_aspect = result_width / result_height;

    if (canvas_aspect < desired_aspect_ratio) {
        result_height = result_width / desired_aspect_ratio;
    } else {
        result_width = result_height * desired_aspect_ratio;
    }

    return {result_width, result_height};
}

void gl_texture(GLuint tex_id, const ImVec2& size)
{
    ImGui::Image(reinterpret_cast<ImTextureID>(tex_id), size, {0, 0}, {1, 1});
}

} // namespace imgui_helpers

// ----------------------------------------------------------------------------

namespace ImGuiPP {

bool SliderSize(const std::string& label, size_t* v, size_t v_min, size_t v_max, float power)
{
    CHECK_NOTNULL_F(v);
    float v_float = *v;
    bool changed = ImGui::SliderFloat(label.c_str(), &v_float, static_cast<float>(v_min), static_cast<float>(v_max), "%.0f", power);
    if (changed) {
        *v = static_cast<size_t>(std::round(v_float));
        *v = std::max(*v, v_min);
        *v = std::min(*v, v_max);
    }
    return changed;
}

bool InputText(const std::string& label, std::string& text, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    char buff[1024];
    strncpy(buff, text.c_str(), sizeof(buff));
    if (ImGui::InputText(label.c_str(), buff, sizeof(buff), flags, callback, user_data)) {
        text = buff;
        return true;
    } else {
        return false;
    }
}

void Text(const std::string& text)
{
    ImGui::Text("%s", text.c_str());
}

void LabelText(const std::string& label, const std::string& text)
{
    ImGui::LabelText(label.c_str(), "%s", text.c_str());
}

bool Button(const std::string& text)
{
    return ImGui::Button(text.c_str());
}

bool ListBox(const std::string& label, std::string* current_item, const std::vector<std::string>& items, int height_in_items)
{
    CHECK_NOTNULL_F(current_item);
    std::vector<const char*> items_c_str;
    items_c_str.reserve(items.size());
    int current_item_index = -1;
    for (int i = 0; i < items.size(); ++i) {
        items_c_str.push_back(items[i].c_str());
        if (items[i] == *current_item) {
            current_item_index = i;
        }
    }

    if (ImGui::ListBox(label.c_str(), &current_item_index, items_c_str.data(), (int)items.size(), height_in_items)) {
        CHECK_F(0 <= current_item_index && current_item_index < items.size());
        *current_item = items[current_item_index];
        return true;
    } else {
        return false;
    }
}

bool Combo(const std::string& label, std::string* current_item, const std::vector<std::string>& items, int height_in_items)
{
    CHECK_NOTNULL_F(current_item);
    std::vector<const char*> items_c_str;
    items_c_str.reserve(items.size());
    int current_item_index = -1;
    for (int i = 0; i < items.size(); ++i) {
        items_c_str.push_back(items[i].c_str());
        if (items[i] == *current_item) {
            current_item_index = i;
        }
    }

    if (ImGui::Combo(label.c_str(), &current_item_index, items_c_str.data(), (int)items.size(), height_in_items)) {
        CHECK_F(0 <= current_item_index && current_item_index < items.size());
        *current_item = items[current_item_index];
        return true;
    } else {
        return false;
    }
}

} // namespace ImGuiPP
