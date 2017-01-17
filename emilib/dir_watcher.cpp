// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "dir_watcher.hpp"

#include <cstring>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __linux__
	// On linux you need to install libkqueue-dev
	#include <kqueue/sys/event.h>
#else
	#include <sys/event.h>
#endif

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <loguru.hpp>

namespace emilib {

using namespace std;

DirWatcher::DirWatcher(std::string dir) : _dir(std::move(dir))
{
	create();
	add_dir(_dir);
}

DirWatcher::~DirWatcher()
{
	destroy();
}

void DirWatcher::create()
{
	_kqueue = kqueue();
}

void DirWatcher::destroy()
{
	for (auto& file : _dirs) {
		close_file(file);
	}
	_dirs.clear();
	_events.clear();
	close(_kqueue);
	_kqueue = 0;
}

void DirWatcher::add_dir(std::string path)
{
	CHECK_GT_F(path.size(), 0);
	if (path[path.size()-1] == '/') {
		path.resize(path.size() - 1);
	}

	struct stat info;
	if (stat(path.c_str(), &info) != 0) {
		LOG_F(ERROR, "DirWatcher: Failed to stat directory '%s'", path.c_str());
		return;
	}

	if (!S_ISDIR(info.st_mode)) {
		LOG_F(ERROR, "DirWatcher: Not a directory: '%s'", path.c_str());
		return;
	}

	File file;
	file.path = path;
	file.mtime = info.st_mtime;
	file.is_dir = true;
	add_kevent(file);

	if (_recursive || _check_files) {
		add_dir(file);
	}

	_dirs.emplace_back(std::move(file));
}

void DirWatcher::add_dir(File& dir)
{
	CHECK_F(dir.is_dir);

	auto directory = opendir(dir.path.c_str());
	if (!directory) {
		LOG_F(ERROR, "Failed to open directory '%s'", dir.path.c_str());
		return;
	}
	while (auto dentry = readdir(directory)) {
		if (dentry->d_name[0] == '.') { continue; }
		string child_path = dir.path + "/" + dentry->d_name;
		struct stat info;
		if (stat(child_path.c_str(), &info) != 0) {
			LOG_F(ERROR, "DirWatcher: Failed to stat '%s'", child_path.c_str());
			continue;
		}
		if ((_check_files && S_ISREG(info.st_mode)) ||
		    (_recursive   && S_ISDIR(info.st_mode)))
		{
			File child;
			child.path      = child_path;
			child.file_name = dentry->d_name;
			child.mtime     = info.st_mtime;
			child.is_dir    = S_ISDIR(info.st_mode);
			add_kevent(child);
			if (child.is_dir) {
				add_dir(child);
			}
			dir.children.emplace_back(std::move(child));
		}
	}
	closedir(directory);
}

void DirWatcher::add_kevent(File& file)
{
	CHECK_EQ_F(file.fd, -1);
	int fd = open(file.path.c_str(), O_RDONLY);
	if (fd == -1) {
		LOG_F(ERROR, "DirWatcher: Failed to open '%s': %s", file.path.c_str(), strerror(errno));
		return;
	}

	struct kevent event;
	EV_SET(&event, fd, EVFILT_VNODE,
		   EV_ADD | EV_ENABLE | EV_ONESHOT,
		   NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB,
		   0, 0);
	file.fd = fd;
	_events.push_back(event);
}

vector<string> DirWatcher::poll_files()
{
	bool did_change = false;

	const struct timespec time_out {0, 0};

	struct kevent event;
	while (auto nev = kevent(_kqueue, _events.data(), (int)_events.size(), &event, 1, &time_out))
	{
		if (nev == -1) {
			perror("DirWatcher: kevent");
			LOG_F(ERROR, "DirWatcher '%s': kevent perror", _dir.c_str());
			destroy();
			create();
			add_dir(_dir);
			did_change = true;
			break;
		} else {
			did_change = true;
		}
	}

	vector<string> changes;
	if (did_change) {
		//LOG_F(1, "DirWatcher: kevent reported a change");
		for (auto& dir : _dirs) {
			poll_files_in(changes, dir);
		}
	}
	return changes;
}

void DirWatcher::poll_files_in(std::vector<std::string>& changes, File& dir)
{
	CHECK_F(dir.is_dir);

	auto path = dir.path.c_str();
	struct stat info;
	if (stat(path, &info) != 0) {
		LOG_F(ERROR, "DirWatcher: Failed to stat directory '%s'", path);
		return;
	}

	if (info.st_mtime != dir.mtime) {
		dir.mtime = info.st_mtime;
		LOG_F(1, "DirWatcher: change detected in directory '%s'", path);
		//changes.push_back(dir.path);
	}

	rescan(changes, dir);

	for (auto& child : dir.children) {
		if (child.is_dir) {
			poll_files_in(changes, child);
		}
	}
}

void DirWatcher::rescan(std::vector<std::string>& changes, File& dir)
{
	CHECK_F(dir.is_dir);

	auto directory = opendir(dir.path.c_str());
	if (!directory) {
		LOG_F(ERROR, "DirWatcher: Failed to open directory '%s'", dir.path.c_str());
		return;
	}

	unsigned file_ix = 0;

	while (auto dentry = readdir(directory)) {
		if (dentry->d_name[0] == '.') { continue; }
		string child_path = dir.path + "/" + dentry->d_name;
		struct stat info;
		if (stat(child_path.c_str(), &info) != 0) {
			LOG_F(ERROR, "DirWatcher: Failed to stat '%s'", child_path.c_str());
			continue;
		}

		if ((_check_files && S_ISREG(info.st_mode)) ||
		    (_recursive   && S_ISDIR(info.st_mode)))
		{
			bool is_new_file = false;

			if (file_ix < dir.children.size()) {
				auto& entry = dir.children[file_ix];
				int result = strcmp(entry.file_name.c_str(), dentry->d_name);
				if (result == 0) {
					if (entry.mtime != info.st_mtime) {
						entry.mtime = info.st_mtime;
						LOG_F(1, "DirWatcher: change detected in '%s'", child_path.c_str());
						changes.push_back(child_path);
					}
					file_ix += 1;
				} else if (result < 0) {
					LOG_F(1, "DirWatcher: deleted: '%s'", entry.path.c_str());
					if (entry.fd != -1) {
						close(entry.fd);
						entry.fd = -1;
					}
					changes.push_back(entry.path);
					dir.children.erase(begin(dir.children) + file_ix);
					// Keep file_ix the same
				} else {
					is_new_file = true;
				}
			} else {
				is_new_file = true;
			}

			if (is_new_file) {
				// dentry is a new file:
				LOG_F(1, "DirWatcher: added: '%s'", child_path.c_str());
				changes.push_back(child_path);
				File child;
				child.path      = child_path;
				child.file_name = dentry->d_name;
				child.mtime     = info.st_mtime;
				child.is_dir    = S_ISDIR(info.st_mode);
				add_kevent(child);
				if (child.is_dir) {
					add_dir(child);
				}
				dir.children.insert(begin(dir.children) + file_ix, std::move(child));
				file_ix += 1;
			}
		}
	}
	closedir(directory);
}

void DirWatcher::close_file(File& file)
{
	for (auto& child : file.children) {
		close_file(child);
	}

	if (file.fd != -1) {
		close(file.fd);
		file.fd = -1;
	}
}

// -------------------------------------------------------

DelayedDirWatcher::DelayedDirWatcher(std::string dir, unsigned frame_delay)
	: _frame_delay(frame_delay), _dir_watcher(std::move(dir))
{
}

std::vector<std::string> DelayedDirWatcher::poll_files()
{
	auto changed = _dir_watcher.poll_files();
	if (!changed.empty()) {
		_dirty_files.insert(begin(changed), end(changed));
		_frames_since_last_change = 0;
	}

	if (!_dirty_files.empty()) {
		if (_frames_since_last_change >= _frame_delay) {
			std::vector<std::string> ret(begin(_dirty_files), end(_dirty_files));
			_dirty_files.clear();
			return ret;
		} else {
			_frames_since_last_change += 1;
		}
	}

	return {};
}

} // namespae emilib
