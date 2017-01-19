// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created 2014 for Ghostel.

/* Crash-course:
	for (const auto ix : irange(end)) { CHECK_F(0 <= ix && ix < end); }
	for (const auto ix : irange(begin, end)) { CHECK_F(begin <= ix && ix < end); }
	for (const auto ix : indices(some_vector)) { CHECK_F(0 <= ix && ix < some_vector.size(); }
	for (const char ch : emilib::cstr_range("hello world!"))
	for (auto& value : it_range(begin, end)) { std::cout << value; }
*/

#pragma once

#include <iterator>
#include <type_traits>

#include <loguru.hpp>

namespace emilib {

template<typename Integer>
class Range
{
public:
	static_assert(std::is_integral<Integer>::value, "Range should only be used for integers.");

	Range() : _begin(0), _end(0) { }

	Range(Integer begin, Integer end) : _begin(begin), _end(end)
	{
		DCHECK_LE_F(begin, end);
	}

	struct iterator : public std::iterator<std::input_iterator_tag, Integer>
	{
		Integer value;

		explicit iterator(Integer v) : value(v) {}

		Integer operator*() const { return value; }

		iterator& operator++()
		{
			++value;
			return *this;
		}

		friend bool operator!=(const iterator& a, const iterator& b)
		{
			return a.value != b.value;
		}
	};

	Integer operator[](size_t ix) const { return _begin + ix; }

	iterator begin() const { return iterator{ _begin }; }
	iterator end()   const { return iterator{ _end   }; }

	Integer size()  const { return _end - _begin; }

	Integer front() const { return _begin;   }
	Integer back()  const { return _end - 1; }

private:
	Integer _begin, _end;
};

/// for (const auto i : irange(end)) { CHECK_F(0 <= i && i < end); }
template<typename Integer>
Range<Integer> irange(Integer end)
{
	return {0, end};
}

/// for (const auto i : irange(begin, end)) { CHECK_F(begin <= i && i < end); }
template<typename Integer>
Range<Integer> irange(Integer begin, Integer end)
{
	return {begin, end};
}

/// for (const auto i : irange_inclusive(first, last)) { CHECK_F(first <= i && i <= last); }
template<typename Integer>
Range<Integer> irange_inclusive(Integer first, Integer last)
{
	return {first, last + 1};
}

/// for (const auto i : indices(some_vector)) { CHECK_F(0 <= i && i < some_vector.size(); }
template<typename Integer = size_t, typename Container>
Range<Integer> indices(const Container& container)
{
	return {0, static_cast<Integer>(container.size())};
}

// -----------------------------------------------------------------------

template<typename Integer, typename Visitor>
void repeat(Integer count, const Visitor& visitor)
{
	CHECK_GE_F(count, static_cast<Integer>(0));
	while (count != 0) {
		visitor();
		count -= 1;
	}
}

// -----------------------------------------------------------------------

template<typename It>
class IteratorRange
{
	It _begin, _end;
public:
	IteratorRange(It begin, It end) : _begin(begin), _end(end) { }

	struct iterator
	{
		It _it;

		auto&& operator*() const { return *_it; }
		auto&& operator->() const { return &*_it; }

		iterator& operator++()
		{
			++_it;
			return *this;
		}

		friend bool operator!=(const iterator& a, const iterator& b)
		{
			return a._it != b._it;
		}
	};

	iterator begin() const { return { _begin }; }
	iterator end()   const { return { _end   }; }

	bool   empty() const { return _begin == _end;              }
	size_t size()  const { return std::distance(_begin, _end); }
};

/// for (auto& value : it_range(begin, end)) { std::cout << value; }
template<typename It>
auto it_range(It begin, It end) { return IteratorRange<It>(begin, end); }

// -----------------------------------------------------------------------

template<typename Chr>
class CStrRange
{
	Chr* _begin;
public:
	CStrRange(const Chr* c) : _begin(c) { }

	struct iterator
	{
		Chr* _ptr;

		Chr operator*() const { return *_ptr; }

		iterator& operator++()
		{
			++_ptr;
			return *this;
		}

		friend bool operator!=(const iterator& a, const iterator& b)
		{
			return *a._ptr != *b._ptr;
		}
	};

	iterator begin() const
	{
		return { _begin };
	}
	iterator end()   const
	{
		static Chr zero = 0;
		return { &zero };
	}
};

/// for (const char ch : emilib::cstr_range("hello world!"))
template<typename Chr>
inline CStrRange<Chr> cstr_range(Chr* str) { return CStrRange<Chr>(str); }

} // namespace emilib
