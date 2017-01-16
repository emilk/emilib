// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <vector>

namespace emilib {

/// Approximate the iso-surface at threshold = 0.
/// `iso` is assumed row-major, `width` * `height`, with positive values meaning "outside".
/// Returns a bunch of line segments as x0, y0, x1, y1.
/// In a system where (0,0) is top left, the returned line segments will be in clock-wise order.
std::vector<float> marching_squares(size_t width, size_t height, const float* iso);

/// Calculate the area of one or several shapes from their outline, as returned by marching_squares.
float calc_area(size_t num_line_segments, const float* xy);

} // namespace emilib
