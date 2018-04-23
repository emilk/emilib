// By Emil Ernerfeldt 2012-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY
//   2012-11-03 - Created for PipeDreams as FILEWrapper.hpp.
//   2014       - Adapted for Ghostel
//   2016       - Moved into emilib.

#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace fs {

class FILEWrapper
{
public:
	FILEWrapper() : _fp(0) {}

	/// throws on fail
	FILEWrapper(const std::string& path, const char* mode);
	~FILEWrapper();

	void close();

	/// Nice version
	bool try_open(const std::string& path, const char* mode);

	bool error() const;
	bool end_of_file() const;

	void read_or_die(void* dest, size_t nbytes);

	/// Returns the number of read bytes
	size_t try_read(void* dest, size_t nbytes);

	void write(const void* src, size_t nbytes);

	/// Write. Now.
	void flush();

	/// Origin = SEEK_SET, SEEK_CUR or SEEK_END
	void seek(int offset, int origin);

	long tell() const;

	/// Returns true on success
	bool read_line(char* dest, int nbytes);

	FILE* handle();

private:
	FILE* _fp;

	FILEWrapper(FILEWrapper&);
	FILEWrapper& operator=(FILEWrapper&);
};

// ------------------------------------------------
// Helper for reading/writing/listing files:

bool                  file_exists(const std::string& path);
size_t                file_size(const std::string& path);
time_t                modified_time(const std::string& path);
bool                  is_file(const std::string& path);
bool                  is_directory(const std::string& path);
std::vector<uint8_t>  read_binary_file(const std::string& path);
std::string           read_text_file(const std::string& path);
void                  write_binary_file(const std::string& path, const void* data, size_t nbytes);
void                  write_text_file(const std::string& path, const char* text);

std::vector<std::string> files_in_directory(const std::string& path);

void print_tree(const std::string& path, const std::string& indent = "");

/// Call the given visitor on all files in the given path recursively.
/// All returned paths will have the @p path as a prefix.
void walk_dir(const std::string& path, const std::function<void(const std::string&)>& visitor);

// ------------------------------------------------
// Path handling:

/// Returns whatever comes after the last . or "", e.g. "foo.bar.png" -> "png"
std::string file_ending(const std::string& path);

/// Returns whatever comes before the last . "foo.bar.png" -> "foo.bar"
std::string without_ending(const std::string& path);

/// strip_path("foo/bar/", "foo/bar/baz/mushroom") -> "baz/mushroom"
std::string strip_path(const std::string& dir_path, const std::string& file_path);

/// foo/bar/baz -> foo/bar/
std::string file_dir(const std::string& path);

/// foo/bar/baz.png -> baz.png
std::string file_name(const std::string& path);

/// foo/bar/baz.png -> baz.png
const char* file_name(const char* path);

} // namespace fs
