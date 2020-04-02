#include "Audio/Sound.h"
#include "Audio/AudioDevice.h"
#include "Audio/Audio.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	Sound::Sound(const AudioDevice* pAudioDevice) :
		m_pAudioDevice(pAudioDevice),
		m_pHandle(nullptr),
		m_pChannel(nullptr),
		m_Panning(0.0f)
	{
	}

	Sound::~Sound()
	{
		if (m_pHandle != nullptr)
		{
			FMOD_Sound_Release(m_pHandle);
			m_pHandle = nullptr;
		}
	}

	bool Sound::Init(const SoundDesc& desc)
	{
		/*
			FMOD_2D - Force 3D OFF
			FMOD_CREATESAMPLE - Decompress at loadtime
			FMOD_OPENMEMORY - Interpret "name_or_data" as data
		*/
		FMOD_MODE soundMode = FMOD_2D | FMOD_CREATESAMPLE | FMOD_OPENMEMORY;

		if (desc.Flags & ESoundFlags::LOOPING)
		{
			soundMode |= FMOD_LOOP_NORMAL;
		}

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		soundCreateInfo.length = desc.DataSize;

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, reinterpret_cast<const char*>(desc.pData), soundMode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_ERROR("[Sound]: Sound \"%s\" could not be initialized", desc.pName);
			return false;
		}

		if (FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pHandle, nullptr, true, &m_pChannel) != FMOD_OK)
		{
			LOG_ERROR("[Sound]: Sound \"%s\" could not be played", desc.pName);
			return false;
		}

		return true;
	}

	void Sound::Toggle()
	{
		FMOD_BOOL paused = 0;
		FMOD_Channel_GetPaused(m_pChannel, &paused);
		FMOD_Channel_SetPaused(m_pChannel, (paused ^ 0x1));
	}

	void Sound::SetPanning(float panning)
	{
		m_Panning = panning;
		FMOD_Channel_SetPan(m_pChannel, glm::clamp(panning, -1.0f, 1.0f));
	}

	void Sound::SetVolume(float volume)
	{
		FMOD_Channel_SetVolume(m_pChannel, glm::clamp(volume, 0.0f, 2.0f));
	}

	void Sound::SetPitch(float pitch)
	{
		FMOD_Channel_SetPitch(m_pChannel, glm::max(pitch, 0.0f));
	}

	float Sound::GetPanning()
	{
		return m_Panning;
	}

	float Sound::GetVolume()
	{
		float volume = 0.0f;
		FMOD_Channel_GetVolume(m_pChannel, &volume);
		return volume;
	}

	float Sound::GetPitch()
	{
		float pitch = 0.0f;
		FMOD_Channel_GetPitch(m_pChannel, &pitch);
		return pitch;
	}
}