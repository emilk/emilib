// By Emil Ernerfeldt 2002-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// Wrapper around OpenAL, a library for playing sounds.
// HISTORY: Originally from around 2002.
// 2018: Refactor Sound and rename it as Buffer.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "al_lib_fwd.hpp"

using ALCdevice = struct ALCdevice_struct;
using ALCcontext = struct ALCcontext_struct;

namespace al {

// ----------------------------------------------------------------------------

struct Vec3f
{
	float _data[3];

	Vec3f() {}
	Vec3f(float x, float y, float z) : _data{x, y, z} {}

	float* data() { return _data; }
	const float* data() const { return _data; }

	#ifdef AL_LIB_VEC3f_EXTRAS
		AL_LIB_VEC3f_EXTRAS // Use this to define implicit conversions to/from your own types.
	#endif
};

void check_for_al_error();

// ----------------------------------------------------------------------------

/// A loaded sound. Can be played via Source. Many Source:s can play the same Buffer at the same.
class Buffer
{
public:
	static Buffer make_wav(const std::string& path);

	/// Create and empty Buffer
	explicit Buffer(const std::string& debug_name);

	/// Fill Buffer with the contents of the given wav file.
	void load_wav(const std::string& path);

	void load_mono_float(float sample_rate, const float* samples, size_t num_samples);
	void load_mono_int16(float sample_rate, const int16_t* samples, size_t num_samples);

	Buffer(Buffer&& o) noexcept : _debug_name(o._debug_name), _buffer_id(o._buffer_id), _size_bytes(o._size_bytes) { o._buffer_id = 0; o._size_bytes = 0; }
	~Buffer();

	/// Memory usage.
	unsigned size_bytes() const { return _size_bytes; }

	int get_freqency() const;
	int get_bits() const;
	int get_channels() const;
	int get_size() const;

private:
	friend class Source;
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	std::string _debug_name;
	unsigned    _buffer_id = 0;
	unsigned    _size_bytes = 0;
};

// ----------------------------------------------------------------------------

/// A sound source. Has position, and a sound to play.
class Source
{
public:
	enum State
	{
		INITIAL,
		PLAYING,
		PAUSED,
		STOPPED,
	};

	/// Returns the maximum number of sources possible to have instantiated at the same time
	static int max_sources();

	Source();
	~Source();

	void set_state(State arg);
	State state() const;

	void play();
	void pause();
	void stop();
	void rewind();

	void set_buffer(Buffer_SP buffer);
	const Buffer_SP& buffer() const;

	/// Volume, [0,1]. >1 MAY work.
	void set_gain(float gain);
	float gain() const;

	/// sets pitch (clamped to [0,2]), does affect speed
	void set_pitch(float pitch);
	/// get current pitch
	float pitch() const;

	void set_pos(Vec3f);
	Vec3f pos() const;

	void set_vel(Vec3f);
	Vec3f vel() const;

	void set_direction(Vec3f);
	Vec3f direction() const;

	/**
	 * Indicate distance above which sources are not
	 * attenuated using the inverse clamped distance model.
	 * Default: +inf
	 */
	void set_max_distance(float arg);
	float max_distance() const;

	/// Controls how fast the sound falls off with distance
	void set_rolloff_factor(float arg);
	float rolloff_factor() const;

	/**
	 * source specific reference distance
	 * At 0.0, no distance attenuation occurs.
	 * Default is 1.0.
	 */
	void set_reference_distance(float arg);
	float reference_distance() const;

	void set_min_gain(float arg);
	float min_gain() const;

	void set_max_gain(float arg);
	float max_gain() const;

	void set_cone_outer_gain(float arg);
	float cone_outer_gain() const;

	void set_cone_inner_angle(float arg);
	float cone_inner_angle() const;

	void set_cone_outer_angle(float arg);
	float cone_outer_angle() const;

	/// is the position relative to the listener? false by default
	void set_relative_to_listener(bool arg);
	bool relative_to_listener() const;

	void set_looping(bool);
	bool looping() const;

private:
	Source(const Source&) = delete;
	Source& operator=(const Source&) = delete;

	unsigned  _source;
	Buffer_SP _buffer;
	float     _gain = 1;
};

// ----------------------------------------------------------------------------

/// All Listeners are really the same.
/// TODO: static interface.
class Listener
{
public:
	void set_pos(Vec3f);
	Vec3f pos() const;

	void set_vel(Vec3f);
	Vec3f vel() const;

	void set_orientation(const Vec3f& forward, const Vec3f& up);

	Vec3f direction() const;
	Vec3f up() const;

	void set_gain(float);
	float gain() const;
};

// ----------------------------------------------------------------------------

/// You should have only one of these.
class SoundMngr
{
public:
	/// Look for sounds relative to sfx_dir
	explicit SoundMngr(const std::string& sfx_dir);
	~SoundMngr();

	//------------------------------------------------------------------------------

	/// sound_name == "subdir/foo.wav"
	void prefetch(const std::string& sound_name);

	/// Recursively prefetch all sounds in sfx_dir/sub_folder
	void prefetch_all(const std::string& sub_folder = "");

	/// Fire and forget - or keep the returned source and modify it.
	/// Returns nullptr on fail
	Source_SP play(const std::string& sound_name);

	//------------------------------------------------------------------------------
	// Global settings

	bool is_working() const;

	Listener* listener() { return &_listener; }

	enum DistanceModel
	{
		NONE,
		INVERSE_DISTANCE,
		INVERSE_DISTANCE_CLAMPED,
	};

	/// set speed of sound. 344 by default (speed of sound in air in meters/second)
	void set_doppler_vel(float vel);
	/// get speed of sound. 344 by default (speed of sound in air in meters/second)
	float doppler_vel();

	/// default is 1, used to (de)exaggerate the effect of the Doppler effect
	void set_doppler_factor(float factor);
	/// default is 1, used to (de)exaggerate the effect of the Doppler effect
	float doppler_factor();

	/// default is INVERSE_DISTANCE
	void set_distance_model(DistanceModel model);
	/// default is INVERSE_DISTANCE
	DistanceModel distance_model();

	const char* vendor();
	const char* version();
	const char* renderer();
	const char* extensions();

	//------------------------------------------------------------------------------

	void print_memory_usage() const;

private:
	Buffer_SP load_buffer(const std::string& sound_name, bool is_hot);
	Source_SP get_source();

	using BufferMap  = std::unordered_map<std::string, Buffer_SP>;
	using SourceList = std::vector<Source_SP>;

	std::string _sfx_dir;
	ALCdevice*  _device  = nullptr;
	ALCcontext* _context = nullptr;
	Listener    _listener;

	BufferMap   _buffer_map;
	SourceList  _sources;
};

} // namespace al
