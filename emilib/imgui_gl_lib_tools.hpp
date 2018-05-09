// By Emil Ernerfeldt 2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <functional>

#include <imgui/imgui.h>

#include "gl_lib_fwd.hpp"

namespace emilib {
namespace gl {

using Painter = std::function<void(gl::MeshPainter&)>;

/// Draw directly to screen now.
void paint_texture_at(
	const Painter& painter,
	const ImVec2&  pos_points,
	const ImVec2&  size_points,
	const ImVec4&  clip_rect); /// Clipping rectangle (x1, y1, x2, y2)

/// Draw later inside of a ImGui window.
/// Example usage:
/// imgui_show_gl("some unique id", ImGui::GetContentRegionAvail(), [=](auto& mesh_painter){
/// 	texture->bind(0);
/// 	program->bind();
/// 	program->set_uniform("u_sampler", 0);
/// 	mesh_painter.paint_strip(*program);
/// 	texture->unbind();
/// 	program->unbind();
/// });
void imgui_show_gl(
	const char*    label,
	const ImVec2&  size,
	const Painter& painter);

} // namespace gl
} // namespace emilib
