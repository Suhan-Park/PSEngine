#ifndef _AUDIO_SYSTEM_H_
#define _AUDIO_SYSTEM_H_

class AudioSystem final
{
private:
	
	AudioSystem() = default;
	~AudioSystem() = default;

	AudioSystem(const AudioSystem &_rhs) = delete;
	AudioSystem& operator = (const AudioSystem &_rhs) = delete;
	AudioSystem(AudioSystem &&_rhs) = delete;
	AudioSystem& operator = (AudioSystem &&_rhs) = delete;

public:

	static AudioSystem* Instance();

public:

	void Initialize();
	void Release();
	void Update();

public:

	void Play(FMOD_SOUND* _sound, FMOD_CHANNEL** _channel);
	void Stop(FMOD_CHANNEL* _channel);
	void Pause(FMOD_CHANNEL* _channel);
	void Resume(FMOD_CHANNEL* _channel);
	void SetVolume(FMOD_CHANNEL* _channel, float _volume);

	void CreateSound(FMOD_SOUND** _sound, const char* _path, bool _loop);

private:

	FMOD_SYSTEM* mSystem = nullptr;
};

#endif

