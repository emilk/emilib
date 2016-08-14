// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "utf8.hpp"

#include <loguru.hpp>

namespace emilib {
namespace utf8 {

size_t count_chars(const char* utf8)
{
	size_t count = 0;
	while (*utf8) {
		if ((*utf8 & 0b11000000) != 0b10000000) {
			count += 1;
		}
		utf8++;
	}
	return count;
}

size_t count_chars(const char* utf8, size_t num_bytes)
{
	size_t count = 0;
	for (size_t i = 0; i < num_bytes; ++i) {
		CHECK_F(*utf8 != '\0', "Premature end of string");
		if ((*utf8 & 0b11000000) != 0b10000000) {
			count += 1;
		}
		utf8++;
	}
	return count;
}

size_t encode(std::string& dst, uint64_t c)
{
	if (c <= 0x7F) {
		// 0XXX XXXX - one byte
		dst += static_cast<char>(c);
		return 1;
	}
	else if (c <= 0x7FF) {
		// 110X XXXX - two bytes
		dst += static_cast<char>(0xC0 | (c >> 6));
		dst += static_cast<char>(0x80 | (c & 0x3F));
		return 2;
	}
	else if (c <= 0xFFFF) {
		// 1110 XXXX - three bytes
		dst += static_cast<char>(0xE0 | (c >> 12));
		dst += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
		dst += static_cast<char>(0x80 | (c & 0x3F));
		return 3;
	}
	else if (c <= 0x1FFFFF) {
		// 1111 0XXX - four bytes
		dst += static_cast<char>(0xF0 | (c >> 18));
		dst += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
		dst += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
		dst += static_cast<char>(0x80 | (c & 0x3F));
		return 4;
	}
	else if (c <= 0x3FFFFFF) {
		// 1111 10XX - five bytes
		dst += static_cast<char>(0xF8 | (c >> 24));
		dst += static_cast<char>(0x80 | (c >> 18));
		dst += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
		dst += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
		dst += static_cast<char>(0x80 | (c & 0x3F));
		return 5;
	}
	else if (c <= 0x7FFFFFFF) {
		// 1111 110X - six bytes
		dst += static_cast<char>(0xFC | (c >> 30));
		dst += static_cast<char>(0x80 | ((c >> 24) & 0x3F));
		dst += static_cast<char>(0x80 | ((c >> 18) & 0x3F));
		dst += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
		dst += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
		dst += static_cast<char>(0x80 | (c & 0x3F));
		return 6;
	}
	else {
		return 0; // Error
	}
}

} // namespace utf8
} // namespace emilib
