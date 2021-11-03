#include "AudioSource.h"
#include "AudioSystem.h"

void AudioSource::Awake()
{
}

void AudioSource::Update(const float _deltaTime)
{
}

void AudioSource::FixedUpdate(const float _fixedDltaTime)
{
}

void AudioSource::Draw(const float _deltaTime)
{
}

void AudioSource::Destroy()
{
	FMOD_Sound_Release(mSound);
}

void AudioSource::CreateSound(std::string _fileName, bool _loop, bool _playOnAwake)
{
/*
	std::string message_a = "convert before message";
	std::wstring message_w;
	message_w.assign(message_a.begin(), message_a.end());
	wprintf(message_w.c_str());

	std::wstring message_w = L"convert before message";
	std::string message_a;
	message_a.assign(message_w.begin(), message_w.end());
	printf(message_a.c_str());
*/
	AudioSystem::Instance()->CreateSound(&mSound, _fileName.c_str(), _loop);

	if (_playOnAwake)
	{
		Play();
	}
}

void AudioSource::Play(float _volume)
{
	AudioSystem::Instance()->Play(mSound, &mChannel);
}

void AudioSource::Pause()
{
	AudioSystem::Instance()->Pause(mChannel);
}

void AudioSource::Resume()
{
	AudioSystem::Instance()->Pause(mChannel);
}

void AudioSource::SetVolume(float _volume)
{
	AudioSystem::Instance()->SetVolume(mChannel, mVolume);
}
