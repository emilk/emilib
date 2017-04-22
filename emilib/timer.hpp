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

/// Simple wall-time monotonic clock.
class Timer
{
public:
	/// Will start the Timer.
	Timer();

	/// Returns seconds since last reset().
	/// Un-pauses.
	double reset();

	double secs() const;
	unsigned long long nanoseconds() const;

	/// Functions for going back or forward in time:
	void set_secs(double s);
	void set_nanoseconds(double s);

	/// Time stops increasing while paused. false by default;
	void set_paused(bool b);
	bool is_paused() { return _paused; }

	/// Seconds since start of program.
	static double seconds_since_startup();

private:
	using Clock = std::chrono::steady_clock; // Always counting up.
	unsigned long long _saved_ns = 0;
	bool               _paused   = false;
	Clock::time_point  _start;
};

} // namespace emilib
