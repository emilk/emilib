//
//  profiler_gui.hpp
//  Created for ghostel
//
//  Created by Emil Ernerfeldt on 2016-03-05.
//  Copyright © 2016 Emil Ernerfeldt. All rights reserved.
//

#pragma once

namespace profiler {

#define PROPER_PINCH_INPUT 1 // TODO: FINGER_MOTION

struct Input
{
	double scroll_x       = 0; // Delta in points.
	double scroll_y       = 0; // Delta in points.
	double pinch_zoom     = 1; // How many times further apart are the fingers now?
	double pinch_center_x = 0;
	double pinch_center_y = 0;
};

/// Show a flamegraph profiler based on data from profiler.hpp
void paint_profiler_gui(const Input& input);

} // namespace profiler
