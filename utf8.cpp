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
	CHECK_LE_F(num_bytes, strlen(utf8));
	size_t count = 0;
	for (size_t i=0; i<num_bytes; ++i) {
		if ((*utf8 & 0b11000000) != 0b10000000) {
			count += 1;
		}
		utf8++;
	}
	return count;
}

// Returns the number of bytes written, or 0 on error
size_t encode(std::string& dst, uint64_t c)
{
	if (c <= 0x7F)  // 0XXX XXXX - one byte
	{
		dst += (char) c;
		return 1;
	}
	else if (c <= 0x7FF)  // 110X XXXX - two bytes
	{
		dst += (char) ( 0xC0 | (c >> 6) );
		dst += (char) ( 0x80 | (c & 0x3F) );
		return 2;
	}
	else if (c <= 0xFFFF)  // 1110 XXXX - three bytes
	{
		dst += (char) (0xE0 | (c >> 12));
		dst += (char) (0x80 | ((c >> 6) & 0x3F));
		dst += (char) (0x80 | (c & 0x3F));
		return 3;
	}
	else if (c <= 0x1FFFFF)  // 1111 0XXX - four bytes
	{
		dst += (char) (0xF0 | (c >> 18));
		dst += (char) (0x80 | ((c >> 12) & 0x3F));
		dst += (char) (0x80 | ((c >> 6) & 0x3F));
		dst += (char) (0x80 | (c & 0x3F));
		return 4;
	}
	else if (c <= 0x3FFFFFF)  // 1111 10XX - five bytes
	{
		dst += (char) (0xF8 | (c >> 24));
		dst += (char) (0x80 | (c >> 18));
		dst += (char) (0x80 | ((c >> 12) & 0x3F));
		dst += (char) (0x80 | ((c >> 6) & 0x3F));
		dst += (char) (0x80 | (c & 0x3F));
		return 5;
	}
	else if (c <= 0x7FFFFFFF)  // 1111 110X - six bytes
	{
		dst += (char) (0xFC | (c >> 30));
		dst += (char) (0x80 | ((c >> 24) & 0x3F));
		dst += (char) (0x80 | ((c >> 18) & 0x3F));
		dst += (char) (0x80 | ((c >> 12) & 0x3F));
		dst += (char) (0x80 | ((c >> 6) & 0x3F));
		dst += (char) (0x80 | (c & 0x3F));
		return 6;
	}
	else {
		return 0; // Error
	}
}

} // namespace utf8
} // namespace emilib
