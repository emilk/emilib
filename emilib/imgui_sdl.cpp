// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "imgui_sdl.hpp"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <string>

#include <imgui/imgui.h>
#include <SDL2/SDL.h>

namespace emilib {

static const char* get_clipboard_text_callback()
{
	return SDL_GetClipboardText();
}

static void set_clipboard_text_callback(const char* text)
{
	SDL_SetClipboardText(text);
}

ImGui_SDL::ImGui_SDL(float width_points, float height_points, float pixels_per_point) : _pixels_per_point(pixels_per_point)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = {width_points, height_points};
	io.DeltaTime = 1.0f/60.0f;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab]        = SDL_SCANCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow]  = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow]    = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow]  = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_Home]       = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End]        = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete]     = SDL_SCANCODE_DELETE;
	io.KeyMap[ImGuiKey_Backspace]  = SDL_SCANCODE_BACKSPACE;
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
	ImGui::Shutdown();
}

void ImGui_SDL::new_frame()
{
	if (!active()) { return; }

	ImGuiIO& io = ImGui::GetIO();

	// Setup timestep
	static double s_last_time = 0.0f;
	const double current_time = SDL_GetTicks() / 1000.0;
	io.DeltaTime = (float)(current_time - s_last_time);
	io.DeltaTime = std::max(io.DeltaTime, 0.0001f);
	s_last_time = current_time;

	// Setup inputs
	int mouse_x, mouse_y;
	auto mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);

	io.MousePos = {(float)mouse_x, (float)mouse_y}; // Works for retina Mac, but not iOS !?

#if TARGET_OS_IPHONE
	int width_pixels, height_pixels;
	SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &width_pixels, &height_pixels);
	float pixels_per_point = static_cast<float>(width_pixels) / io.DisplaySize.x;
	io.MousePos.x /= pixels_per_point;
	io.MousePos.y /= pixels_per_point;
#endif

	io.MouseDown[0] = (mouse_state & SDL_BUTTON_LMASK);
	io.MouseDown[1] = (mouse_state & SDL_BUTTON_RMASK);

	// Start the frame
	ImGui::NewFrame();
}

void ImGui_SDL::paint()
{
	if (!active()) { return; }

	ImGui::Render();
}

void ImGui_SDL::on_event(const SDL_Event& event)
{
	// Ignore active() - we still want to update keys

	ImGuiIO& io = ImGui::GetIO();
	auto num_imgui_keys = sizeof(io.KeysDown) / sizeof(io.KeysDown[0]);

	switch (event.type)
	{
		case SDL_KEYDOWN: case SDL_KEYUP: {
			auto sym = event.key.keysym.scancode;
			if (sym < num_imgui_keys) {
				io.KeysDown[sym] = (event.key.state == SDL_PRESSED);
			}
			if (event.key.keysym.sym == SDLK_LGUI || event.key.keysym.sym == SDLK_RGUI) {
				io.KeyCtrl = (event.key.state == SDL_PRESSED);
			}
			if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT) {
				io.KeyShift = (event.key.state == SDL_PRESSED);
			}
		} break;

		case SDL_MOUSEWHEEL: {
			io.MouseWheel = event.wheel.y;
		} break;

		case SDL_TEXTINPUT: {
			const char* utf8 = event.text.text;

			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf32conv;
			std::u32string utf32 = utf32conv.from_bytes(utf8);
			if (utf32.size() == 1 && utf32[0] < 0x10000) {
				io.AddInputCharacter((unsigned short)utf32[0]);
			}
		} break;

		case SDL_WINDOWEVENT: {
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				auto w = event.window.data1;
				auto h = event.window.data2;
				LOG_F(INFO, "Resized: %dx%d points", w, h);
				io.DisplaySize.x = w;
				io.DisplaySize.y = h;
			}
		} break;
	}
}

// Key modifiers:
bool ImGui_SDL::mod_command() const
{
	return ImGui::GetIO().KeyCtrl;
}

bool ImGui_SDL::mod_shift() const
{
	return ImGui::GetIO().KeyShift;
}

} // namespace emilib
