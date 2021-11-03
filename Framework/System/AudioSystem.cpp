#include "AudioSystem.h"

AudioSystem* AudioSystem::Instance()
{
	static AudioSystem instance;
	return &instance;
}

void AudioSystem::Initialize()
{
	FMOD_System_Create(&mSystem);
	FMOD_System_Init(mSystem, 32, FMOD_INIT_NORMAL, nullptr);
}

void AudioSystem::Release()
{
	FMOD_System_Close(mSystem);
	FMOD_System_Release(mSystem);
}

void AudioSystem::Update()
{
	FMOD_System_Update(mSystem);
}

void AudioSystem::Play(FMOD_SOUND* _sound, FMOD_CHANNEL** _channel)
{
	FMOD_System_PlaySound(mSystem, _sound, nullptr, false, _channel);
}

void AudioSystem::Stop(FMOD_CHANNEL * _channel)
{
	FMOD_Channel_Stop(_channel);
}

void AudioSystem::Pause(FMOD_CHANNEL * _channel)
{
	FMOD_Channel_SetPaused(_channel, true);
}

void AudioSystem::Resume(FMOD_CHANNEL * _channel)
{
	FMOD_Channel_SetPaused(_channel, false);
}

void AudioSystem::SetVolume(FMOD_CHANNEL * _channel, float _volume)
{
	FMOD_Channel_SetVolume(_channel, _volume);
}

void AudioSystem::CreateSound(FMOD_SOUND** _sound, const char* _path, bool _loop)
{
	if (_loop) {
		FMOD_System_CreateSound(mSystem, _path, FMOD_LOOP_NORMAL, 0, _sound);
	}
	else {
		FMOD_System_CreateSound(mSystem, _path, FMOD_DEFAULT, 0, _sound);
	}
}
