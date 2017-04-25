// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <vector>
#include <utility>

namespace emilib {

/// like std::equal_to but no need to #include <functional>
template<typename T>
struct ListMapEqualTo
{
	constexpr bool operator()(const T& lhs, const T& rhs) const
	{
		return lhs == rhs;
	}
};

/// Linear lookup map for quick lookups among few values.
template<typename KeyT, typename ValueT, typename EqT = ListMapEqualTo<KeyT>>
class ListMap
{
public:
	using Pair = std::pair<KeyT, ValueT>;
	using List = std::vector<Pair>;

	using size_type       = size_t;
	using value_type      = Pair;
	using reference       = Pair&;
	using const_reference = const Pair&;
	using iterator        = typename List::iterator;
	using const_iterator  = typename List::const_iterator;

	iterator begin() { return _list.begin(); }
	iterator end() { return _list.end(); }
	const_iterator begin() const { return _list.begin(); }
	const_iterator end() const { return _list.end(); }

	size_t size() const { return _list.size(); }
	bool empty() const { return _list.empty(); }

	iterator find(const KeyT& key)
	{
		iterator e=end();
		for (iterator it=begin(); it!=e; ++it) {
			if (_eq(it->first, key)) {
				return it;
			}
		}
		return e;
	}

	const_iterator find(const KeyT& key) const
	{
		const_iterator e=end();
		for (const_iterator it=begin(); it!=e; ++it) {
			if (_eq(it->first, key)) {
				return it;
			}
		}
		return e;
	}

	size_t count(const KeyT& key) const
	{
		return find(key) == end() ? 0 : 1;
	}

	ValueT& operator[](const KeyT& key)
	{
		iterator e=end();
		for (iterator it=begin(); it!=e; ++it) {
			if (_eq(it->first, key)) {
				return it->second;
			}
		}
		_list.push_back(Pair(key, ValueT()));
		return _list.back().second;
	}

	const ValueT& at(const KeyT& key) const
	{
		auto it = find(key);
		if (it == end()) { throw std::domain_error("No such key in ListMap"); }
		return it->second;
	}

	bool insert(const Pair& p)
	{
		const_iterator e=end();
		for (const_iterator it=begin(); it!=e; ++it) {
			if (_eq(it->first, p.first)) {
				return false; // like std::map we do not insert if we already have it
			}
		}
		_list.push_back(p);
		return true;
	}

	void insert_or_assign(const KeyT& key, ValueT&& value)
	{
		const_iterator e=end();
		for (const_iterator it=begin(); it!=e; ++it) {
			if (_eq(it->first, key)) {
				it->second = std::move(value);
				return;
			}
		}
		_list.push_back(std::make_pair(key, std::move(value)));
	}

	iterator erase(iterator it)
	{
		*it = _list.back();
		_list.pop_back();
		return it;
	}

	void erase(const KeyT& key)
	{
		iterator e=end();
		for (iterator it=begin(); it!=e; ++it) {
			if (_eq(it->first, key)) {
				erase(it);
				return;
			}
		}
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
