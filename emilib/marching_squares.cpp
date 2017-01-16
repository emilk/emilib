// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "marching_squares.hpp"

namespace emilib {

std::vector<float> marching_squares(size_t width, size_t height, const float* iso)
{
	std::vector<float> lines;

	for (size_t y = 0; y < height - 1; ++y) {
		for (size_t x = 0; x < width - 1; ++x) {
			const float tl_f = iso[x + 0 + width * (y + 0)];
			const float tr_f = iso[x + 1 + width * (y + 0)];
			const float bl_f = iso[x + 0 + width * (y + 1)];
			const float br_f = iso[x + 1 + width * (y + 1)];
			const int tl_i = tl_f >= 0.0f;
			const int tr_i = tr_f >= 0.0f;
			const int bl_i = bl_f >= 0.0f;
			const int br_i = br_f >= 0.0f;

			int config = (br_i << 3) | (bl_i << 2) | (tr_i << 1) | tl_i;

			if (config == 0b0000 || config == 0b1111) { continue; }

			const float y_left   = tl_f / (tl_f - bl_f);
			const float y_right  = tr_f / (tr_f - br_f);
			const float x_bottom = bl_f / (bl_f - br_f);
			const float x_top    = tl_f / (tl_f - tr_f);

			switch (config) {
				case 0b0001:
					lines.push_back(x + 0.0f);   // x
					lines.push_back(y + y_left); // y
					lines.push_back(x + x_top);  // x
					lines.push_back(y + 0.0f);   // y
					break;
				case 0b1110:
					lines.push_back(x + x_top);  // x
					lines.push_back(y + 0.0f);   // y
					lines.push_back(x + 0.0f);   // x
					lines.push_back(y + y_left); // y
					break;

				case 0b0010:
					lines.push_back(x + x_top);   // x
					lines.push_back(y + 0.0f);    // y
					lines.push_back(x + 1.0f);    // x
					lines.push_back(y + y_right); // y
					break;
				case 0b1101:
					lines.push_back(x + 1.0f);    // x
					lines.push_back(y + y_right); // y
					lines.push_back(x + x_top);   // x
					lines.push_back(y + 0.0f);    // y
					break;

				case 0b0011:
					lines.push_back(x + 0.0f);    // x
					lines.push_back(y + y_left);  // y
					lines.push_back(x + 1.0f);    // x
					lines.push_back(y + y_right); // y
					break;
				case 0b1100:
					lines.push_back(x + 1.0f);    // x
					lines.push_back(y + y_right); // y
					lines.push_back(x + 0.0f);    // x
					lines.push_back(y + y_left);  // y
					break;

				case 0b0100:
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					lines.push_back(x + 0.0f);     // x
					lines.push_back(y + y_left);   // y
					break;
				case 0b1011:
					lines.push_back(x + 0.0f);     // x
					lines.push_back(y + y_left);   // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					break;

				case 0b0101:
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					lines.push_back(x + x_top);    // x
					lines.push_back(y + 0.0f);     // y
					break;
				case 0b1010:
					lines.push_back(x + x_top);    // x
					lines.push_back(y + 0.0f);     // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					break;

				case 0b0110:
					lines.push_back(x + x_top);    // x
					lines.push_back(y + 0.0f);     // y
					lines.push_back(x + 0.0f);     // x
					lines.push_back(y + y_left);   // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					lines.push_back(x + 1.0f);     // x
					lines.push_back(y + y_right);  // y
					break;
				case 0b1001:
					lines.push_back(x + 0.0f);     // x
					lines.push_back(y + y_left);   // y
					lines.push_back(x + x_top);    // x
					lines.push_back(y + 0.0f);     // y
					lines.push_back(x + 1.0f);     // x
					lines.push_back(y + y_right);  // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					break;

				case 0b0111:
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					lines.push_back(x + 1.0f);     // x
					lines.push_back(y + y_right);  // y
					break;
				case 0b1000:
					lines.push_back(x + 1.0f);     // x
					lines.push_back(y + y_right);  // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					break;
			}
		}
	}

	return lines;
}

float calc_area(size_t num_line_segments, const float* xy)
{
	double area = 0;

	for (size_t i = 0; i < num_line_segments; ++i) {
		const double p0x = xy[4 * i + 0];
		const double p0y = xy[4 * i + 1];
		const double p1x = xy[4 * i + 2];
		const double p1y = xy[4 * i + 3];
		area += p0x * p1y - p1x * p0y;
	}

	return static_cast<float>(area / 2);
}

} // namespace emilib
