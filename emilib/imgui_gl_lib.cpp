// By Emil Ernerfeldt 2015-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include <emilib/gl_lib.hpp>
#include <emilib/gl_lib_opengl.hpp>
#include <imgui/imgui.h>

namespace emilib {
namespace gl {

static gl::Program_UP     s_prog;
static gl::MeshPainter_UP s_mesh_painter;

static void paint_imgui_draw_lists(ImDrawData* draw_data)
{
	if (!draw_data) { return; }

	EMILIB_GL_PAINT_FUNCTION();

	// Setup render state
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	s_prog->bind();

	// Setup texture
	glActiveTexture(GL_TEXTURE0);

	// Setup orthographic projection matrix
	const float width = ImGui::GetIO().DisplaySize.x;
	const float height = ImGui::GetIO().DisplaySize.y;
	float sx = 2.0f / width;
	float sy = -2.0f / height;
	float mvp[16] = {
		sx, 0,  0, 0,
		 0, sy, 0, 0,
		 0, 0,  1, 0,
		-1, 1,  0, 1,
	};

	s_prog->set_uniform("u_sampler", 0);
	glUniformMatrix4fv(s_prog->get_uniform_loc("u_mvp"), 1, false, mvp);
	const auto u_clip_loc = s_prog->get_uniform_loc("u_clip");

	for (int n = 0; n < draw_data->CmdListsCount; ++n)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

		ImDrawVert* vert_dest = s_mesh_painter->allocate_vert<ImDrawVert>(cmd_list->VtxBuffer.size());
		std::copy_n(&cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size(), vert_dest);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
                // User callback (registered via ImDrawList::AddCallback)
				pcmd->UserCallback(cmd_list, pcmd);

				// Restore some state UserCallback may have messed up:
				s_prog->bind();
				glActiveTexture(GL_TEXTURE0);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glUniform4f(u_clip_loc, pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);

				uint32_t* index_dest = s_mesh_painter->allocate_indices(pcmd->ElemCount);
				std::copy_n(idx_buffer, pcmd->ElemCount, index_dest);

				s_mesh_painter->paint(*s_prog, GL_TRIANGLES);
			}
			idx_buffer += pcmd->ElemCount;
		}
	}
}

gl::Program_UP load_shader()
{
	const auto vs = R"(
		vs_in vec2 a_pos;
		vs_in vec2 a_tc;
		vs_in vec4 a_color;

		vs_out vec2 v_tc;
		vs_out vec4 v_color;
		vs_out vec2 v_pixel;

		uniform mat4 u_mvp;

		void main() {
			gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
			v_tc    = a_tc;
			v_color = a_color;
			v_pixel = a_pos;
		}
	)";

	const auto fs = R"(
		fs_in vec2 v_tc;
		fs_in vec4 v_color;
		fs_in vec2 v_pixel;

		uniform sampler2D u_sampler;
		uniform vec4 u_clip; // min_x, min_y, max_x, max_y

		void main() {
			if (v_pixel.x < u_clip.x || u_clip.z < v_pixel.x ||
				v_pixel.y < u_clip.y || u_clip.w < v_pixel.y)
			{
				discard;
			}
			out_FragColor = v_color * texture2D(u_sampler, v_tc);
		}
	)";

	return gl::Program_UP{new gl::Program{gl::compile_program(vs, fs, "imgui")}};
}

void bind_imgui_painting()
{
	ImGuiIO& io = ImGui::GetIO();

	// Load font texture;
	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Default font (embedded in code)
	unsigned char* tex_data;
	int tex_x, tex_y;
	io.Fonts->GetTexDataAsRGBA32(&tex_data, &tex_x, &tex_y);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
	io.Fonts->TexID = (void *)(intptr_t)tex_id;

	s_prog = load_shader();

	s_mesh_painter.reset(new gl::MeshPainter(gl::Usage::WRITE_MANY_READ_MANY, {
		gl::VertComp::Vec2f("a_pos"),
		gl::VertComp::Vec2f("a_tc"),
		gl::VertComp::RGBA32("a_color"),
	}));
}

void paint_imgui()
{
	paint_imgui_draw_lists(ImGui::GetDrawData());
}

void unbind_imgui_painting()
{
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts = nullptr;
	s_prog = nullptr;
	s_mesh_painter = nullptr;
}

} // namespace gl
} // namespace emilib
