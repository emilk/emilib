// By Emil Ernerfeldt 2015-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

union SDL_Event;

namespace emilib {

/**
 * Provides bindings between Dear ImGui and SDL. Handles input, copy-paste etc.
 * Does NOT handle painting! Please use imgui_gl_lib.hpp for that.
 * You should have your own even loop and feed the events to ImGui_SDL.
 *
 *	emilib::sdl::Params sdl_params;
 *	sdl_params.window_name = "My window";
 *	auto sdl = emilib::sdl::init(sdl_params);
 *
 *	emilib::ImGui_SDL imgui_sdl(sdl.width_points, sdl.height_points, sdl.pixels_per_point);
 *
 *	gl::bind_imgui_painting();
 *
 *	bool quit = false;
 *	while (!quit) {
 *		SDL_Event event;
 *		while (SDL_PollEvent(&event)) {
 *			if (event.type == SDL_QUIT) { quit = true; }
 *			imgui_sdl.on_event(event);
 *		}
 *
 *		// Handle window resize:
 *		gl::TempViewPort::set_back_buffer_size(
 *			(int)std::round(imgui_sdl.width_pixels()),
 *			(int)std::round(imgui_sdl.height_pixels()));
 *
 *		imgui_sdl.new_frame();
 *
 *		if (ImGui::Button("Quit!")) {
 *			quit = true;
 *		}
 *
 *		imgui_sdl.paint();
 *
 *		glClearColor(0.1f, 0.1f, 0.1f, 0);
 *		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
 *		gl::paint_imgui();
 *		SDL_GL_SwapWindow(sdl.window);
 *	}
 */
class ImGui_SDL
{
public:
	ImGui_SDL(float width_points, float height_points, float pixels_per_point);
	~ImGui_SDL(); ///< Will call ImGui::Shutdown

	/// When invisible, you can still call ImGui functions, but you cannot see or interact any widgets.
	bool visible() const { return _visible; }
	void set_visible(bool v) { _visible = v; }

	/// When not interactive, any ImGui widgets are passive (you can't click them).
	bool interactive() const { return _interactive; }
	void set_interactive(bool v) { _interactive = v; }

	/// Call once at the start of each frame.
	void new_frame();

	/// Call once at the end of each frame.
	void paint();

	/// You must call this yourself!
	void on_event(const SDL_Event& e);

	/// Key modifiers:
	bool mod_command() const;
	bool mod_shift() const;

	/// Window sizes.
	/// Note that on a High DPI display (e.g a retina Mac) there are many pixels per point.
	float width_points()     const { return _width_points;     }
	float height_points()    const { return _height_points;    }
	float width_pixels()     const { return _pixels_per_point * _width_points;  }
	float height_pixels()    const { return _pixels_per_point * _height_points; }
	float pixels_per_point() const { return _pixels_per_point; }

	void set_size_points(float width_points, float height_points);

private:
	bool  _visible = true;
	bool  _interactive = true;
	float _width_points;
	float _height_points;
	float _pixels_per_point;
};

} // namespace emilib
