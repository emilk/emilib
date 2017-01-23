// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cmath>
#include <cstdint> // uint8_t etc
#include <limits>

namespace math {

template<class T>
constexpr T PI = static_cast<T>(3.1415926535897932384626433);

template<class T>
constexpr T TAU = 2 * PI<T>;

template<class T>
constexpr T INF = std::numeric_limits<T>::infinity();

constexpr float PIf  = PI<float>;
constexpr float TAUf = 2 * PIf; ///< Oh yes. http://tauday.com/tau-manifesto.pdf
constexpr float NaNf = std::numeric_limits<float>::quiet_NaN();
constexpr float INFf = std::numeric_limits<float>::infinity();

// ----------------------------------------------------------------------------

template<typename F>
constexpr int floor_to_int(F f)
{
	return (int)floor(f); // TODO: doesn't work as constexpr!
}

template<typename F>
constexpr int ceil_to_int(F f)
{
	return (int)ceil(f);
}

/// nearest integer, rounding away from zero in halfway cases
template<typename F>
constexpr int round_to_int(F f)
{
	//return (int)round(f); // doesn't work as constexpr!
	//return floor_to_int(f+0.5f);
	return int(f < 0 ? f-0.5f : f+0.5f); // int(foo) rounds towards zero
}
static_assert(round_to_int(+0.4) == 0,  "round_to_int test");
static_assert(round_to_int(+0.5) == +1, "round_to_int test");
static_assert(round_to_int(+0.6) == +1, "round_to_int test");
static_assert(round_to_int(-0.4) == 0,  "round_to_int test");
static_assert(round_to_int(-0.5) == -1, "round_to_int test");
static_assert(round_to_int(-0.6) == -1, "round_to_int test");

/// nearest integer, rounding away from zero in halfway cases
template<typename F>
constexpr unsigned round_to_uint(F f)
{
	return unsigned(f + 0.5f);
}
static_assert(round_to_uint(+0.4) == 0,  "round_to_uint test");
static_assert(round_to_uint(+0.5) == +1, "round_to_uint test");
static_assert(round_to_uint(+0.6) == +1, "round_to_uint test");

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr T clamp(T x, T min, T max)
{
	return (x < min ? min : x > max ? max : x);
}

template<typename T>
inline constexpr T saturate(T x)
{
	return clamp<T>(x, 0, 1);
}

template<typename T>
inline constexpr T lerp(const T& a, const T& b, float t)
{
	return a*(1-t) + b*t;
}

template<typename T>
inline constexpr T lerp(const T& a, const T& b, double t)
{
	return a*(1-t) + b*t;
}

// For color-components:
template<>
inline constexpr uint8_t lerp(const uint8_t& a, const uint8_t& b, float t)
{
	return (uint8_t)round_to_int((1 - t) * a + t * b);
}

inline float remap(float x, float in_min, float in_max,
                   float out_min, float out_max)
{
	float t = (x - in_min) / (in_max - in_min);
	return lerp(out_min, out_max, t);
}

inline float remap_clamp(float x, float in_min, float in_max,
                         float out_min, float out_max)
{
	float t = (x - in_min) / (in_max - in_min);
	t = saturate(t);
	return lerp(out_min, out_max, t);
}

/// Optional last argument with an easing, e.g. ease_in_ease_out
inline float remap_clamp(float x, float in_min, float in_max,
                         float out_min, float out_max, float(*ease)(float))
{
	float t = (x - in_min) / (in_max - in_min);
	t = saturate(t);
	t = ease(t);
	return lerp(out_min, out_max, t);
}

// ----------------------------------------------------------------------------

template<typename T>
constexpr T min3(const T& a, const T& b, const T& c)
{
	return std::min(a, std::min(b, c));
}

template<typename T>
constexpr T max3(const T& a, const T& b, const T& c)
{
	return std::max(a, std::max(b, c));
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr T sqr(T x)
{
	return x*x;
}

template<typename T>
inline constexpr T cube(T x)
{
	return x*x*x;
}

// ----------------------------------------------------------------------------

inline float deg2rad(float a)
{
	return a * PIf / 180;
}

inline float rad2deg(float a)
{
	return a * 180 / PIf;
}

// ----------------------------------------------------------------------------

} // namespace math
