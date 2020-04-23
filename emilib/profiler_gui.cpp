//
//  profiler_gui.cpp
//  Created for ghostel
//
//  Created by Emil Ernerfeldt on 2016-03-05.
//  Copyright © 2016 Emil Ernerfeldt. All rights reserved.
//

#include "profiler_gui.hpp"

#include <unordered_map>

#include <imgui/imgui.h>
#include <loguru.hpp>

#include <util/profiler.hpp>

namespace profiler {

struct Options
{
	profiler::NanoSeconds start_ns;

	float points_per_ns = 0;
	float rounding     =  4;
	float rect_height  = 16;
	float spacing      =  4;
	float font_size    =  12;
	ImU32 text_color  = 0xFFFFFFFF;

	ImU32 grid_color  = 0x33FFFFFF;

	float min_width = 0.5f;

	ImU32 rect_color         = 0xAA0000AA;
	ImU32 rect_color_hovered = 0xFF0000AA;

	float scroll_speed = 1;
	float pinch_speed  = 1;

	double scroll_x_points = 0;

	float point_from_ns(profiler::NanoSeconds ns) const
	{
		return float(scroll_x_points + (ns - start_ns) * points_per_ns);
	};
};

struct Painter
{
	Painter()
		: draw_list(ImGui::GetWindowDrawList())
		, window_font(ImGui::GetWindowFont())
		, window_font_size(ImGui::GetWindowFontSize())
		, canvas_pos(ImGui::GetCursorScreenPos())
		, canvas_size(ImGui::GetContentRegionAvail())
	{}

	ImDrawList* draw_list;
	ImFont*     window_font;
	float       window_font_size;
	ImVec2      canvas_pos;
	ImVec2      canvas_size;

	void add_text(ImVec2 pos, const std::string& text, ImU32 color, float font_size_arg)
	{
		if (font_size_arg <= 0)
		{
			font_size_arg = window_font_size;
		}

		draw_list->AddText(window_font, font_size_arg, pos, color, text.c_str());
	}
};

enum class PaintResult { kCulled, kHovered, kNormal };

PaintResult paint_record(Painter& io_painter, const Options& options,
				  const Record& record, size_t depth)
{
	auto start_x = io_painter.canvas_pos.x + options.point_from_ns(record.start_ns);
	if (io_painter.canvas_pos.x + io_painter.canvas_size.x < start_x) { return PaintResult::kCulled; }
	auto stop_x = io_painter.canvas_pos.x + options.point_from_ns(record.start_ns + record.duration_ns);
	if (stop_x < io_painter.canvas_pos.x) { return PaintResult::kCulled; }

	float width = stop_x - start_x;

	if (width < 0.5f) { return PaintResult::kCulled; }
	// if (width < options.min_width) { stop_x = start_x + options.min_width; }

	auto start_y = io_painter.canvas_pos.y + depth * (options.rect_height + options.spacing);
	auto stop_y = start_y + options.rect_height;

	auto mpos_x = ImGui::GetIO().MousePos.x;
	auto mpos_y = ImGui::GetIO().MousePos.y;

	bool is_hovered =
		start_x <= mpos_x && mpos_x <= stop_x &&
		start_y <= mpos_y && mpos_y <= stop_y;

	auto rect_min = ImVec2{ start_x, start_y };
	auto rect_max = ImVec2{ stop_x,  stop_y  };
	auto rect_color = is_hovered ? options.rect_color_hovered : options.rect_color;
	io_painter.draw_list->AddRectFilled(rect_min, rect_max, rect_color, options.rounding);

	if (width > 32) {
		rect_min.x = std::max(rect_min.x, io_painter.canvas_pos.x);
		rect_min.y = std::max(rect_min.y, io_painter.canvas_pos.y);
		rect_max.x = std::min(rect_max.x, io_painter.canvas_pos.x + io_painter.canvas_size.x);
		rect_max.y = std::min(rect_max.y, io_painter.canvas_pos.y + io_painter.canvas_size.y);

		io_painter.draw_list->PushClipRect(rect_min, rect_max);
		auto text = loguru::strprintf("%s %s %6.3f ms", record.id, record.extra, record.duration_ns * 1e-6f);
		io_painter.add_text(ImVec2{start_x + 4, start_y + 0.5f * (options.rect_height - options.font_size)},
							text, options.text_color, options.font_size);
		io_painter.draw_list->PopClipRect();
	}

	return is_hovered ? PaintResult::kHovered : PaintResult::kNormal;
}

PaintResult paint_scope(Painter& io_painter, const Options& options,
				 const profiler::Stream& stream,
				 const profiler::Scope& scope, size_t depth)
{
	auto result = paint_record(io_painter, options, scope.record, depth);
	if (result == PaintResult::kCulled) { return result; }

	size_t num_children = 0;
	size_t idx = scope.child_idx;
	while (auto child = profiler::parse_scope(stream, idx)) {
		paint_scope(io_painter, options, stream, *child, depth + 1);
		idx = child->next_idx;
		num_children += 1;
	}
	CHECK_EQ_F(idx, scope.child_end_idx);

	if (result == PaintResult::kHovered) {
		ImGui::BeginTooltip();
		ImGui::Text("id:       %s",       scope.record.id);
		ImGui::Text("extra:    %s",       scope.record.extra);
		ImGui::Text("duration: %6.3f ms", scope.record.duration_ns * 1e-6f);
		ImGui::Text("children: %lu",      num_children);
		ImGui::EndTooltip();
	}

	return result;
}

void color_edit_4(const char* label, ImU32* color_u32)
{
	auto color_float = ImGui::ColorConvertU32ToFloat4(*color_u32);
	ImGui::ColorEdit4(label, &color_float.x);
	*color_u32 = ImGui::ColorConvertFloat4ToU32(color_float);
}

void show_options(Options& options)
{
	ImGui::ColorEditMode(ImGuiColorEditMode_UserSelectShowButton);
	ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);

