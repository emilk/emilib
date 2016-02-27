#pragma once

#include <cstddef>  // size_t

namespace emilib {

// Memory-mapped file. Really fast way of reading stuff from disk.
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

