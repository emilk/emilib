// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <algorithm>

#include <loguru.hpp>

namespace emilib {

/// Sort the given std::vector and remove duplicate elements
template<typename Vector>
void stable_sort_uniq(Vector* vec)
{
	CHECK_NOTNULL_F(vec);
	std::stable_sort(std::begin(*vec), std::end(*vec));
	auto it = std::unique(std::begin(*vec), std::end(*vec));
	vec->erase(it, std::end(*vec));
}

}
