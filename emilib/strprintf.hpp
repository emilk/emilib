// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel
//   Moved into emilib in 2016

#pragma once

#include <string>

namespace emilib {

// strprintf acts like printf, but writes to a std::string.

// Try to catch format string errors at compile time with compiler-specific extensions:
#ifdef _MSC_VER
	std::string strprintfv(_In_z_ _Printf_format_string_ const char* format, va_list);
	std::string strprintf(_In_z_ _Printf_format_string_ const char* format, ...);
#elif defined(__clang__) || defined(__GNUC__)
	std::string strprintfv(const char* format, va_list) __attribute__((format(printf, 1, 0)));
	std::string strprintf(const char* format, ...) __attribute__((format(printf, 1, 2)));
#else
	std::string strprintfv(const char* format, va_list);
	std::string strprintf(const char* format, ...);
#endif

} // namespace emilib
