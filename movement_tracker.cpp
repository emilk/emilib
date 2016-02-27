//  Created by emilk on 2012-11-11.
//  Copyright (c) 2015 Emil Ernerfeldt. All rights reserved.
//

#include "movement_tracker.hpp"

#include <cmath>

namespace emilib {

double wrap_angle(double a)
{
	while (a < -M_PI) a += 2.0 * M_PI;
	while (a > +M_PI) a -= 2.0 * M_PI;
	return a;
}

float RotationTracker::velocity(double now) const
{
	size_t begin;
	if (!velocity_calc_begin(begin, now)) {
		return 0;
	}

	double dt = _list.back().when - _list[begin].when;

	if (dt <= 0) {
		return 0;
	}

	// Sum intelligently:
	double sum = 0;
	for (size_t i = begin + 1; i < _list.size(); ++i) {
		sum += wrap_angle( _list[i].where - _list[i-1].where );
	}

	return (float)(sum / dt);
}

} // namespace emilib
