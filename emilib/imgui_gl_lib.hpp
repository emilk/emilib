// By Emil Ernerfeldt 2015-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

namespace emilib {
namespace gl {

/// Call this to use gl_lib to paint Dear ImGui. Call once at the start of your program.
/// Will modify OmGui::GetIO().RenderDrawListsFn and OmGui::GetIO().Fonts->TexID
/// Tested on MacOS and iOS
void bind_imgui_painting();

/// Call this to draw the ImGui things onto the actual OpenGl backbuffer.
void paint_imgui();

/// Call this to stop using gl_lib to paint Dear ImGui.
void unbind_imgui_painting();

} // namespace gl
} // namespace emilib
