// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
#include "music.hpp"

#include <string>

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

#include <loguru.hpp>

namespace emilib {

struct Music::MusicImpl
{
	const std::string _path;
	AVAudioPlayer*    _player = nil;

	MusicImpl(const char* path) : _path(path)
	{
		NSString *ns_path = [NSString stringWithUTF8String: path];
		NSURL *fileURL = [NSURL fileURLWithPath:ns_path];

		_player = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:nil];
		LOG_F(INFO, "Playing %s", path);
		_player.numberOfLoops = -1; //infinite
		[_player prepareToPlay];
	}

	~MusicImpl()
	{
		[_player stop];
	}

	void play()  { [ _player play  ]; }
	void pause() { [ _player pause ]; }
	void stop()  { [ _player stop  ]; }

	float volume() const { return _player.volume; }

	void set_volume(float volume)
	{
		_player.volume = volume;
	}
};

Music::Music(const char* path)
{
	_impl = std::make_unique<MusicImpl>(path);
}

Music::~Music() = default;
Music::Music(Music&&) = default;
Music& Music::operator=(Music&&) = default;

const std::string& Music::path() const { return _impl->_path; }

void Music::update(float dt)
{
	if (_has_fade) {
		float volume = _impl->volume();

		if (0 < _fade.target) {
			//VLOG_F(1"Fading in, volume: %4.2f", volume);
			volume += _fade.speed * dt;
			if (volume < _fade.target) {
				_impl->set_volume(volume);
			} else {
				_impl->set_volume(_fade.target);
				_has_fade = false;
			}
		} else {
			//VLOG_F(1"Fading out, volume: %4.2f", volume);
			volume -= _fade.speed * dt;
			if (0 < volume) {
				_impl->set_volume(volume);
			} else {
				_has_fade = false;
				pause();
				_impl->set_volume(_volume);
			}
		}
	}
}

void Music::fade_in(float duration)
{
	CHECK_F(_volume > 0 && duration > 0);
	_fade = Fade{ _volume, _volume/duration };
	_has_fade = true;
	_impl->set_volume(0);
	play();
}

void Music::fade_out_and_pause(float duration)
{
	CHECK_F(_volume > 0 && duration > 0);
	_fade = Fade{ 0, _volume/duration };
	_has_fade = true;
}

void Music::play()  { _impl->play();  }
void Music::pause() { _impl->pause(); }
void Music::stop()  { _impl->stop();  }
void Music::set_volume(float volume) { _volume = volume; _impl->set_volume(volume); }
void Music::set_muted(bool muted) { _impl->set_volume(muted ? 0 : _volume); }

} // namespace emilib
