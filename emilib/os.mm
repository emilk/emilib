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
namespace {

// Path to app root
NSString* app_path()
{
	//NSHomeDirectory() ?
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	return [paths objectAtIndex:0];
}

std::string from_ns_string(NSString* string)
{
	std::string utf8 = [string cStringUsingEncoding:NSUTF8StringEncoding];
	return utf8;
}

} // namespace

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

std::string user_documents_dir()
{
	NSString* path_ns = app_path();
	std::string dir = [path_ns fileSystemRepresentation];
	if (dir.empty() || dir.back() != '/') {
		dir += "/";
	}
	return dir;
}

// ----------------------------------------------------------------------------

void create_folders(const char* dir)
{
	NSString *ns_dir = [NSString stringWithUTF8String: dir];
	NSError *error;

	if (![[NSFileManager defaultManager] createDirectoryAtPath:ns_dir
								   withIntermediateDirectories:NO
													attributes:nil
														 error:&error])
	{
		NSLog(@"Failed to create directory %@: %@", ns_dir, error);
	}
}

void delete_folder(const char* dir)
{
	NSString *ns_dir = [NSString stringWithUTF8String: dir];
	NSError *error;
	BOOL success = [[NSFileManager defaultManager] removeItemAtPath:ns_dir error:&error];
	if (!success) {
		NSLog(@"Failed to delete folder %@: %@", ns_dir, error);
	}
}

} // namespace os
} // namespace emilib
