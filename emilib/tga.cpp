// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include <cstdint>
#include <cstdio>
#include <vector>

#include <loguru.hpp>

namespace emilib {

std::vector<uint8_t> encode_tga(size_t width, size_t height, const void* rgba_ptr, bool include_alpha)
{
	CHECK_LE_F(width,  0xFFFF);
	CHECK_LE_F(height, 0xFFFF);

	const size_t TGA_HEADER_SIZE = 18;
	const size_t TGA_FOOTER_SIZE = 26;
	const size_t data_size       = width * height * (include_alpha ? 4 : 3);

	std::vector<uint8_t> data(TGA_HEADER_SIZE + data_size + TGA_FOOTER_SIZE, 0);

	// Write header:
	data[  2 ] = 2; // truecolor
	data[ 12 ] = width & 0xFF;
	data[ 13 ] = (width >> 8) & 0xFF;
	data[ 14 ] = height & 0xFF;
	data[ 15 ] = (height >> 8) & 0xFF;
	data[ 16 ] = include_alpha ? 32 : 24; // bits per pixel

	size_t index = TGA_HEADER_SIZE;

	// The image data is stored bottom-to-top, left-to-right
	for (int y = (int)height - 1; y >= 0; y--) {
		for (size_t x = 0; x < width; x++) {
			uint32_t rgba = *((const uint32_t*)rgba_ptr + y*width + x);
			if (include_alpha) {
				uint8_t r = (rgba & 0x000000FF);
				uint8_t g = (rgba & 0x0000FF00) >> 8;
				uint8_t b = (rgba & 0x00FF0000) >> 16;
				uint8_t a = (rgba & 0xFF000000) >> 24;
				data[index++] = b & 0xFF;
				data[index++] = g & 0xFF;
				data[index++] = r & 0xFF;
				data[index++] = a & 0xFF;
			} else {
				uint8_t r = (rgba & 0x0000FF);
				uint8_t g = (rgba & 0x00FF00) >> 8;
				uint8_t b = (rgba & 0xFF0000) >> 16;
				data[index++] = b & 0xFF;
				data[index++] = g & 0xFF;
				data[index++] = r & 0xFF;
			}
		}
	}

	CHECK_EQ_F(index, TGA_HEADER_SIZE + data_size);

	memcpy(&data[index], "\0\0\0\0\0\0\0\0", 8); // no extension area + no developer directory
	index += 8;

	memcpy(&data[index], "TRUEVISION-XFILE.", strlen("TRUEVISION-XFILE." + 1));
	index += strlen("TRUEVISION-XFILE.") + 1;

	CHECK_EQ_F(index, data.size());

	return data;
}

bool write_tga(const char* path, size_t width, size_t height, const void* rgba_ptr, bool include_alpha)
{
	FILE* fp = fopen(path, "w");
	if (fp == nullptr) {
		LOG_F(ERROR, "Failed to open '%s' for writing", path);
		return false;
	}

	const auto data = encode_tga(width, height, rgba_ptr, include_alpha);

	fwrite(data.data(), 1, data.size(), fp);

	fclose(fp);
	return true;
}

} // namespace emilib
