// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace emilib {

using CalcWidth = std::function<float(const std::string& text)>;

/// Split text into lines, no longer than max_width each.
/// The algorithm tries to keep lines equal length:
///
/// BAD:
/// 	This is a long line that should
/// 	wrap.
///
/// GOOD:
/// 	This is a long line
/// 	that should wrap.
///
/// This feature makes this algorithm suitable for centered text.
///
/// This function only breaks on spaces. It is not yet fully-featured.
std::vector<std::string> word_wrap(
	const std::string& text,
	float              max_width,
	const CalcWidth&   calc_width);

void unit_test_word_wrap();

} // namespace emilib
