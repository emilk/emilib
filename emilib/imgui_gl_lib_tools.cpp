#include "imgui_gl_lib_tools.hpp"

#include <unordered_map>

#include "gl_lib.hpp"

namespace emilib {
namespace gl {
namespace {

struct PosTC
{
	ImVec2 a_pos;
	ImVec2 a_tc;
};

struct PaintInfo
{
	Painter painter;
	ImVec2  pos_points;
	ImVec2  size_points;
};

void imgui_paint_callback(const ImDrawList*, const ImDrawCmd* cmd)
{
	ERROR_CONTEXT("function", "imgui_paint_callback");
	CHECK_NOTNULL_F(cmd);
	CHECK_NOTNULL_F(cmd->UserCallbackData);
	const PaintInfo& paint_info = *reinterpret_cast<PaintInfo*>(cmd->UserCallbackData);
	paint_texture_at(paint_info.painter, paint_info.pos_points, paint_info.size_points, cmd->ClipRect);
}

} // namespace

void paint_texture_at(
	const Painter& painter,
	const ImVec2&  pos_points,
	const ImVec2&  size_points,
	const ImVec4&  clip_rect)
{
	static gl::MeshPainter s_mesh_painter{gl::Usage::WRITE_MANY_READ_MANY, {
		gl::VertComp::Vec2f("a_pos"),
		gl::VertComp::Vec2f("a_tc"),
	}};

	const float window_width = ImGui::GetIO().DisplaySize.x;
	const float window_height = ImGui::GetIO().DisplaySize.y;

	const float y_increases_down = true;
	const float y_sign = (y_increases_down ? -1.0f : +1.0f);

	auto transform_x = [&](const float x)
	{
		return x * 2.0f / window_width - 1.0f;
	};

	auto transform_y = [&](const float y)
	{
		return y_sign * (y * 2.0f / window_height - 1.0f);
	};

	PosTC* verts = s_mesh_painter.allocate_vert<PosTC>(4);

	const float left_pts   = std::max(clip_rect.x, pos_points.x);
	const float right_pts  = std::min(clip_rect.z, pos_points.x + size_points.x);
	const float top_pts    = std::max(clip_rect.y, pos_points.y);
	const float bottom_pts = std::min(clip_rect.w, pos_points.y + size_points.y);

	if (right_pts <= left_pts) { return; }
	if (bottom_pts <= top_pts) { return; }

	const float left_uv   = (left_pts   - pos_points.x) / size_points.x;
	const float right_uv  = (right_pts  - pos_points.x) / size_points.x;
	const float top_uv    = (top_pts    - pos_points.y) / size_points.y;
	const float bottom_uv = (bottom_pts - pos_points.y) / size_points.y;

	verts[0] = PosTC{{transform_x(left_pts),  transform_y(top_pts)},    {left_uv, 1.0f - top_uv}};
	verts[1] = PosTC{{transform_x(left_pts),  transform_y(bottom_pts)}, {left_uv, 1.0f - bottom_uv}};
	verts[2] = PosTC{{transform_x(right_pts), transform_y(top_pts)},    {right_uv, 1.0f - top_uv}};
	verts[3] = PosTC{{transform_x(right_pts), transform_y(bottom_pts)}, {right_uv, 1.0f - bottom_uv}};

	painter(s_mesh_painter);
}

void imgui_show_gl(
	const char*    label,
	const ImVec2&  size,
	const Painter& painter)
{
	static std::unordered_map<ImGuiID, PaintInfo> s_paint_infos;
	PaintInfo& paint_info = s_paint_infos[ImGui::GetID(label)];

	paint_info.painter = painter;
	paint_info.pos_points = ImGui::GetCursorScreenPos();
	paint_info.size_points = size;

	// ImGui::InvisibleButton(label, size); // Can't drag window by grabbing the area.
	ImGui::Dummy(size); // Grab the paint area to drag window.

	ImGui::GetWindowDrawList()->AddCallback(imgui_paint_callback, &paint_info);
}

} // namespace gl
} // namespace emilib
