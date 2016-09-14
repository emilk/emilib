// By Emil Ernerfeldt 2013-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "os.hpp"

#include <Foundation/Foundation.h>
#import <GLKit/GLKit.h>

#if TARGET_OS_IPHONE
	#import <UIKit/UIKit.h>
#endif // TARGET_OS_IPHONE

namespace emilib {
namespace os {

Device device()
{
#if TARGET_OS_IPHONE
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
		return Device::Tablet;
	} else {
		return Device::Phone;
	}
#else
	return Device::Desktop;
#endif
}

float pixels_per_point()
{
#if TARGET_OS_IPHONE
	UIScreen* screen = [UIScreen mainScreen];
	return (float)screen.scale;
#else
	float displayScale = 1;
	if ([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)]) {
		NSArray *screens = [NSScreen screens];
		for (int i = 0; i < [screens count]; i++) {
		float s = (float)[[screens objectAtIndex:i] backingScaleFactor];
		if (s > displayScale)
			displayScale = s;
		}
	}
	return displayScale;
#endif
}

#if TARGET_OS_IPHONE
Size screen_size_points()
{
	UIScreen* screen = [UIScreen mainScreen];
	CGRect screenRect = [screen bounds];
	CGSize size = screenRect.size;
	return Size{(float)size.width, (float)size.height};
}
#endif // TARGET_OS_IPHONE

} // namespace os
} // namespace emilib
