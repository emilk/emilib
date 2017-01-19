// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel.

#pragma once

#include <string>

namespace emilib {
namespace utf8 {

size_t count_chars(const char* utf8);

/// Count how many whole codepoints there are in the first num_bytes <= strlen(utf8)
size_t count_chars(const char* utf8, size_t num_bytes);

/// Returns the number of characters outputted, or 0 on error
size_t encode(std::string& out_utf8, uint64_t codepoints);

} // namespace utf8
} // namespace emilib
