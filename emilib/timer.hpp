// By Emil Ernerfeldt 2012-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2012-05-26.

#pragma once

#include <chrono>

namespace emilib {

// Simple wall-time monotonic clock.
class Timer
{
public:
	// Will start the Timer.
	Timer();

	// Returns seconds since last reset().
	double reset();

	double secs() const;
	unsigned long long nanoseconds() const;

	// Functions for going back or forward in time:
	void set_secs(double s);
	void set_nanoseconds(double s);

	// Seconds since start of program. The value of this is dubious.
	static double seconds_since_startup();

private:
	using Clock = std::chrono::steady_clock; // Always counting up.
	Clock::time_point _start;
};

} // namespace emilib
