// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "strprintf.hpp"

#include <cassert>

namespace emilib {

static std::string vstrprintf(const char* format, va_list vlist)
{
#ifdef _MSC_VER
	int bytes_needed = vsnprintf(nullptr, 0, format, vlist);
	assert(bytes_needed >= 0);
	std::string str;
	str.resize(bytes_needed + 1);
	vsnprintf(&str[0], bytes_needed, format, vlist);
	str.resize(bytes_needed); // Remove final '\0'
	return str;
#else
	char* buff = nullptr;
	int result = vasprintf(&buff, format, vlist);
	assert(result >= 0);
	(void)result;
	std::string str(buff);
	free(buff);
	return str;
#endif
}

std::string strprintf(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	auto result = vstrprintf(format, vlist);
	va_end(vlist);
	return result;
}

} // namespace emilib
