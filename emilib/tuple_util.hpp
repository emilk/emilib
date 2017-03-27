// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel.

#pragma once

#include <tuple>

namespace emilib  {

template<int N>
class Int_ {};

namespace detail
{
	// just to keep things concise and readable
#define ENABLE_IF(x) typename std::enable_if<(x), bool>::type

	// base case
	template <std::size_t N, typename... Args, typename Iterator>
	ENABLE_IF(N == 0) iterate(std::tuple<Args...>&, Iterator&&)
	{
		return true;
	}

	// recursive case
	template <std::size_t N, typename... Args, typename Iterator>
	ENABLE_IF(N >= 1) iterate(std::tuple<Args...>& tup, Iterator&& it)
	{
		if (iterate<N - 1>(tup, it)) {
			auto&& val = std::get<N-1>(tup);
			return it( Int_<N-1>(), val );
		} else {
			return false;
		}
	}

	// const version:

	// base case
	template <std::size_t N, typename... Args, typename Iterator>
	ENABLE_IF(N == 0) iterate(const std::tuple<Args...>&, Iterator&)
	{
		return true;
	}

	// recursive case
	template <std::size_t N, typename... Args, typename Iterator>
	ENABLE_IF(N >= 1) iterate(const std::tuple<Args...>& tup, Iterator& it)
	{
		if (iterate<N - 1>(tup, it)) {
			auto&& val = std::get<N-1>(tup);
			return it( Int_<N-1>(), val );
		} else {
			return false;
		}
	}
}

/// 'Func' must have a method:
///
/// template<int N, class T>
/// bool operator()(Int_<N>, T)
///
/// Return 'true' to continue, 'false' to break.
///
/// for_each_tuple return 'false' iff any call to 'func()' returned false.
template<typename Func, typename... Args>
bool for_each_tuple(std::tuple<Args...>& tup, Func&& func)
{
	return detail::iterate<sizeof...(Args)>(tup, func);
}

/// const version
template<typename Func, typename... Args>
bool for_each_tuple(const std::tuple<Args...>& tup, Func&& func)
{
	return detail::iterate<sizeof...(Args)>(tup, func);
}

// ------------------------------------------------

template<std::size_t N>
struct TupleArrayRef {
	template<class Tuple>
	static auto ref_to_members(Tuple&& tup, int ix)
	{
		return std::tuple_cat(TupleArrayRef<N-1>::ref_to_members(tup, ix),
		                      std::tie( std::get<N-1>(tup)[ix] ) );
	}

};

template<>
struct TupleArrayRef<0> {
	template<class Tuple>
	static std::tuple<> ref_to_members(Tuple&&, int)
	{
		// done
		return std::tuple<>();
	}
};

// ------------------------------------------------

} // namespace emilib

namespace std {
namespace {

	/// Code from boost
	/// Reciprocal of the golden ratio helps spread entropy and handles duplicates.
	/// See Mike Seymour in magic-numbers-in-boosthash-combine:
	///     http://stackoverflow.com/questions/4948780
	template <class T>
	inline void hash_combine(std::size_t& seed, T const& v)
	{
		seed ^= hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}

	/// Recursive template code derived from Matthieu M.
	template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
	struct HashValueImpl
	{
		static void apply(size_t& seed, Tuple const& tuple)
		{
			HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
			hash_combine(seed, get<Index>(tuple));
		}
	};

	template <class Tuple>
	struct HashValueImpl<Tuple,0>
	{
		static void apply(size_t& seed, Tuple const& tuple)
		{
			hash_combine(seed, get<0>(tuple));
		}
	};
} // anonymous namespace

/// Hashing of tuples.
/// From http://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
template <typename ... TT>
struct hash<std::tuple<TT...>>
{
	size_t operator()(std::tuple<TT...> const& tt) const
	{
		size_t seed = 0;
		HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
		return seed;
	}

};

} // namespace std
