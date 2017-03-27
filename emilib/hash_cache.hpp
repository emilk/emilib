// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include <functional> // std::hash

namespace emilib {

/// Wraps a value and memoizes the hash of that value.
/// This can be used to speed up std::unoredred_set/unoredred_map or emilib::HashSet/HashMap
/// when calculating the hash of the key is expensive.
/// Example: HashMap<Key, Value> -> HashMap<HashCache<Key>, Value>
/// Example: HashSet<Key, KeyHasher> -> HashSet<HashCache<Key>, HashCacheHasher<KeyHasher>>
template<typename T>
class HashCache
{
public:
	HashCache(T value) noexcept : _value(std::move(value))
	{
		using Hasher = typename std::hash<T>;
		_hash = Hasher()(_value);
	}

	HashCache(const HashCache& other) : _value(other._value)
	{
		_hash = other._hash;
	}

	const HashCache& operator=(const HashCache& other)
	{
		_hash = other._hash;
		_value = other._value;
	}

	HashCache(HashCache&& other) noexcept { other.swap(*this); }
	void operator=(HashCache&& other) noexcept { other.swap(*this); }

	void swap(HashCache& other) noexcept
	{
		std::swap(this->_value, other._value);
		std::swap(this->_hash, other._hash);
	}

	const T& value() const { return _value; }
	std::size_t hash() const { return _hash; }

	operator T() { return _value; }

	friend bool operator==(const HashCache& a, const HashCache& b)
	{
		return a._hash == b._hash && a._value == b._value;
	}

	friend bool operator!=(const HashCache& a, const HashCache& b)
	{
		return a._hash != b._hash || a._value != b._value;
	}

private:
	T           _value;
	std::size_t _hash;
};

template<typename Hasher>
struct HashCacheHasher
{
	Hasher hasher;

	template<typename T>
	size_t operator()(const HashCache<T>& x) const
	{
		return hasher(x.value());
	}
};

} // namespace emilib

namespace std {

template <typename T>
struct hash<emilib::HashCache<T>>
{
	const size_t operator()(const emilib::HashCache<T>& v) const
	{
		return v.hash();
	}
};

} // namespace std
