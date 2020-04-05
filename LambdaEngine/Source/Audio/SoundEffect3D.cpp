#include "Audio/SoundEffect3D.h"
#include "Audio/AudioDevice.h"
#include "Audio/SoundHelper.h"
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

	bool SoundEffect3D::Init(const SoundEffect3DDesc& desc)
	{
		m_pName = desc.pFilepath;

		FMOD_MODE mode = FMOD_3D | FMOD_CREATESAMPLE | FMOD_LOOP_OFF;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, desc.pFilepath, mode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_ERROR("[Sound]: Sound \"%s\" could not be initialized", m_pName);
			return false;
		}

		FMOD_Sound_GetLength(m_pHandle, &m_LengthMS, FMOD_TIMEUNIT_MS);
		return true;
	}

	void SoundEffect3D::PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch)
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

	FMOD_SOUND* SoundEffect3D::GetHandle() 
	{ 
		return m_pHandle; 
	}

	uint32 SoundEffect3D::GetLengthMS() 
	{ 
		return m_LengthMS; 
	}
}
