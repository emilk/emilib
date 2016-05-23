#pragma once

#include <emilib/gl_lib_fwd.hpp>

namespace gl {

// iOS only!
gl::Texture load_pvr(const char* path, gl::TexParams params);

} // namespace gl
