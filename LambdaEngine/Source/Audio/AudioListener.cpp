#include "Audio/AudioListener.h"
#include "Audio/AudioDevice.h"

#include "Audio/Audio.h"

namespace LambdaEngine
{
	AudioListener::AudioListener(const AudioDevice* pAudioDevice) : 
		m_pAudioDevice(pAudioDevice)
	{
	}

	AudioListener::~AudioListener()
	{
	}

	bool AudioListener::Init(const AudioListenerDesc& desc)
	{
		m_ListenerIndex = desc.ListenerIndex;

		return true;
	}

	void AudioListener::Update(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
	{
		FMOD_VECTOR fmodPosition	= { position.x,		position.y,		position.z };
		FMOD_VECTOR fmodVelocity	= { 0.0f,			0.0f,			0.0f };
		FMOD_VECTOR fmodForward		= { forward.x,		forward.y,		forward.z };
		FMOD_VECTOR fmodUp			= { up.x,			up.y,			up.z };

		FMOD_System_Set3DListenerAttributes(m_pAudioDevice->pSystem, m_ListenerIndex, &fmodPosition, &fmodVelocity, &fmodForward, &fmodUp);
	}
}