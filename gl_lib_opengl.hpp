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
