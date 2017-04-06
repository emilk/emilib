// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel.

#pragma once

#include <cstddef> // size_t

namespace emilib {

struct Wav
{
	double      duration_sec;     ///< Length of the sound in seconds.
	int         channels;         ///< 1=mono, 2=stereo.
	int         bits_per_sample;  ///< Probably 16.
	int         sample_rate;      ///< Frames per second. Probably 44100.
	const void* data;             ///< Sample data, little-endian.
	size_t      data_size;        ///< Number of bytes in 'data'.
};

/// Throws on failure. The returned Wav will point into wav_data.
Wav parse_wav(const void* wav_data, size_t size_bytes);

} // namespace emilib
