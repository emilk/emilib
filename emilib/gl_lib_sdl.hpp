// By Emil Ernerfeldt 2013-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <SDL2/SDL.h>

namespace emilib {
namespace sdl {

struct Params
{
	const char* window_name = "emilib";

	size_t width_points  = 1024;
	size_t height_points = 768;

	int depth_buffer   = 0; ///< e.g. 24
	int stencil_buffer = 0; ///< e.g. 8
	int msa            = 0; ///< e.g. 8 for 8-point anti-aliasing.

	// Enable high-dpi screens.
	bool high_dpi = true;
};

struct InitResult
{
	SDL_Window*   window;
	SDL_GLContext gl_context;
	size_t        width_points;
	size_t        height_points;
	size_t        width_pixels;
	size_t        height_pixels;
	float         pixels_per_point;
};

/// Init SDL and glew.
InitResult init(const Params& params);

} // namespace sdl
} // namespace emilib
