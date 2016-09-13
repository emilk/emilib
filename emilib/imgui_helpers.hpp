// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cstdint>

struct ImVec2;
using GLuint = uint32_t;

namespace imgui_helpers {

// Pick a size that maintains the aspect ration of the given image.
ImVec2 aspect_correct_image_size(const ImVec2& desired_size, const ImVec2& canvas_size, const ImVec2& minimum_size);

void gl_texture(GLuint tex_id, const ImVec2& size);

}
