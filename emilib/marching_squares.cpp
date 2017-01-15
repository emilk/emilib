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

			int config = tl_i | (tr_i << 1) | (bl_i << 2) | (br_i << 3);
			if (config > 7) { config = 15 - config; }

			if (config == 0) { continue; }

			const float y_left   = tl_f / (tl_f - bl_f);
			const float y_right  = tr_f / (tr_f - br_f);
			const float x_bottom = 1.0f - br_f / (br_f - bl_f);
			const float x_top    = 1.0f - tr_f / (tr_f - tl_f);

			switch (config) {
				case 1:
					lines.push_back(x + 0.0f);   // x
					lines.push_back(y + y_left); // y
					lines.push_back(x + x_top);  // x
					lines.push_back(y + 0.0f);   // y
					break;
				case 2:
					lines.push_back(x + x_top);   // x
					lines.push_back(y + 0.0f);    // y
					lines.push_back(x + 1.0f);    // x
					lines.push_back(y + y_right); // y
					break;
				case 3:
					lines.push_back(x + 0.0f);    // x
					lines.push_back(y + y_left);  // y
					lines.push_back(x + 1.0f);    // x
					lines.push_back(y + y_right); // y
					break;
				case 4:
					lines.push_back(x + 0.0f);     // x
					lines.push_back(y + y_left);   // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					break;
				case 5:
					lines.push_back(x + x_top);    // x
					lines.push_back(y + 0.0f);     // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					break;
				case 6:
					lines.push_back(x + x_top);    // x
					lines.push_back(y + 0.0f);     // y
					lines.push_back(x + 0.0f);     // x
					lines.push_back(y + y_left);   // y
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					lines.push_back(x + 1.0f);     // x
					lines.push_back(y + y_right);  // y
					break;
				case 7:
					lines.push_back(x + x_bottom); // x
					lines.push_back(y + 1.0f);     // y
					lines.push_back(x + 1.0f);     // x
					lines.push_back(y + y_right);  // y
					break;
			}
		}
	}

	return lines;
}

} // namespace emilib
