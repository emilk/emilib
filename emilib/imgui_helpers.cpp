// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "imgui_helpers.hpp"

#include <imgui/imgui.h>

namespace imgui_helpers {

ImVec2 aspect_correct_image_size(const ImVec2& desired_size, const ImVec2& canvas_size, const ImVec2& minimum_size)
{
    const auto desired_width  = std::max(1.0f, desired_size.x);
    const auto desired_height = std::max(1.0f, desired_size.y);
    const float desired_aspect_ratio = desired_width / desired_height;

    auto result_width  = std::max(minimum_size.x, canvas_size.x);
    auto result_height = std::max(minimum_size.y, canvas_size.y);
    const float canvas_aspect = result_width / result_height;

    if (canvas_aspect < desired_aspect_ratio)
    {
        result_height = result_width / desired_aspect_ratio;
    }
    else
    {
        result_width = result_height * desired_aspect_ratio;
    }

    return {result_width, result_height};
}

void gl_texture(GLuint tex_id, const ImVec2& size)
{
    ImGui::Image(reinterpret_cast<ImTextureID>(tex_id), size, {0, 0}, {1, 1});
}

}
