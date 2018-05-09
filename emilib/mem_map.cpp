// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
#include "mem_map.hpp"

#include <stdexcept>
#include <string>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <loguru.hpp>

namespace emilib {

MemMap::MemMap(const char* path)
{
	auto file = open(path, O_RDONLY);
	struct stat file_status;
	auto ret = fstat(file, &file_status);
	if (ret != 0) {
		throw std::runtime_error(
			(std::string)"Failed to stat file '" + path + "' - maybe it's not there?");
	}

	_size = (size_t)file_status.st_size;
	_data = mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, file, 0);
	CHECK_NE_F(_data, MAP_FAILED);

	// Note this will not close the file/mapping right now, as it will be held until unmapped.
	close(file);
}

MemMap::~MemMap()
{
	if (_data) {
		munmap(_data, _size);
	}
}

void MemMap::operator=(MemMap&& o)
{
	std::swap(this->_size, o._size);
	std::swap(this->_data, o._data);
}

} // namespace emilib
