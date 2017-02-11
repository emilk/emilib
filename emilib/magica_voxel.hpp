// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cstdint>
#include <vector>

namespace emilib {
namespace magica_voxel {

struct RGBA  { uint8_t r, g, b, a;     };
struct Voxel { uint8_t x, y, z, color; };

struct Model
{
	int                version      = -1;
	int                size[3]      = {0}; ///< width, height, depth
	RGBA               palette[256];
	std::vector<Voxel> voxels;
};

/// Load a .vox MagicaVoxel model.
std::vector<Model> load(const char* path);

} // namespace magica_voxel
} // namespace emilib
