// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel

#pragma once

#include <emilib/gl_lib_fwd.hpp>

namespace emilib {
namespace gl {

/// iOS only!
gl::Texture load_pvr(const char* path, gl::TexParams params);

} // namespace gl
} // namespace emilib