	ImGui::SliderFloat("Time scale", &options.points_per_ns, 1e-8f,  1e-3f,  "%.10f points/us", 10);
	ImGui::DragFloat("scroll_speed", &options.scroll_speed,  0.05f,  0.0f);
	ImGui::DragFloat("pinch_speed",  &options.pinch_speed,   0.05f,  0.0f);
	ImGui::SliderFloat("Font size",  &options.font_size,     4,     24,     "%.0f");
	color_edit_4("Rect color",         &options.rect_color);
	color_edit_4("Rect color hovered", &options.rect_color_hovered);
	color_edit_4("Grid color",         &options.grid_color);
}

void paint_grid(Painter& painter, const Options& options, NanoSeconds start_ns, NanoSeconds stop_ns)
{
	if (options.points_per_ns <= 0) { return; }

	auto screen_width_ns = painter.canvas_size.x / options.points_per_ns;

	auto step_size = 1000 * 1000;
	auto num_steps = screen_width_ns / step_size;
	while (num_steps > 20) {
		step_size *= 10;
		num_steps /= 10;
	}

	auto grid_ns = start_ns;
	for (;;) {
		float grid_x = options.point_from_ns(grid_ns);

		if (grid_x > painter.canvas_pos.x + painter.canvas_size.x) { break; }

		painter.draw_list->AddLine(
			{painter.canvas_pos.x + grid_x, painter.canvas_pos.y},
			{painter.canvas_pos.x + grid_x, painter.canvas_pos.y + painter.canvas_size.y},
			options.grid_color);

		painter.add_text(
			{painter.canvas_pos.x + grid_x, painter.canvas_pos.y + painter.canvas_size.y - options.font_size},
			loguru::strprintf("%.0f ms", 1e-6f * (grid_ns - start_ns)),
			options.text_color, options.font_size);

		if (grid_x > options.point_from_ns(stop_ns)) { break; }
		grid_ns += step_size;
	};
}

struct MergedScope
{
	Record             record;
	std::vector<Scope> basis; // We are these scopes merged.
};

std::vector<MergedScope> merge_scopes(const std::vector<Scope>& scopes)
{
	if (scopes.empty()) { return {}; }

	std::vector<MergedScope> merged_scopes;
	std::unordered_map<std::string, size_t> id_to_index;

	for (const auto& scope : scopes) {
		auto it = id_to_index.find(scope.record.id);
		size_t index;
		if (it == id_to_index.end()) {
			index = merged_scopes.size();
			id_to_index[scope.record.id] = index;
			MergedScope merged;
			merged.record = scope.record;
			merged.record.extra = "";
			merged.record.duration_ns = 0;
			merged_scopes.push_back(merged);
		} else {
			index = it->second;
		}

		auto& merged = merged_scopes[index];
		merged.record.start_ns = std::min(merged.record.start_ns, scope.record.start_ns);
		merged.record.duration_ns += scope.record.duration_ns;
		merged.basis.push_back(scope);
	}

	// Position with no overlap:
	std::sort(merged_scopes.begin(), merged_scopes.end(),
		[](const auto& a, const auto& b) { return a.record.start_ns < b.record.start_ns; });

	auto ns = merged_scopes[0].record.start_ns;

	for (auto& merged_scope : merged_scopes)
	{
		merged_scope.record.start_ns = std::max(merged_scope.record.start_ns, ns);
		ns = merged_scope.record.start_ns + merged_scope.record.duration_ns;
	}

	return merged_scopes;
}

