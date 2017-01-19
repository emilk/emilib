// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cstdint>
#include <vector>

namespace emilib {

/// Dump image to disk as .tga. Image should be stored row by row, top-left to bottom-right.
/// rgba_ptr should point to width * height * 4 bytes.
/// If include_alpha is false, the alpha channel will be omitted.
bool write_tga(const char* path, size_t width, size_t height, const void* rgba_ptr, bool include_alpha);

/// As above, but returns the tga in memory.
std::vector<uint8_t> encode_tga(size_t width, size_t height, const void* rgba_ptr, bool include_alpha);

} // namespace emilib
