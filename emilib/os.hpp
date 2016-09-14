// By Emil Ernerfeldt 2013-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

namespace emilib {
namespace os {

enum class Device
{
	Phone,
	Tablet,
	Desktop
};

Device device();

inline bool is_phone()    { return device() == Device::Phone;   }
inline bool is_tablet()   { return device() == Device::Tablet;  }
inline bool is_desktop()  { return device() == Device::Desktop; }

// ----------------------------------------------------------------------------
// Display:

// e.g. 2 on most iOS devices.
float pixels_per_point();

#if TARGET_OS_IPHONE
	struct Size
	{
		float width, height;
	};

	Size screen_size_points();
#endif // TARGET_OS_IPHONE

} // namespace os
} // namespace emilib
