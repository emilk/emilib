// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// PURPOSE:
//   sdl_input wraps the input events from SDL with an easy-to-use
//   interface that is unified for desktop and mobile.

#pragma once

#include <functional>
#include <map>

#include <SDL2/SDL_keycode.h>

union SDL_Event;

namespace emilib {
namespace input {

/// matches SDL_FingerID
using FingerID = int64_t;

enum class TouchEvent
{
	kDown,
	kMove,
	kUp,
};

enum MouseButton
{
	kNone      = 0,
	kPrimary   = 1,  // Left
	kSecondary = 2,  // Right
	kMiddle    = 4,  // Middle
};

/// All coordinates are in points.
struct Vec2f
{
	float x, y;
};

/// All coordinates are in points.
struct Touch
{
	FingerID    id;                                ///< Unique id of this touch.
	uint32_t    time_ms;                           ///< Time of last touch
	Vec2f       pos          = {0,0};              ///< Last known position
	Vec2f       rel          = {0,0};              ///< Last movement
	Vec2f       vel          = {0,0};              ///< Velocity
	bool        down         = false;              ///< Can only be false for mouse cursors
	MouseButton button_state = MouseButton::kNone; ///< Bit-or of MouseButton
};

struct PinchState
{
	bool  is_active = false; ///< True if two fingers are on the track-pad.
	Vec2f scroll    = {0,0}; ///< Delta in points.
	float zoom      = 1;     ///< How many times further apart are the fingers now?
	// Vec2f pinch_center = {0,0}; ///< Center of pinch gesture in points.
};

using TouchCallback    = std::function<void(TouchEvent, Touch)>;
using ResizeCallback   = std::function<void(int width_points, int height_points)>;
using KeyboardCallback = std::function<void(SDL_Keycode key_down)>;
using TextCallback     = std::function<void(const char* utf8)>;

struct Callbacks
{
	KeyboardCallback               key_down;
	ResizeCallback                 resize_window;
	std::function<void()>          quit;
	std::function<void(SDL_Event)> sdl_event;
	TextCallback                   text;
	TouchCallback                  touch;
};

using TrackpadMap = std::map<FingerID, Vec2f>;
using TouchMap    = std::map<FingerID, Touch>;

struct State
{
	PinchState  pinch_state;
	Vec2f       mouse_pos;
	TouchMap    touches;
	TrackpadMap trackpad;
};

/// You must fill this in before polling for events.
struct Context
{
	/// Size of the full window in points.
	Vec2f window_size_points;
};

void poll_for_events(State* state, const Context& context, const Callbacks& callbacks);

} // namespace input
} // namespace emilib
