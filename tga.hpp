#pragma once

namespace emilib {

// Dump image to disk as .tga. Image should be stored row by row, top-left to bottom-right.
// rgba_ptr should point to width * height * 4 bytes.
// If include_alpha is false, the alpha channel will be ommited.
bool write_tga(const char* path, size_t width, size_t height, const void* rgba_ptr, bool include_alpha);

} // namespace emilib