void paint_merged_scope(
	Painter&           io_painter,
	const Options&     options,
	const Stream&      stream,
	const MergedScope& merged_scope,
	size_t             depth)
{
	auto result = paint_record(io_painter, options, merged_scope.record, depth);
	if (result == PaintResult::kCulled) { return; }

	std::vector<Scope> child_scopes;
	for (const auto& basis_scope : merged_scope.basis) {

		size_t idx = basis_scope.child_idx;
		while (auto child = profiler::parse_scope(stream, idx)) {
			auto child_scope = *child;
			// Make child relative to parent:
			child_scope.record.start_ns -= basis_scope.record.start_ns;
			child_scopes.push_back(child_scope);
			idx = child->next_idx;
		}
		CHECK_EQ_F(idx, basis_scope.child_end_idx);
	}

	auto merged_children = merge_scopes(child_scopes);
	for (auto& merged_child : merged_children) {
		// Make child in world units again:
		for (auto& basis : merged_child.basis) {
			basis.record.start_ns += merged_scope.record.start_ns;
		}
		merged_child.record.start_ns += merged_scope.record.start_ns;
		paint_merged_scope(io_painter, options, stream, merged_child, depth + 1);
	}

	if (result == PaintResult::kHovered) {
		ImGui::BeginTooltip();
		ImGui::Text("id:           %s",       merged_scope.record.id);
		ImGui::Text("sum duration: %6.3f ms", merged_scope.record.duration_ns * 1e-6f);
		ImGui::Text("sum of:       %lu",      merged_scope.basis.size());
		ImGui::EndTooltip();
	}
}

void paint_profiler_gui(const Input& input)
{
	static Options s_options;
	static profiler::ThreadStreams s_thread_streams;
	static bool s_merge_ids = true;

	if (s_thread_streams.empty() || ImGui::Button("First frame")) {
		s_thread_streams = profiler::ProfilerMngr::instance().first_frame();
	}

	if (ImGui::Button("Capture new frame")) {
		s_thread_streams = profiler::ProfilerMngr::instance().last_frame();
	}

	const auto main_thread_stream = s_thread_streams[std::this_thread::get_id()].stream;

	auto top_scopes = collectScopes(main_thread_stream, 0);
	if (top_scopes.empty()) { return; }

	ImGui::Checkbox("Merge ID:s", &s_merge_ids);

	std::vector<MergedScope> merged;
	NanoSeconds start_ns, duration_ns;

	if (s_merge_ids) {
		merged = merge_scopes(top_scopes);
		start_ns = top_scopes.front().record.start_ns;
		duration_ns  = top_scopes.back().record.duration_ns;
	} else {
		start_ns = top_scopes.front().record.start_ns;
		duration_ns  = top_scopes.back().record.duration_ns;
	}

	if (ImGui::Button("Reset view") || s_options.points_per_ns <= 0) {
		s_options.points_per_ns = ImGui::GetContentRegionAvail().x / duration_ns;
		s_options.scroll_x_points = 0;
	}

	s_options.start_ns = start_ns;

	show_options(s_options);

	Painter painter;

	// ------------------------------------------------------------------------

	s_options.scroll_x_points -= input.scroll_x * s_options.scroll_speed;

	auto zoom_factor = 1 + (input.pinch_zoom - 1) * s_options.pinch_speed;
	s_options.points_per_ns *= (float)zoom_factor;
	auto zoom_center = input.pinch_center_x - painter.canvas_pos.x;
	s_options.scroll_x_points = (s_options.scroll_x_points - zoom_center ) * zoom_factor + zoom_center;

	// ------------------------------------------------------------------------

	paint_grid(painter, s_options, start_ns, start_ns + duration_ns);

	// ------------------------------------------------------------------------

	if (s_merge_ids) {
		for (const auto& merged_scope : merged) {
			paint_merged_scope(painter, s_options, main_thread_stream, merged_scope, 0);
		}
	} else {
		for (const auto& scope : top_scopes) {
			paint_scope(painter, s_options, main_thread_stream, scope, 0);
		}
	}
}

} // namespace profiler
