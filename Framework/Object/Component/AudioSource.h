#ifndef _AUDIO_SOURCE_H_
#define _AUDIO_SOURCE_H_

#include "Component.h"

class AudioSource : public Component
{
public:
	
	AudioSource() = default;
	virtual ~AudioSource() = default;
	
private:
	
	AudioSource(const AudioSource& _rhs) = delete;
	AudioSource& operator =(const AudioSource& _rhs) = delete;
	AudioSource(AudioSource&& _rhs) = delete;
	AudioSource& operator =(AudioSource&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

public:

	void Play(float _volume = 1.0f);
	void Pause();
	void Resume();
	void SetVolume(float _volume);

	void CreateSound(std::string _fileName, bool _loop = false, bool _playOnAwake = false);

private:

	FMOD_SOUND* mSound = nullptr;
	FMOD_CHANNEL* mChannel = nullptr;

	bool mLoop = false;
	float mVolume = 1.0f;
};

#endif