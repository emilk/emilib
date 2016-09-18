// By Emil Ernerfeldt 2013-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "gl_lib_sdl.hpp"

#include <cmath>

#include "gl_lib_opengl.hpp"

#if TARGET_OS_IPHONE
	#include "os.hpp"
#endif

namespace emilib {
namespace sdl {

#ifdef GLEW_OK
// Error callback
void onGLError(
	GLenum        source,
	GLenum        type,
	GLuint        id,
	GLenum        severity,
	GLsizei       length,
	const GLchar* message,
	const void*   user_param)
{
	LOG_F(WARNING, "GL debug: %s", message);
}

void init_glew()
{
	CHECK_FOR_GL_ERROR;
	glewExperimental = true;
	GLenum glew_err = glewInit();
	CHECK_F(glew_err == GLEW_OK, "Failed to initialize GLEW: %s", glewGetErrorString(glew_err));
	glGetError();  // glew sometimes produces faux GL_INVALID_ENUM
	CHECK_FOR_GL_ERROR;

	CHECK_F(glewIsSupported("GL_VERSION_3_2"));

	if (glDebugMessageCallbackARB)
	{
		LOG_F(INFO, "ARB_debug_output supported");
		glDebugMessageCallbackARB( onGLError, nullptr );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
	}
	else
	{
		LOG_F(INFO, "ARB_debug_output not supported");
	}
}
#endif // GLEW_OK

InitResult init(const Params& params)
{
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		ABORT_F("ERROR: SDL_Init: %s", SDL_GetError());
	}

#if GLLIB_GLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); // TODO: GLES 3
#else
	auto major = GLLIB_OPENGL_VERSION / 100;
	auto minor = (GLLIB_OPENGL_VERSION % 100) / 10;
	LOG_F(INFO, "Using OpenGL %d.%d", major, minor);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,  1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, params.depth_buffer);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, params.stencil_buffer);
	//SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1); // TODO?

	if (params.msa != 0) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, params.msa);
	}

	int window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

#if TARGET_OS_IPHONE
	window_flags |= SDL_WINDOW_BORDERLESS; // Hides the status bar
#else
	window_flags |= SDL_WINDOW_RESIZABLE;
#endif

	InitResult results;
	results.window = SDL_CreateWindow(params.window_name,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(int)params.width_points, (int)params.height_points, window_flags);
	CHECK_F(results.window != nullptr);

	// SDL_SysWMinfo info;
	// SDL_VERSION(&info.version);

#if TARGET_OS_IPHONE
	const auto size_points   = os::screen_size_points();
	results.pixels_per_point = os::pixels_per_point();
	results.width_points     = static_cast<size_t>(size_points.width);
	results.height_points    = static_cast<size_t>(size_points.height);
	results.width_pixels     = static_cast<size_t>(size_points.width  * results.pixels_per_point);
	results.height_pixels    = static_cast<size_t>(size_points.height * results.pixels_per_point);
#else
	int width_points, height_points;
	SDL_GetWindowSize(results.window, &width_points, &height_points);

	int width_pixels, height_pixels;
	SDL_GL_GetDrawableSize(results.window, &width_pixels, &height_pixels);

	results.width_points     = width_points;
	results.height_points    = height_points;
	results.width_pixels     = width_pixels;
	results.height_pixels    = height_pixels;
	results.pixels_per_point = (float)width_pixels / (float)width_points;
#endif

	LOG_F(INFO, "Points size: %lux%lu", results.width_points, results.height_points);
	LOG_F(INFO, "Pixel size: %lux%lu", results.width_pixels, results.height_pixels);
	LOG_F(INFO, "Pixels per point: %f", results.pixels_per_point);

	results.gl_context = SDL_GL_CreateContext(results.window);
	CHECK_F(results.gl_context != nullptr);

	SDL_GL_SetSwapInterval(1); // vsync
	//wgl_swap_intervalEXT(-1); // Try to vsync, but tear if late

	CHECK_FOR_GL_ERROR;

	#ifdef GLEW_OK
		init_glew();
	#endif

	gl::TempViewPort::set_back_buffer_size(results.width_pixels, results.height_pixels);

	return results;
}

} // namespace sdl
} // namespace emilib
