// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

/*
	A library for dual numbers.

	Dual numbers can be used to get numerical stable differentiation with minimal effort:
		auto result = f(Dual<float>(x, 1));
		result.real == f(x)
		result.eps == d f(x) / dx

	If you want to use dual numbers in Eigen matrices then:
		#define DUALS_EGIEN 1
*/
#pragma once

#include <cmath>
#include <iosfwd>
#include <limits>
#include <utility>

namespace emilib {

template<typename T>
class Dual
{
public:
	inline constexpr Dual() : real(T()) , eps(T()) { }
	inline constexpr explicit Dual(const T& real_) : real(real_) , eps(T()) { }
	inline constexpr Dual(const T& real_, const T& eps_) : real(real_) , eps(eps_) { }

	T real;
	T eps;
};

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr Dual<T> operator+(const Dual<T>& left, const Dual<T>& right)
{
	return {left.real + right.real, left.eps + right.eps};
}

template<typename T>
inline constexpr Dual<T> operator+(const Dual<T>& left, const T& right)
{
	return {left.real + right, left.eps};
}

template<typename T>
inline constexpr Dual<T> operator+(const T& left, const Dual<T>& right)
{
	return {left + right.real, right.eps};
}

template<typename T>
inline constexpr void operator+=(Dual<T>& left, const Dual<T>& right)
{
	left = left + right;
}

template<typename T>
inline constexpr void operator+=(Dual<T>& left, const T& right)
{
	left.real += right;
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr Dual<T> operator-(const Dual<T>& left, const Dual<T>& right)
{
	return {left.real - right.real, left.eps - right.eps};
}

template<typename T>
inline constexpr Dual<T> operator-(const Dual<T>& left, const T& right)
{
	return {left.real - right, left.eps};
}

template<typename T>
inline constexpr Dual<T> operator-(const T& left, const Dual<T>& right)
{
	return {left - right.real, right.eps};
}

template<typename T>
inline constexpr void operator-=(Dual<T>& left, const Dual<T>& right)
{
	left = left - right;
}

template<typename T>
inline constexpr void operator-=(Dual<T>& left, const T& right)
{
	left.real -= right;
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr Dual<T> operator*(const T& left, const Dual<T>& right)
{
	return {left * right.real, left * right.eps};
}

template<typename T>
inline constexpr Dual<T> operator*(const Dual<T>& left, const T& right)
{
	return {left.real * right, left.eps * right};
}

template<typename T>
inline constexpr void operator*=(Dual<T>& left, const T& right)
{
	left = left * right;
}

template<typename T>
inline constexpr Dual<T> operator*(const Dual<T>& left, const Dual<T>& right)
{
	return {left.real * right.real, left.real * right.eps + right.real * left.eps};
}

template<typename T>
inline constexpr void operator*=(Dual<T>& left, const Dual<T>& right)
{
	left = left * right;
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr Dual<T> operator/(const Dual<T>& left, const T& right)
{
	return {left.real / right, left.eps / right};
}

template<typename T>
inline constexpr void operator/=(Dual<T>& left, const T& right)
{
	left = left / right;
}

template<typename T>
inline constexpr Dual<T> operator/(const Dual<T>& left, const Dual<T>& right)
{
	if (right.real == T()) {
		if (right.eps == T()) {
			// Anything divided by zero:
			return {std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
		} else if (left.real == T()) {
			// eps divided by eps:
			return Dual<T>{left.eps / right.eps, std::numeric_limits<T>::quiet_NaN()};
			// return Dual<T>{left.eps / right.eps, left.eps / right.eps}; // Assumes the eps is repeated as a hyper-eps
			// return Dual<T>(left.eps - right.eps); // e.g. for x/x this makes sense for filling in a singularity.
			// return Dual<T>{std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()}; // We shouldn't rely on dividing by 0 + eps
		} else {
			// real divided by eps:
			const T sign = left.real * right.eps;
			const T signed_inf = sign * std::numeric_limits<T>::infinity();
			return Dual<T>(signed_inf, signed_inf);
		}
	} else {
		// real divided by real:
		return Dual<T>{
			left.real / right.real,
			(left.eps * right.real - left.real * right.eps) / (right.real * right.real)
		};
	}
}

template<typename T>
inline constexpr void operator/=(Dual<T>& left, const Dual<T>& right)
{
	left = left / right;
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr Dual<T> operator+(const Dual<T>& operand)
{
	return {+operand.real, +operand.eps};
}

template<typename T>
inline constexpr Dual<T> operator-(const Dual<T>& operand)
{
	return {-operand.real, -operand.eps};
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr bool operator==(const Dual<T>& left, const Dual<T>& right)
{
	return left.real == right.real && left.eps == right.eps;
}

template<typename T>
inline constexpr bool operator!=(const Dual<T>& left, const Dual<T>& right)
{
	return left.real != right.real || left.eps != right.eps;
}

template<typename T>
inline constexpr bool operator<(const Dual<T>& left, const Dual<T>& right)
{
	return std::make_pair(left.real, left.eps) < std::make_pair(right.real, right.eps);
}

template<typename T>
inline constexpr bool operator<=(const Dual<T>& left, const Dual<T>& right)
{
	return std::make_pair(left.real, left.eps) <= std::make_pair(right.real, right.eps);
}

template<typename T>
inline constexpr bool operator>=(const Dual<T>& left, const Dual<T>& right)
{
	return std::make_pair(left.real, left.eps) >= std::make_pair(right.real, right.eps);
}

template<typename T>
inline constexpr bool operator>(const Dual<T>& left, const Dual<T>& right)
{
	return std::make_pair(left.real, left.eps) > std::make_pair(right.real, right.eps);
}

// ----------------------------------------------------------------------------

template<typename T>
std::ostream& operator<<(std::ostream& os, const Dual<T>& x)
{
	if      (x.eps  == T()) { return os << x.real; }
	else if (x.real == T()) { return os << x.eps << "ε"; }
	else                    { return os << x.real << (x.eps < 0 ? '-' : '+') << std::abs(x.eps) << "ε"; }
}

} // namespace emilib

// ----------------------------------------------------------------------------

namespace std {

template<typename T>
inline constexpr emilib::Dual<T> abs(const emilib::Dual<T>& x)
{
	if (x.real < 0) { return { -x.real, -x.eps }; }
	if (x.real > 0) { return { x.real,  x.eps  }; }
	return {0, abs(x.eps)};
}

template<typename T>
emilib::Dual<T> sin(const emilib::Dual<T>& x)
{
	return {sin(x.real), x.eps * cos(x.real)};
}

template<typename T>
emilib::Dual<T> cos(const emilib::Dual<T>& x)
{
	return {cos(x.real), -x.eps * sin(x.real)};
}

template <typename T>
emilib::Dual<T> log(const emilib::Dual<T>& x)
{
	return {log(x.real), x.eps / x.real};
}

template <typename T>
emilib::Dual<T> log10(const emilib::Dual<T>& x)
{
	return {log10(x.real), x.eps / (log(T(10)) * x.real)};
}

template <typename T>
emilib::Dual<T> sqrt(const emilib::Dual<T>& x)
{
	return {
		sqrt(x.real),
		x.eps / (T(2) * sqrt(x.real))
	};
}

template <typename T, typename S>
emilib::Dual<T> pow(const emilib::Dual<T>& left, const S& right)
{
	return {
		pow(left.real, right),
		left.eps * T(right) * pow(left.real, right - S(1))
	};
}

template <typename T>
emilib::Dual<T> pow(const T& left, const emilib::Dual<T>& right)
{
	return {
		pow(left, right.real),
		right.eps * log(left) * pow(left, right.real)
	};
}

template <typename T>
emilib::Dual<T> pow(const emilib::Dual<T>& left, const emilib::Dual<T>& right)
{
	return {
		pow(left.real, right.real),
		left.eps * right.real * pow(left.real, right.real - T(1)) +
		right.eps * log(left.real) * pow(left.real, right.real)
	};
}

template <typename T>
bool isfinite(const emilib::Dual<T>& left)
{
	return isfinite(left.real);
}

template <typename T>
T ceil(const emilib::Dual<T>& x)
{
	return ceil(x.real);
}

} // namespace std

// ----------------------------------------------------------------------------

// Define DUALS_EGIEN to have you Dual:s work inside of Eigen matrices.
#if DUALS_EGIEN
	#include <Eigen/Geometry>

	namespace Eigen {

	template <typename T>
	struct NumTraits<emilib::Dual<T>> : GenericNumTraits<emilib::Dual<T>>
	{
		using Real = T;
		using Literal = typename NumTraits<T>::Literal;
		enum {
			IsComplex             = 1,
			RequireInitialization = NumTraits<Real>::RequireInitialization,
			ReadCost              = 2 * NumTraits<Real>::ReadCost,
			AddCost               = 2 * NumTraits<Real>::AddCost,
			MulCost               = 3 * NumTraits<Real>::MulCost + NumTraits<Real>::AddCost
		};

		EIGEN_DEVICE_FUNC static inline Real epsilon()         { return NumTraits<Real>::epsilon();         }
		EIGEN_DEVICE_FUNC static inline Real dummy_precision() { return NumTraits<Real>::dummy_precision(); }
		EIGEN_DEVICE_FUNC static inline int  digits10()        { return NumTraits<Real>::digits10();        }
	};

	template<typename T, typename BinaryOp>
	struct ScalarBinaryOpTraits<T, emilib::Dual<T>, BinaryOp>
	{
		using ReturnType = emilib::Dual<T>;
	};

	template<typename T, typename BinaryOp>
	struct ScalarBinaryOpTraits<emilib::Dual<T>, T, BinaryOp>
	{
		using ReturnType = emilib::Dual<T>;
	};

	} // namespace Eigen
#endif // DUALS_EGIEN
