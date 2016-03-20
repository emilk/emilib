//  Created by Emil Ernerfeldt on 2012-05-26.
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

	// The abolute value of this is dubious, but
	static double current_time_secs();

private:
	using Clock = std::chrono::steady_clock; // Always counting up.
	Clock::time_point _start;
};

} // namespace emilib
