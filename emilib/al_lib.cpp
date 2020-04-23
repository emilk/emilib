// By Emil Ernerfeldt 2002-2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "al_lib.hpp"

#include <algorithm>
#include <string>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include <loguru.hpp>

#include "file_system.hpp"
#include "mem_map.hpp"
#include "wav.hpp"

namespace al {

static int s_max_sources = -1;

std::string error_str(ALenum error)
{
	if (error == AL_NO_ERROR)          return "AL_NO_ERROR";
	if (error == AL_INVALID_NAME)      return "AL_INVALID_NAME";
	if (error == AL_ILLEGAL_ENUM)      return "AL_ILLEGAL_ENUM";
	if (error == AL_INVALID_ENUM)      return "AL_INVALID_ENUM";
	if (error == AL_INVALID_VALUE)     return "AL_INVALID_VALUE";
	if (error == AL_ILLEGAL_COMMAND)   return "AL_ILLEGAL_COMMAND";
	if (error == AL_INVALID_OPERATION) return "AL_INVALID_OPERATION";
	if (error == AL_OUT_OF_MEMORY)     return "AL_OUT_OF_MEMORY";

	/*
	Overlaps with the above: use alcGetError for these!
	if (error == ALC_NO_ERROR) return "ALC_NO_ERROR";
	if (error == ALC_INVALID_DEVICE) return "ALC_INVALID_DEVICE";
	if (error == ALC_INVALID_CONTEXT) return "ALC_INVALID_CONTEXT";
	if (error == ALC_INVALID_ENUM) return "ALC_INVALID_ENUM";
	if (error == ALC_INVALID_VALUE) return "ALC_INVALID_VALUE";
	if (error == ALC_OUT_OF_MEMORY) return "ALC_OUT_OF_MEMORY";
	*/

	return std::to_string((unsigned)error);
}

void check_for_al_error()
{
	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		auto err_str = error_str(error);
		LOG_F(ERROR, "OpenAL error: %s", err_str.c_str());
	}
}

// ----------------------------------------------------------------------------

Buffer Buffer::make_wav(const std::string& path)
{
	Buffer buffer(path);
	buffer.load_wav(path);
	return buffer;
}

Buffer::Buffer(const std::string& debug_name) : _debug_name(debug_name)
{
	check_for_al_error();
	alGenBuffers(1, &_buffer_id);
	check_for_al_error();
}

/// Fill buffer with the contents of the given wav file.
void Buffer::load_wav(const std::string& path)
{
	ERROR_CONTEXT("Loading wav", path.c_str());
	const auto mem_map = emilib::MemMap(path.c_str());
	const emilib::Wav wav = emilib::parse_wav(mem_map.data(), mem_map.size());

	if (wav.channels != 1) {
		LOG_F(WARNING, "We don't support attenuation of stereo sound wav:s: '%s'", path.c_str());
	}

	ALenum format = wav.channels == 1 ?
		(wav.bits_per_sample == 16 ? AL_FORMAT_MONO16   : AL_FORMAT_MONO8  ) :
		(wav.bits_per_sample == 16 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8);

	CHECK_F(_buffer_id != 0);
	alBufferData(_buffer_id, format, wav.data, (ALsizei)wav.data_size, wav.sample_rate);

	check_for_al_error();
}

void Buffer::load_mono_float(float sample_rate, const float* samples, size_t num_samples)
{
	std::vector<int16_t> samples16(num_samples);
	bool did_clip = false;
	for (size_t i = 0; i < num_samples; ++i) {
		float sample = samples[i] * 32768.0f;
		if (sample < -32768.0f) {
			sample = -32768.0f;
			did_clip = true;
		}
		if (sample > 32767.0f) {
			sample = 32767.0f;
			did_clip = true;
		}
		samples16[i] = std::round(sample);
	}
	if (did_clip) {
		LOG_F(WARNING, "Clipped sound '%s'", _debug_name.c_str());
	}

	load_mono_int16(sample_rate, samples16.data(), samples16.size());
}

void Buffer::load_mono_int16(float sample_rate, const int16_t* samples, size_t num_samples)
{
	CHECK_F(_buffer_id != 0);
	alBufferData(_buffer_id, AL_FORMAT_MONO16, samples, (ALsizei)num_samples * sizeof(int16_t), sample_rate);
	check_for_al_error();
}

Buffer::~Buffer()
{
	check_for_al_error();
	if (_buffer_id) {
		alDeleteBuffers(1, &_buffer_id);
		check_for_al_error();
	}
}

int Buffer::get_freqency() const
{
	int temp;
	alGetBufferi(_buffer_id, AL_FREQUENCY, &temp);
	return temp;
}

