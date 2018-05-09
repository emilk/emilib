// By Emil Ernerfeldt 2012-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#if __APPLE__
	#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE
	#include <OpenGLES/ES2/glext.h>
#else
	// #define GLEW_NO_GLU
	#include <GL/glew.h>
#endif
