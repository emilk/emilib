// By Emil Ernerfeldt 2015-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "imgui_sdl.hpp"

#include <algorithm>
#include <string>

#include <imgui/imgui.h>
#include <SDL2/SDL.h>

namespace emilib {

static const char* get_clipboard_text_callback(void*)
{
	return SDL_GetClipboardText();
}

static void set_clipboard_text_callback(void*, const char* text)
{
	SDL_SetClipboardText(text);
}

ImGui_SDL::ImGui_SDL(float width_points, float height_points, float pixels_per_point)
	: _width_points(width_points)
	, _height_points(height_points)
	, _pixels_per_point(pixels_per_point)
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = {width_points, height_points};
	io.DeltaTime = 1.0f / 60.0f; // Whatever – this will be properly measured later on.

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab]        = SDL_SCANCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow]  = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow]    = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow]  = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp]     = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown]   = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home]       = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End]        = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert]     = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete]     = SDL_SCANCODE_DELETE;
	io.KeyMap[ImGuiKey_Backspace]  = SDL_SCANCODE_BACKSPACE;
	io.KeyMap[ImGuiKey_Space]      = SDL_SCANCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter]      = SDL_SCANCODE_RETURN;
	io.KeyMap[ImGuiKey_Escape]     = SDL_SCANCODE_ESCAPE;
	io.KeyMap[ImGuiKey_A]          = SDLK_a;
	io.KeyMap[ImGuiKey_C]          = SDLK_c;
	io.KeyMap[ImGuiKey_V]          = SDLK_v;
	io.KeyMap[ImGuiKey_X]          = SDLK_x;
	io.KeyMap[ImGuiKey_Y]          = SDLK_y;
	io.KeyMap[ImGuiKey_Z]          = SDLK_z;

	io.SetClipboardTextFn = set_clipboard_text_callback;
	io.GetClipboardTextFn = get_clipboard_text_callback;
}

ImGui_SDL::~ImGui_SDL()
{
	ImGui::DestroyContext();
}

void ImGui_SDL::new_frame()
{
	ImGuiIO& io = ImGui::GetIO();

	// Setup timestep:
	static double s_last_time = 0.0f;
	const double current_time = SDL_GetTicks() / 1000.0;
	io.DeltaTime = (float)(current_time - s_last_time);
	io.DeltaTime = std::max(io.DeltaTime, 0.0001f);
	s_last_time = current_time;

	if (interactive()) {
		// Setup inputs
		int mouse_x, mouse_y;
		auto mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
		io.MousePos = {(float)mouse_x, (float)mouse_y};
		io.MouseDown[0] = (mouse_state & SDL_BUTTON_LMASK);
		io.MouseDown[1] = (mouse_state & SDL_BUTTON_RMASK);
		io.MouseDown[2] = (mouse_state & SDL_BUTTON_MMASK);
	} else {
		io.MouseDown[0] = false;
		io.MouseDown[1] = false;
		io.MouseDown[2] = false;
		io.MousePos.x = -FLT_MAX;
		io.MousePos.y = -FLT_MAX;

		memset(io.KeysDown, 0, sizeof(io.KeysDown));
		io.KeyShift = false;
		io.KeyCtrl  = false;
		io.KeyAlt   = false;
		io.KeySuper = false;
	}

	// Hide OS mouse cursor if ImGui is drawing it
	SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

	// Start the frame
	ImGui::NewFrame();
}

void ImGui_SDL::paint()
{
	if (visible()) {
		ImGui::Render();
	}
}

void ImGui_SDL::on_event(const SDL_Event& event)
{
	if (!interactive()) { return; }

	ImGuiIO& io = ImGui::GetIO();
	auto num_imgui_keys = sizeof(io.KeysDown) / sizeof(io.KeysDown[0]);

	switch (event.type)
	{
		case SDL_KEYDOWN: case SDL_KEYUP: {
			int key = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
			if (key < num_imgui_keys) {
				io.KeysDown[key] = (event.type == SDL_KEYDOWN);
			}
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl  = ((SDL_GetModState() & KMOD_CTRL)  != 0);
			io.KeyAlt   = ((SDL_GetModState() & KMOD_ALT)   != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI)   != 0);
		} break;

		case SDL_MOUSEWHEEL: {
			io.MouseWheel = event.wheel.y;
		} break;

		case SDL_TEXTINPUT: {
			io.AddInputCharactersUTF8(event.text.text);
		} break;

		case SDL_WINDOWEVENT: {
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				set_size_points(event.window.data1, event.window.data2);
				VLOG_F(1, "Resized to %fx%f points", _width_points, _height_points);
			}
		} break;
	}
}

bool ImGui_SDL::mod_command() const
{
	return ImGui::GetIO().KeyCtrl;
}

bool ImGui_SDL::mod_shift() const
{
	return ImGui::GetIO().KeyShift;
}

void ImGui_SDL::set_size_points(float width_points, float height_points)
{
	_width_points = width_points;
	_height_points = height_points;
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = _width_points;
	io.DisplaySize.y = _height_points;
}

} // namespace emilib