int Buffer::get_bits() const
{
	int temp;
	alGetBufferi(_buffer_id, AL_BITS, &temp);
	return temp;
}

int Buffer::get_channels() const
{
	int temp;
	alGetBufferi(_buffer_id, AL_CHANNELS, &temp);
	return temp;
}

int Buffer::get_size() const
{
	int temp;
	alGetBufferi(_buffer_id, AL_SIZE, &temp);
	return temp;
}

// ----------------------------------------------------------------------------

int Source::max_sources()
{
	if (s_max_sources == -1) {
		/*
		 //add sources until we get an error
		 using container = vectorPOD<ALuint>;
		 container sources;
		 while (alGetError()==AL_NO_ERROR)
		 {
		 sources.push_back();
		 alGenSources(1, &sources.back());
		 }

		 //remove last invalid source
		 sources.pop_back();

		 //free everything again
		 for (container::iterator it = sources.begin(); it!=sources.end(); ++it)
		 alDeleteSources(1, &*it);

		 s_max_sources = static_cast<int>(sources.size());
		 /*/

		s_max_sources = 32;
		/**/
	}

	return s_max_sources;
}

Source::Source()
{
	alGenSources(1, &_source);
	check_for_al_error();
}

Source::~Source()
{
	//check_for_al_error();
	if (_source != 0) {
		alDeleteSources(1, &_source);
		//check_for_al_error();
	}
}

void Source::set_buffer(Buffer_SP buffer)
{
	if (_buffer != buffer) {
		stop();
		_buffer = buffer;
		CHECK_NOTNULL_F(_buffer);
		check_for_al_error();
		CHECK_F(alIsBuffer(_buffer->_buffer_id));
		check_for_al_error();
		CHECK_F(alIsSource(_source));
		check_for_al_error();
		alSourcei(_source, AL_BUFFER, _buffer->_buffer_id);
		check_for_al_error();
	}
}

void Source::set_state(State arg)
{
	int state;
	switch (arg) {
		case INITIAL:   state = AL_INITIAL;  break;
		case PLAYING:   state = AL_PLAYING;  break;
		case PAUSED:    state = AL_PAUSED;   break;
		case STOPPED:   state = AL_STOPPED;  break;
		default:        ABORT_F("Argument must be one of INITIAL, PLAYING, PAUSED or STOPPED");
	};
	alSourcei(_source, AL_SOURCE_STATE, state);
}

Source::State Source::state() const
{
	int temp;
	alGetSourcei(_source, AL_SOURCE_STATE, &temp);
	switch (temp) {
		case AL_INITIAL:   return INITIAL;
		case AL_PLAYING:   return PLAYING;
		case AL_PAUSED:    return PAUSED;
		default:           return STOPPED;
	};
}

void Source::play()
{
	check_for_al_error();
	alSourcePlay(_source);
	check_for_al_error();
}

void Source::pause()
{
	alSourcePause(_source);
}

void Source::stop()
{
	alSourceStop(_source);
}

void Source::rewind()
{
	alSourceRewind(_source);
}

void Source::set_pos(Vec3f arg)
{
	alSourcefv(_source, AL_POSITION, arg.data());
}

Vec3f Source::pos() const
{
	Vec3f temp;
	alGetSourcefv(_source, AL_POSITION, temp.data());
	return temp;
}

void Source::set_vel(Vec3f arg)
{
	alSourcefv(_source, AL_VELOCITY, arg.data());
}

Vec3f Source::vel() const
{
	Vec3f temp;
	alGetSourcefv(_source, AL_VELOCITY, temp.data());
	return temp;
}

void Source::set_direction(Vec3f arg)
{
	alSourcefv(_source, AL_DIRECTION, arg.data());
}

Vec3f Source::direction() const
{
	Vec3f temp;
	alGetSourcefv(_source, AL_DIRECTION, temp.data());
	return temp;
}

void Source::set_pitch(float pitch)
{
	pitch = std::max(0.01f,  pitch);
	pitch = std::min(2.00f,  pitch);
	alSourcef(_source, AL_PITCH, pitch);
}

float Source::pitch() const
{
	float pitch;
	alGetSourcef(_source, AL_PITCH, &pitch);
	return pitch;
}

void Source::set_gain(float arg)
{ alSourcef(_source, AL_GAIN, arg); check_for_al_error(); }
float Source::gain() const
{ float temp; alGetSourcef(_source, AL_GAIN, &temp); return temp; }

void Source::set_min_gain(float arg)
{ alSourcef(_source, AL_MIN_GAIN, arg); check_for_al_error(); }
float Source::min_gain() const
{ float temp; alGetSourcef(_source, AL_MIN_GAIN, &temp); return temp; }

