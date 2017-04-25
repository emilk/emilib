// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <vector>

namespace emilib {

/// like std::equal_to but no need to #include <functional>
template<typename T>
struct ListSetEqualTo
{
	constexpr bool operator()(const T& lhs, const T& rhs) const
	{
		return lhs == rhs;
	}
};

/// Linear lookup set for quick lookups among few values.
template<typename KeyT, typename EqT = ListSetEqualTo<KeyT>>
class ListSet
{
public:
	using List = std::vector<KeyT>;

	using size_type       = size_t;
	using value_type      = KeyT;
	using reference       = KeyT&;
	using const_reference = const    KeyT&;
	using iterator        = typename List::iterator;
	using const_iterator  = typename List::const_iterator;

	iterator begin() { return _list.begin(); }
	iterator end() { return _list.end(); }
	const_iterator begin() const { return _list.begin(); }
	const_iterator end() const { return _list.end(); }

	size_t size() const { return _list.size(); }
	bool empty() const { return _list.empty(); }

	int count(const KeyT& key) const
	{
		for (auto&& k : *this) {
			if (_eq(k, key)) {
				return 1;
			}
		}
		return 0;
	}

	bool insert(const KeyT& key)
	{
		for (auto&& k : *this) {
			if (_eq(k, key)) {
				return false; // like std::set we do not insert if we already have it
			}
		}
		_list.push_back(key);
		return true;
	}

	/// Frees unnecessary memory
	void shrink_to_fit()
	{
		_list.shrink_to_fit();
	}

	void clear() { _list.clear(); }

private:
	List _list;
	EqT  _eq;
};

} // namespace emilib
