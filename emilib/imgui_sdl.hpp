// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

union SDL_Event;

namespace emilib {

/*
Provides bindings between Dear ImGui and SDL. Handles input, copy-past etc.
Does NOT handle painting! Please use imgui_gl_lib.hpp for that.
You should have your own even loop and feed the events to ImGui_SDL;
*/
class ImGui_SDL
{
public:
	ImGui_SDL(float width_points, float height_points, float pixels_per_point);
	~ImGui_SDL(); // Will call ImGui::Shutdown

	// When inactive, you cannot interact with Dear ImGui.
	bool active() const { return _active; }
	void set_active(bool v) { _active = v; }

	void new_frame();
	void paint();

	// You must call this yourself!
	void on_event(const SDL_Event& e);

	// Key modifiers:
	bool mod_command() const;
	bool mod_shift() const;

private:
	bool  _active = true;
	float _pixels_per_point;
};

} // namespace emilib