void Source::set_max_gain(float arg)
{ alSourcef(_source, AL_MAX_GAIN, arg); check_for_al_error(); }
float Source::max_gain() const
{ float temp; alGetSourcef(_source, AL_MAX_GAIN, &temp); return temp; }

void Source::set_max_distance(float arg)
{ alSourcef(_source, AL_MAX_DISTANCE, arg); check_for_al_error(); }
float Source::max_distance() const
{ float temp; alGetSourcef(_source, AL_MAX_DISTANCE, &temp); return temp; }

void Source::set_rolloff_factor(float arg)
{ alSourcef(_source, AL_ROLLOFF_FACTOR, arg); check_for_al_error(); }
float Source::rolloff_factor() const
{ float temp; alGetSourcef(_source, AL_ROLLOFF_FACTOR, &temp); return temp; }

void Source::set_reference_distance(float arg)
{ alSourcef(_source, AL_REFERENCE_DISTANCE, arg); check_for_al_error(); }
float Source::reference_distance() const
{ float temp; alGetSourcef(_source, AL_REFERENCE_DISTANCE, &temp); return temp; }

void Source::set_cone_outer_gain(float arg)
{ alSourcef(_source, AL_CONE_OUTER_GAIN, arg); check_for_al_error(); }
float Source::cone_outer_gain() const
{ float temp; alGetSourcef(_source, AL_CONE_OUTER_GAIN, &temp); return temp; }

void Source::set_cone_inner_angle(float arg)
{ alSourcef(_source, AL_CONE_INNER_ANGLE, arg); check_for_al_error(); }
float Source::cone_inner_angle() const
{ float temp; alGetSourcef(_source, AL_CONE_INNER_ANGLE, &temp); return temp; }

void Source::set_cone_outer_angle(float arg)
{ alSourcef(_source, AL_CONE_OUTER_ANGLE, arg); check_for_al_error(); }
float Source::cone_outer_angle() const
{ float temp; alGetSourcef(_source, AL_CONE_OUTER_ANGLE, &temp); return temp; }

void Source::set_relative_to_listener(bool arg)
{ alSourcei(_source, AL_SOURCE_RELATIVE, (arg ? AL_TRUE : AL_FALSE)); check_for_al_error(); }
bool Source::relative_to_listener() const
{ int temp; alGetSourcei(_source, AL_SOURCE_RELATIVE, &temp); return temp==AL_TRUE; }

void Source::set_looping(bool arg)
{ alSourcei(_source, AL_LOOPING, (arg ? AL_TRUE : AL_FALSE)); check_for_al_error(); }
bool Source::looping() const
{ int temp; alGetSourcei(_source, AL_LOOPING, &temp); return temp==AL_TRUE; }

// ----------------------------------------------------------------------------

void Listener::set_pos(Vec3f pos)
{ alListenerfv(AL_POSITION, pos.data()); }

Vec3f Listener::pos() const
{
	Vec3f temp;
	alGetListenerfv(AL_POSITION, temp.data());
	return temp;
}

void Listener::set_vel(Vec3f pos)
{ alListenerfv(AL_VELOCITY, pos.data()); }

Vec3f Listener::vel() const
{
	Vec3f temp;
	alGetListenerfv(AL_VELOCITY, temp.data());
	return temp;
}

void Listener::set_orientation(const Vec3f& forward, const Vec3f& up)
{
	float temp[6];
	std::copy_n(forward.data(), 3, temp);
	std::copy_n(up.data(),      3, temp+3);
	alListenerfv(AL_ORIENTATION, temp);
}

Vec3f Listener::direction() const
{
	float temp[6];
	alGetListenerfv(AL_ORIENTATION, temp);
	return Vec3f{temp[0], temp[1], temp[2]};
}

Vec3f Listener::up() const
{
	float temp[6];
	alGetListenerfv(AL_ORIENTATION, temp);
	return Vec3f{temp[3], temp[4], temp[5]};
}

void Listener::set_gain(float arg)
{ alListenerf(AL_GAIN, arg); }

float Listener::gain() const
{
	float temp;
	alGetListenerf(AL_GAIN, &temp);
	return temp;
}

// ------------------------------------------

