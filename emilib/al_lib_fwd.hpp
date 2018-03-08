// By Emil Ernerfeldt 2012-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// Forward declarations for things in al_lib.hpp.
#pragma once

#include <memory>

namespace al {

class Buffer;
using Buffer_SP = std::shared_ptr<Buffer>;

class Source;
using Source_SP = std::shared_ptr<Source>;

} // namespace al
