// By Emil Ernerfeldt 2013-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

namespace emilib {
namespace os {

// ----------------------------------------------------------------------------

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

/// e.g. 2 on most iOS devices.
float pixels_per_point();

#if TARGET_OS_IPHONE
	struct Size
	{
		float width, height;
	};

	Size screen_size_points();
	Size screen_size_points_landscape();
	Size screen_size_px_landscape();
	Size screen_size_px_portrait();
	Size screen_size_points_portrait();
#endif // TARGET_OS_IPHONE

// ----------------------------------------------------------------------------

/// On iOS/MacOS this is the only place where an app can write files.
/// The returned path ends with a slash.
std::string user_documents_dir();

// -----------------------------------------------------------------

/// mkdir -p dir
bool create_folders(const char* dir);

bool delete_folder(const char* dir);

// -----------------------------------------------------------------

/// Call BEFORE initializing e.g. OpenAL to allow background music to play.
void set_audio_category_ambient();

} // namespace os
} // namespace emilib
