#pragma once

#include <memory>

namespace emilib {

// Stream .mp3 files on iOS and OSX.
class Music
{
public:
	explicit Music(const char* path);
	~Music();
	Music(Music&&);
	Music& operator=(Music&&);
	Music(const Music&) = delete;
	Music& operator=(const Music&) = delete;

	const std::string& path() const;

	void update(float dt); // For fades

	void fade_in(float duration = 0.1f);
	void fade_out_and_pause(float duration = 0.1f);

	void play();
	void pause();
	void stop();

	void set_volume(float volume);
	void set_muted(bool muted);

private:
	struct Fade
	{
		float target; // 0 or _volume
		float speed;
	};

	struct MusicImpl;

	std::unique_ptr<MusicImpl> _impl;
	float                      _volume = 1;
	bool                       _has_fade = false;
	Fade                       _fade;
};

} // namespace emilib
