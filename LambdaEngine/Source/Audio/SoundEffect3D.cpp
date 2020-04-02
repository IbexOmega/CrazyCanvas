#include "Audio/SoundEffect3D.h"
#include "Audio/AudioDevice.h"
#include "Audio/Audio.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundEffect3D::SoundEffect3D(const AudioDevice* pAudioDevice) :
		m_pAudioDevice(pAudioDevice),
		m_pHandle(nullptr)
	{
	}

	SoundEffect3D::~SoundEffect3D()
	{
		if (m_pHandle != nullptr)
		{
			FMOD_Sound_Release(m_pHandle);
			m_pHandle = nullptr;
		}
	}

	bool SoundEffect3D::Init(const SoundDesc& desc)
	{
		FMOD_MODE soundMode = FMOD_3D | FMOD_CREATESAMPLE | FMOD_OPENMEMORY;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		soundCreateInfo.length = desc.DataSize;

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, reinterpret_cast<const char*>(desc.pData), soundMode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_ERROR("[Sound]: Sound \"%s\" could not be initialized", desc.pName);
			return false;
		}

		return true;
	}

	void SoundEffect3D::PlayAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch)
	{
		FMOD_CHANNEL* pChannel = nullptr;
		FMOD_VECTOR fmodPosition = { position.x, position.y, position.z };
		FMOD_VECTOR fmodVelocity = { velocity.x, velocity.y, velocity.z };

		FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pHandle, nullptr, true, &pChannel);
		FMOD_Channel_Set3DAttributes(pChannel, &fmodPosition, &fmodVelocity);
		FMOD_Channel_SetVolume(pChannel, volume);
		FMOD_Channel_SetPitch(pChannel, pitch);
		FMOD_Channel_SetPaused(pChannel, 0);
	}
}