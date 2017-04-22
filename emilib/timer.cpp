// By Emil Ernerfeldt 2012-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "timer.hpp"

namespace emilib {

static Timer s_startup_timer;

Timer::Timer()
{
	reset();
}

double Timer::reset()
{
	auto now = Clock::now();
	auto dur = now - _start;
	_saved_ns = 0;
	_start = now;
	_paused = false;
	return 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
}

double Timer::secs() const
{
	return nanoseconds() * 1e-9;
}

unsigned long long Timer::nanoseconds() const
{
	auto dur = Clock::now() - _start;
	return _saved_ns + std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
}

void Timer::set_secs(double s)
{
	set_nanoseconds(s * 1e9);
}

void Timer::set_nanoseconds(double ns_f)
{
	std::chrono::nanoseconds dur(static_cast<unsigned long long>(ns_f));
	_saved_ns = dur.count();
	_start = Clock::now();
}

void Timer::set_paused(bool paused)
{
	if (_paused == paused) { return; }
	_paused = paused;
	if (paused) {
		_saved_ns  = nanoseconds();
	} else {
		_start = Clock::now();
	}
}

double Timer::seconds_since_startup()
{
	return s_startup_timer.secs();
}

} // namespace emilib
