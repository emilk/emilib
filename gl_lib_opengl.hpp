// By Emil Ernerfeldt 2012-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include "gl_lib_fwd.hpp"

// ------------------------------------------------

#define GLEW_NO_GLU

// ------------------------------------------------
#if TARGET_OS_IPHONE
	#include <OpenGLES/ES2/glext.h>
// ------------------------------------------------
#elif TARGET_OS_MAC
	#include <GL/glew.h>

	//#ifndef __OBJC__
	#if 0
		#include <GL/glxew.h>
		#undef None
	#endif

	#if GLLIB_OPENGL_VERSION < 300
		#include <OpenGL/gl.h>
	#else
		#include <OpenGL/gl3.h>
	#endif
// ------------------------------------------------
#else // Windows?
	#include <GL/glew.h>
// ------------------------------------------------

#endif
