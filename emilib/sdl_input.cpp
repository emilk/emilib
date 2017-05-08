// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "sdl_input.hpp"

#include <cmath>

#include <SDL2/SDL_events.h>

#define PROPER_PINCH_INPUT 1

namespace emilib {
namespace input {

float distance(const Vec2f& a, const Vec2f& b)
{
	return std::hypot(a.x - b.x, a.y - b.y);
}

bool check_for_pinch_gesture(PinchState* pinch_state, const TrackpadMap& prev, const TrackpadMap& next)
{
	*pinch_state = {};
	if (prev.size() != 2 || next.size() != 2) { return false; }

	const auto& prev_touch_0 = *prev.begin();
	const auto& prev_touch_1 = *std::next(prev.begin());
	if (next.count(prev_touch_0.first) == 0) { return false; }
	if (next.count(prev_touch_1.first) == 0) { return false; }

	const auto& next_touch_0 = *next.find(prev_touch_0.first);
	const auto& next_touch_1 = *next.find(prev_touch_1.first);

	Vec2f prev_pos[2] = { prev_touch_0.second, prev_touch_1.second };
	Vec2f next_pos[2] = { next_touch_0.second, next_touch_1.second };

	const float prev_center_x = 0.5f * (prev_pos[0].x + prev_pos[1].x);
	const float prev_center_y = 0.5f * (prev_pos[0].y + prev_pos[1].y);
	const float next_center_x = 0.5f * (next_pos[0].x + next_pos[1].x);
	const float next_center_y = 0.5f * (next_pos[0].y + next_pos[1].y);
	float prev_dist = distance(prev_pos[0], prev_pos[1]);
	float next_dist = distance(next_pos[0], next_pos[1]);

	pinch_state->scroll.x = next_center_x - prev_center_x;
	pinch_state->scroll.y = next_center_y - prev_center_y;
	if (prev_dist > 0 && next_dist > 0) {
		pinch_state->zoom = next_dist / prev_dist;
	}
	// pinch_state->pinch_center.x = next_center_x;
	// pinch_state->pinch_center.y = next_center_y;
	pinch_state->is_active = true;
	return true;
}

void on_touch_event(State* state, const Callbacks& callbacks, TouchEvent e, FingerID finger_id, MouseButton button_state, Vec2f pos, uint32_t time_ms)
{
	if (finger_id == SDL_TOUCH_MOUSEID) {
		state->mouse_pos = pos;
	}

	auto& touch = state->touches[finger_id];
	if (e == TouchEvent::kDown) {
		touch = Touch();
		touch.id           = finger_id;
		touch.pos          = pos;
		touch.down         = true;
		touch.time_ms      = time_ms;
		touch.button_state = button_state;
	} else {
		auto dt = 1e-3f * (float)(time_ms - touch.time_ms);
		touch.rel.x        = pos.x - touch.pos.x;
		touch.rel.y        = pos.y - touch.pos.y;
		touch.pos          = pos;
		touch.button_state = button_state;
		touch.time_ms      = time_ms;

		if (dt > 0) {
			touch.vel.x = touch.rel.x / dt;
			touch.vel.y = touch.rel.y / dt;
		} else {
			// Sustain last velocity
		}
	}

	if (touch.down && callbacks.touch) {
		callbacks.touch(e, touch);
	}

	if (e == TouchEvent::kUp) {
		state->touches.erase(finger_id);
	}
}

MouseButton mouse_buttons(const SDL_MouseMotionEvent& motion)
{
	int button_state = 0;

	if (motion.state & SDL_BUTTON_LMASK) {
		button_state |= MouseButton::kPrimary;
	}

	if (motion.state & SDL_BUTTON_RMASK) {
		button_state |= MouseButton::kSecondary;
	}

	if (motion.state & SDL_BUTTON_MMASK) {
		button_state |= MouseButton::kMiddle;
	}

	return (MouseButton)button_state;
}

Vec2f game_from_touch(const Context& context, const SDL_TouchFingerEvent& t)
{
	return {t.x * context.window_size_points.x, t.y * context.window_size_points.y};
}

Vec2f game_from_mouse(const Context& context, const SDL_MouseMotionEvent& t)
{
	return { (float)t.x, (float)t.y };
};

void handle_event(State* state, const Context& context, const Callbacks& callbacks, const SDL_Event& event)
{
	if (event.type == SDL_QUIT && callbacks.quit) {
		callbacks.quit();
	}

	// We cannot handle SDL_APP_ here - that's done with an SDL_SetEventFilter

#if TARGET_OS_IPHONE
	if (event.type == SDL_FINGERDOWN) {
		auto&& t = event.tfinger;
		on_touch_event(state, callbacks, TouchEvent::kDown, t.fingerId, MouseButton::kPrimary, game_from_touch(context, t), t.timestamp);
	}

	if (event.type == SDL_FINGERMOTION) {
		// Also for mac trackpad touches, kind of.
		auto&& t = event.tfinger;
		on_touch_event(state, callbacks, TouchEvent::kMove, t.fingerId, MouseButton::kPrimary, game_from_touch(context, t), t.timestamp);
	}

	if (event.type == SDL_FINGERUP) {
		auto&& t = event.tfinger;
		on_touch_event(state, callbacks, TouchEvent::kUp, t.fingerId, MouseButton::kPrimary, game_from_touch(context, t), t.timestamp);
	}
#else // !TARGET_OS_IPHONE
	// Mac trackpad:
	if (event.type == SDL_FINGERDOWN) {
		auto&& t = event.tfinger;
		state->trackpad[t.fingerId] = game_from_touch(context, t);
	}

	if (event.type == SDL_FINGERMOTION) {
		auto&& t = event.tfinger;
		state->trackpad[t.fingerId] = game_from_touch(context, t);
	}

	if (event.type == SDL_FINGERUP) {
		auto&& t = event.tfinger;
		state->trackpad.erase(t.fingerId);
	}
#endif // !TARGET_OS_IPHONE

#if 1
	// iOS-touches doubly reported as mouse events
	if (event.type == SDL_MOUSEBUTTONDOWN && event.motion.which != SDL_TOUCH_MOUSEID) {
		auto&& m = event.motion;
		on_touch_event(state, callbacks, TouchEvent::kDown, SDL_TOUCH_MOUSEID, mouse_buttons(m), game_from_mouse(context, m), m.timestamp);
	}

	if (event.type == SDL_MOUSEMOTION && event.motion.which != SDL_TOUCH_MOUSEID) {
		auto&& m = event.motion;
		on_touch_event(state, callbacks, TouchEvent::kMove, SDL_TOUCH_MOUSEID, mouse_buttons(m), game_from_mouse(context, m), m.timestamp);
	}

	if (event.type == SDL_MOUSEBUTTONUP && event.motion.which != SDL_TOUCH_MOUSEID) {
		auto&& m = event.motion;
		on_touch_event(state, callbacks, TouchEvent::kUp, SDL_TOUCH_MOUSEID, mouse_buttons(m), game_from_mouse(context, m), m.timestamp);
	}
#endif

#if !PROPER_PINCH_INPUT
	if (event.type == SDL_MOUSEWHEEL) {
		state->pinch_state->scroll.x = event.wheel.x;
		state->pinch_state->scroll.y = event.wheel.y;
	}

	if (event.type == SDL_MULTIGESTURE) {
		// state->pinch_state.pinch_rotation = event.mgesture.dTheta;
		state->pinch_state.zoom = 1 + event.mgesture.dDist;
		// state->pinch_state.pinch_center.x = event.mgesture.x * context.window_size_points.x;
		// state->pinch_state.pinch_center.y = event.mgesture.y * context.window_size_points.y;
		state->pinch_state.pinch_center.x = state->mouse_pos.x;
		state->pinch_state.pinch_center.y = state->mouse_pos.y;
	}
#endif // PROPER_PINCH_INPUT

	if (event.type == SDL_KEYDOWN && callbacks.key_down) {
		callbacks.key_down(event.key.keysym.sym);
	}

	if (event.type == SDL_TEXTINPUT && callbacks.text) {
		callbacks.text(event.text.text);
	}

	if (event.type == SDL_WINDOWEVENT) {
		if (event.window.event == SDL_WINDOWEVENT_RESIZED && callbacks.resize_window) {
			auto width_points = event.window.data1;
			auto height_points = event.window.data2;
			callbacks.resize_window(width_points, height_points);
		}
	}

	if (callbacks.sdl_event) {
		callbacks.sdl_event(event);
	}
}

void poll_for_events(State* state, const Context& context, const Callbacks& callbacks)
{
#if PROPER_PINCH_INPUT
	const auto trackpad_before = state->trackpad;
#endif

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		handle_event(state, context, callbacks, event);
	}

#if TARGET_OS_IPHONE && PROPER_PINCH_INPUT
	state->trackpad.clear();
	for (const auto& touch : state->touches) {
		state->trackpad[touch.first] = touch.second.pos;
	}
#endif

#if PROPER_PINCH_INPUT
	check_for_pinch_gesture(&state->pinch_state, trackpad_before, state->trackpad);
#endif
}

} // namespace input
} // namespace emilib
