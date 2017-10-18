// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "wav.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include <loguru.hpp>

namespace emilib {

Wav parse_wav(const void* wav_void_data, size_t wav_size)
{
	const uint8_t* data = reinterpret_cast<const uint8_t*>(wav_void_data);
	size_t         pos = 0;

	auto read_uint16 = [&]() -> uint16_t
	{
		if (pos + sizeof(uint16_t) > wav_size) {
			throw std::runtime_error("Premature end of WAV file.");
		}
		pos += sizeof(uint16_t);
		return uint16_t(data[pos - 2])
		     | uint16_t(uint16_t(data[pos - 1]) << 8);
	};

	auto read_uint32 = [&]() -> uint32_t
	{
		if (pos + sizeof(uint32_t) > wav_size) {
			throw std::runtime_error("Premature end of WAV file.");
		}
		pos += sizeof(uint32_t);
		return (uint32_t(data[pos - 4]) <<  0)
		     | (uint32_t(data[pos - 3]) <<  8)
		     | (uint32_t(data[pos - 2]) << 16)
		     | (uint32_t(data[pos - 1]) << 24);
	};

	auto skip = [&](size_t n)
	{
		if (pos + n > wav_size) {
			throw std::runtime_error("Premature end of WAV file.");
		}
		pos += n;
	};

	auto check_four_bytes = [&](const char* expected)
	{
		if (pos + 4 > wav_size) {
			throw std::runtime_error("Premature end of WAV file.");
		}
		return strncmp(reinterpret_cast<const char*>(data + pos), expected, 4) == 0;
	};

	auto skip_four_bytes = [&](const char* expected)
	{
		if (!check_four_bytes(expected)) {
			const auto actual = std::string(reinterpret_cast<const char*>(data + pos), 4);
			throw std::runtime_error("Not a WAV file: expected '" + std::string(expected) + "' block, got '" + actual + "'");
		}
		skip(4); // skip expected
	};

	// ------------------------------------------------------------------------

	skip_four_bytes("RIFF");

	// we had 'RIFF' let's continue
	skip(4); // skip file size, which is probably wrong anyway.

	skip_four_bytes("WAVE");

	// ------------------------------------------------------------------------

	if (check_four_bytes("JUNK")) {
		skip(4); // skip "JUNK"
		uint32_t chunk_size = read_uint32();
		chunk_size += (chunk_size % 2);
		skip(chunk_size);
	}

	// ------------------------------------------------------------------------

	skip_four_bytes("fmt ");

	{
		uint32_t chunk_size = read_uint32();
		if (chunk_size != 16) {
			throw std::runtime_error("Expected 'fmt ' block to be 16 bytes, was " + std::to_string(chunk_size));
		}
	}

	int16_t audio_format = read_uint16(); // our 16 values
	if (audio_format != 1) {
		throw std::runtime_error("Not PCM");
	}

	uint16_t channels        = read_uint16(); // 1 mono, 2 stereo
	uint32_t sample_rate     = read_uint32();
	uint32_t bytes_per_sec   = read_uint32();
	// uint16_t block_align     = read_uint16();
	skip(2); // Skip block_align - its probably 2 bytes.
	uint16_t bits_per_sample = read_uint16(); // 8 bit or 16 bit file?

	// ------------------------------------------------------------------------

	// Non-standard filler block:
	if (check_four_bytes("FLLR")) {
		skip(4); // skip "FLLR"
		uint32_t chunk_size = read_uint32();
		chunk_size += (chunk_size % 2);
		skip(chunk_size);
	}

	// ------------------------------------------------------------------------

	skip_four_bytes("data");
	uint32_t data_size = read_uint32(); // How many bytes of sound data we have

	if (pos + data_size < wav_size) {
		// There might be trailing blocks. Ignore.
		auto extra = wav_size - data_size - pos;
		LOG_F(WARNING, "%lu bytes of extra data in WAV file", extra);
	}
	if (pos + data_size > wav_size) {
		throw std::runtime_error("Premature end of WAV file.");
	}

	// ------------------------------------------------------------------------

	Wav wav;
	wav.duration_sec     = data_size / (double)bytes_per_sec;
	wav.channels         = channels;
	wav.bits_per_sample  = bits_per_sample;
	wav.sample_rate      = sample_rate;
	wav.data             = data + pos;
	wav.data_size        = data_size;
	return wav;
}

} // namespace emilib
