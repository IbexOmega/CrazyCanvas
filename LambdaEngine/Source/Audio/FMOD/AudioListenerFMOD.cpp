#include "Audio/FMOD/AudioListenerFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	AudioListenerFMOD::AudioListenerFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice))
	{
	}

	AudioListenerFMOD::~AudioListenerFMOD()
	{
	}

	bool AudioListenerFMOD::Init(const AudioListenerDesc& desc)
	{
		m_pName = desc.pName;
		m_ListenerIndex = desc.ListenerIndex;

		D_LOG_MESSAGE("[AudioListenerFMOD]: AudioListenerFMOD %s successfully initialized!", m_pName);

		return true;
	}

	void AudioListenerFMOD::Update(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
	{
		FMOD_VECTOR fmodPosition	= { position.x,		position.y,		position.z };
		FMOD_VECTOR fmodVelocity	= { 0.0f,			0.0f,			0.0f };
		FMOD_VECTOR fmodForward		= { forward.x,		forward.y,		forward.z };
		FMOD_VECTOR fmodUp			= { up.x,			up.y,			up.z };

		FMOD_System_Set3DListenerAttributes(m_pAudioDevice->pSystem, m_ListenerIndex, &fmodPosition, &fmodVelocity, &fmodForward, &fmodUp);
	}
}