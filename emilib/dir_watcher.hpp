// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY
//   2014 - Created for Ghostel
//   2016 - revived for PipeDreams
//   2016 - added to emilib

#pragma once

#include <ctime>
#include <set>
#include <string>
#include <vector>

struct kevent;

namespace emilib {

/// Watches for any changes in a directory (file changed, added, removed).
class DirWatcher
{
public:
	/// Feel free to end with a slash or not.
	explicit DirWatcher(std::string dir);
	~DirWatcher();

	/// Returns a list of absolute paths to files that where added, removed or changed.
	std::vector<std::string> poll_files();

private:
	DirWatcher(DirWatcher&) = delete;
	DirWatcher(DirWatcher&&) = delete;
	DirWatcher& operator=(DirWatcher&) = delete;
	DirWatcher& operator=(DirWatcher&&) = delete;

	struct File
	{
		std::string       path; // Full, absolute path.
		std::string       file_name;
		time_t            mtime;
		bool              is_dir;
		std::vector<File> children;
		int               fd = -1; // File descriptor
	};

	void create();
	void add_dir(std::string dir);
	void destroy();

	void add_dir(File& dir);
	void poll_files_in(std::vector<std::string>& changes, File& dir);
	void rescan(std::vector<std::string>& changes, File& dir);

	void add_kevent(File& file);

	void close_file(File& file);

private:
	const std::string   _dir;
	bool                _recursive   = true;
	bool                _check_files = true;
	std::vector<File>   _dirs;
	std::vector<kevent> _events;
	int                 _kqueue;
};

// -------------------------------------------------------

/// Acts like DirWatcher but with a delay of a few frames to let things 'settle'
class DelayedDirWatcher
{
public:
	/// frame_delay: wait this many calls to poll_files before reporting a change.
	DelayedDirWatcher(std::string dir, unsigned frame_delay = 6);

	std::vector<std::string> poll_files();

private:
	const unsigned        _frame_delay;
	DirWatcher            _dir_watcher;
	std::set<std::string> _dirty_files;
	unsigned              _frames_since_last_change = 0;
};

} // namespace emilib
