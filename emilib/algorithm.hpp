// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <algorithm>
#include <numeric>

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

/// Erase elements that matches the predicate without reordering existing elements
template<typename Vector, typename Predicate>
void erase_if(Vector* vec, const Predicate& predicate)
{
	const auto it = std::remove_if(std::begin(*vec), std::end(*vec), predicate);
	vec->erase(it, std::end(*vec));
}

/// Apply a function to each element and return the results.
template<typename Container, typename Kernel>
auto map(Container&& inputs, const Kernel& kernel)
    -> std::vector<decltype(kernel(inputs.front()))>
{
	std::vector<decltype(kernel(inputs.front()))> results;
	results.reserve(inputs.size());
	for (auto&& input_value : inputs) {
		results.emplace_back(kernel(input_value));
	}
	return results;
}

/// Flatten multiple containers into one vector (NOT recursively).
template<typename Container>
auto flatten(std::initializer_list<Container> containers)
{
	std::vector<typename Container::value_type> result;
	for (const auto& container : containers) {
		result.insert(end(result), begin(container), end(container));
	}
	return result;
}

/// Sort the given range so elements with lower key(element) comes first.
template<typename Vector, typename Key>
void stable_sort_by_key(Vector* vec, const Key& key)
{
	CHECK_NOTNULL_F(vec);
	std::stable_sort(std::begin(*vec), std::end(*vec), [&key](const auto& a, const auto& b)
	{
		return key(a) < key(b);
	});
}

template<typename Vector>
bool all_same(const Vector& v)
{
	CHECK_F(!v.empty());
	for (size_t i = 1; i < v.size(); ++i) {
		if (v[i] != v[0]) {
			return false;
		}
	}
	return true;
}

template<typename T> struct Accumulator        { using type = T;      };
template<>           struct Accumulator<float> { using type = double; };

template<typename Vector>
auto sum(const Vector& v)
{
	using namespace std;
	using value_type = typename Vector::value_type;
	using accumulator = typename Accumulator<value_type>::type;
	auto sum = std::accumulate(begin(v), end(v), accumulator());
	return static_cast<value_type>(sum);
}

template<typename Container>
auto max(const Container& v)
{
	CHECK_F(!v.empty());
	return *std::max_element(v.begin(), v.end());
}

} // namespace emilib
