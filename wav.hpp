#include <cstddef> // size_t

namespace emilib {

struct Wav
{
	double      duration_sec;     // Length of the sound in seconds.
	int         channels;         // 1=mono, 2=stereo.
	int         bits_per_sample;  // Probably 16.
	int         sample_rate;      // Frames per second. Probably 44100.
	const void* data;             // Sample data, little-endian.
	size_t      data_size;        // Number of bytes in 'data'.
};

// Throws on failure. The returned Wab will point into wav_data.
Wav parse_wav(const void* wav_data, size_t size_bytes);

} // namespace emilib
