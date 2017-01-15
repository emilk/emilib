// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <vector>

namespace emilib {

/// Approximate the iso-surface at threshold = 0.
/// `iso` is assumed row-major, `width` * `height`.
/// Returns a bunch of line segments as x0, y0, x1, y1.
/// Not optimized. Line segments may not align seamlessly.
std::vector<float> marching_squares(size_t width, size_t height, const float* iso);

} // namespace emilib
