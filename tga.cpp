#include <cstdint>
#include <cstdio>

#include <loguru.hpp>

namespace emilib {

bool write_tga(const char* path, size_t width, size_t height, const void* rgba_ptr, bool include_alpha)
{
	CHECK_LE_F(width,  0xFFFF);
	CHECK_LE_F(height, 0xFFFF);

	FILE* fp = fopen(path, "w");
	if (fp == nullptr) {
		LOG_F(ERROR, "Failed to open '%s' for writing", path);
		return false;
	}

	// The image header
	char header[ 18 ] = { 0 }; // char = byte
	header[ 2 ] = 2; // truecolor
	header[ 12 ] = width & 0xFF;
	header[ 13 ] = (width >> 8) & 0xFF;
	header[ 14 ] = height & 0xFF;
	header[ 15 ] = (height >> 8) & 0xFF;
	header[ 16 ] = include_alpha ? 32 : 24; // bits per pixel

	fwrite((const char*)&header, 1, sizeof(header), fp);

	// The image data is stored bottom-to-top, left-to-right
	for (int y = (int)height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {
			uint32_t rgba = *((const uint32_t*)rgba_ptr + y*width + x);
			if (include_alpha) {
				char r = (rgba & 0x000000FF);
				char g = (rgba & 0x0000FF00) >> 8;
				char b = (rgba & 0x00FF0000) >> 16;
				char a = (rgba & 0xFF000000) >> 24;
				putc((int)(b & 0xFF), fp);
				putc((int)(g & 0xFF), fp);
				putc((int)(r & 0xFF), fp);
				putc((int)(a & 0xFF), fp);
			} else {
				char r = (rgba & 0x0000FF);
				char g = (rgba & 0x00FF00) >> 8;
				char b = (rgba & 0xFF0000) >> 16;
				putc((int)(b & 0xFF), fp);
				putc((int)(g & 0xFF), fp);
				putc((int)(r & 0xFF), fp);
			}
		}
	}

	// The file footer
	static const char footer[ 26 ] =
		"\0\0\0\0" // no extension area
		"\0\0\0\0" // no developer directory
		"TRUEVISION-XFILE" // yep, this is a TGA file
		".";
	fwrite((const char*)&footer, 1, sizeof(footer), fp);

	fclose(fp);
	return true;
}

} // namespace emilib
