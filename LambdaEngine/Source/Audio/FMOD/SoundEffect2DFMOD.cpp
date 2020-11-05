#include "Audio/FMOD/SoundEffect2DFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundEffect2DFMOD::SoundEffect2DFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice)),
		m_pHandle(nullptr)
	{
	}

	SoundEffect2DFMOD::~SoundEffect2DFMOD()
	{
		if (m_pHandle != nullptr)
		{
			FMOD_Sound_Release(m_pHandle);
			m_pHandle = nullptr;
		}
	}

	bool SoundEffect2DFMOD::Init(const SoundEffect2DDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_Name = pDesc->Filepath;

		FMOD_MODE mode = FMOD_2D | FMOD_CREATESAMPLE | FMOD_LOOP_OFF;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, pDesc->Filepath.c_str(), mode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_ERROR("[Sound]: Sound \"%s\" could not be initialized", m_Name.c_str());
			return false;
		}

		FMOD_Sound_GetLength(m_pHandle, &m_LengthMS, FMOD_TIMEUNIT_MS);
		return true;
	}

	void SoundEffect2DFMOD::PlayOnce(float volume, float pitch)
	{
		FMOD_CHANNEL* pChannel = nullptr;

		FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pHandle, nullptr, true, &pChannel);

		FMOD_Channel_SetVolume(pChannel, volume);
		FMOD_Channel_SetPitch(pChannel, pitch);

		FMOD_Channel_SetPaused(pChannel, 0);
	}

	FMOD_SOUND* SoundEffect2DFMOD::GetHandle()
	{
		return m_pHandle;
	}

	uint32 SoundEffect2DFMOD::GetLengthMS()
	{
		return m_LengthMS;
	}
}
