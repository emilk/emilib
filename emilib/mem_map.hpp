// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel

#pragma once

#include <cstddef>  // size_t

namespace emilib {

/// Memory-mapped file. Really fast way of reading stuff from disk.
class MemMap
{
public:
	MemMap() : _size(0), _data(nullptr) { }
	explicit MemMap(const char* path); // Open for reading
	~MemMap();

	MemMap(MemMap&& o) : _size(o._size), _data(o._data)
	{
		o._size = 0;
		o._data = nullptr;
	}

	void operator=(MemMap&& o);

	size_t      size() const { return _size; }
	const void* data() const { return _data; }

private:
	MemMap(MemMap&) = delete;
	MemMap& operator=(MemMap&) = delete;

	size_t _size;
	void*  _data;
};

} // namespace emilib