SoundMngr::SoundMngr(const std::string& sfx_dir)
	: _sfx_dir(sfx_dir)
{
	// Open default device
	_device = alcOpenDevice(nullptr);

	if (!_device) {
		LOG_F(ERROR, "Could not open default OpenAL device.");
		return;
	}

	_context = alcCreateContext(_device, nullptr);

	if (!_context) {
		LOG_F(ERROR, "Failed to create OpenAL context for default device.");
		return;
	}

	if (!alcMakeContextCurrent(_context)) {
		LOG_F(ERROR, "Failed to set current OpenAL context.");
		return;
	}

	LOG_F(INFO, "OpenAL initialized.");

	check_for_al_error();
}

SoundMngr::~SoundMngr()
{
	_sources.clear();
	_buffer_map.clear();

	alcMakeContextCurrent(nullptr);

	if (_context) {
		alcDestroyContext(_context);
		_context = nullptr;
	}

	if (_device) {
		alcCloseDevice(_device);
		_device = nullptr;
	}
}

bool SoundMngr::is_working() const
{
	return _device != nullptr && _context != nullptr;
}

Buffer_SP SoundMngr::load_buffer(const std::string& sound_name, bool is_hot)
{
	auto it = _buffer_map.find(sound_name);

	if (it == _buffer_map.end()) {
		Buffer_SP buffer_ptr;
		try {
			if (is_hot) {
				LOG_F(WARNING, "Hot-Loading sound '%s'...", sound_name.c_str());
			}
			auto path = _sfx_dir + sound_name;
			buffer_ptr = std::make_shared<Buffer>(Buffer::make_wav(path.c_str()));
			check_for_al_error();
		} catch (std::exception& e) {
			LOG_F(ERROR, "Failed to load sound '%s': %s", sound_name.c_str(), e.what());
		}
		_buffer_map[sound_name] = buffer_ptr;
		return buffer_ptr;
	} else {
		return it->second;
	}
}

void SoundMngr::prefetch(const std::string& sound_name)
{
	load_buffer(sound_name, false);
}

void SoundMngr::prefetch_all(const std::string& sub_folder)
{
	const auto root_path = _sfx_dir + sub_folder;
	fs::walk_dir(root_path, [=](const std::string& file_path) {
		if (fs::file_ending(file_path) == "wav") {
			prefetch(fs::strip_path(root_path, file_path));
		}
	});
}

Source_SP SoundMngr::play(const std::string& sound_name)
{
	if (auto buffer = load_buffer(sound_name, true)) {
		if (auto source = get_source()) {
			source->set_buffer(buffer);
			source->play();
			return source;
		}
	}
	return nullptr;
}

Source_SP SoundMngr::get_source()
{
	check_for_al_error();

	// Erase any not still playing:
	auto erase_it = std::remove_if(begin(_sources), end(_sources), [](auto&& s) {
		return s->state() != Source::PLAYING;
	});
	_sources.erase(erase_it, _sources.end());

	check_for_al_error();

	auto src = std::make_shared<Source>();
	check_for_al_error();
	_sources.push_back(src);
	return src;
}

void SoundMngr::set_doppler_vel(float vel)
{ alDopplerVelocity(vel); }

float SoundMngr::doppler_vel()
{ return alGetFloat(AL_DOPPLER_VELOCITY); }

void SoundMngr::set_doppler_factor(float factor)
{ alDopplerFactor(factor); }

float SoundMngr::doppler_factor()
{ return alGetFloat(AL_DOPPLER_FACTOR); }

void SoundMngr::set_distance_model(DistanceModel model)
{
	switch (model)
	{
		case NONE:               alDistanceModel(AL_NONE);                      break;
		case INVERSE_DISTANCE:   alDistanceModel(AL_INVERSE_DISTANCE);          break;
		default:                 alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);  break;
	}
}

SoundMngr::DistanceModel SoundMngr::distance_model()
{
	switch (alGetInteger(AL_DISTANCE_MODEL))
	{
		case AL_NONE:             return NONE;
		case AL_INVERSE_DISTANCE: return INVERSE_DISTANCE;
		default:                  return INVERSE_DISTANCE_CLAMPED;
	}
}

const char* SoundMngr::vendor()
{ return (const char*)alGetString(AL_VENDOR); }
const char* SoundMngr::version()
{ return (const char*)alGetString(AL_VERSION); }
const char* SoundMngr::renderer()
{ return (const char*)alGetString(AL_RENDERER); }
const char* SoundMngr::extensions()
{ return (const char*)alGetString(AL_EXTENSIONS); }

void SoundMngr::print_memory_usage() const
{
	size_t size_bytes = 0;
	unsigned count = 0;
	for (auto&& p : _buffer_map) {
		count += 1;
		size_bytes += p.second->size_bytes();
	}
	const float MiB = 1024 * 1024;
	LOG_F(1, "%5.1f MiB in %3u sounds", size_bytes / MiB, count);
}

} // namespace al
