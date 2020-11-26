#include "Audio/FMOD/SoundInstance2DFMOD.h"
#include "Audio/FMOD/SoundEffect3DFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundInstance2DFMOD::SoundInstance2DFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice)),
		m_pSoundEffect(nullptr),
		m_pChannel(nullptr),
		m_Mode(FMOD_DEFAULT),
		m_Volume(1.0f),
		m_Pitch(1.0f)
	{
	}

	SoundInstance2DFMOD::~SoundInstance2DFMOD()
	{
	}

	bool SoundInstance2DFMOD::Init(const SoundInstance2DDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;

		if (pDesc->pSoundEffect == nullptr)
		{
			LOG_WARNING("[SoundInstance2DFMOD]: Init failed for %s, pSoundEffect can't be nullptr", m_pName);
			return false;
		}

		m_pSoundEffect = reinterpret_cast<SoundEffect3DFMOD*>(pDesc->pSoundEffect);

		m_Mode = FMOD_2D;

		if (pDesc->Flags & FSoundModeFlags::SOUND_MODE_LOOPING)
		{
			m_Mode |= FMOD_LOOP_NORMAL;
		}

		m_Volume = pDesc->Volume;

		return true;
	}

	void SoundInstance2DFMOD::Play()
	{
		RecreateHandleIfNeeded();

		FMOD_Channel_SetPaused(m_pChannel, 0);
	}

	void SoundInstance2DFMOD::Pause()
	{
		if (IsPlaying())
		{
			FMOD_Channel_SetPaused(m_pChannel, 1);
		}
	}

	void SoundInstance2DFMOD::Stop()
	{
		if (IsPlaying())
		{
			FMOD_Channel_Stop(m_pChannel);
		}
	}

	void SoundInstance2DFMOD::Toggle()
	{
		RecreateHandleIfNeeded();

		FMOD_BOOL paused = 0;
		FMOD_Channel_GetPaused(m_pChannel, &paused);
		FMOD_Channel_SetPaused(m_pChannel, (paused ^ 0x1));
	}

	void SoundInstance2DFMOD::SetVolume(float volume)
	{
		m_Volume = volume;

		if (IsPlaying())
		{
			if (FMOD_Channel_SetVolume(m_pChannel, volume) != FMOD_OK)
			{
				LOG_DEBUG("[SoundInstance2DFMOD]: Volume could not be set for %s", m_pName);
			}
		}
	}

	void SoundInstance2DFMOD::SetPitch(float pitch)
	{
		m_Pitch = pitch;

		if (IsPlaying())
		{
			if (FMOD_Channel_SetPitch(m_pChannel, pitch) != FMOD_OK)
			{
				LOG_DEBUG("[SoundInstance2DFMOD]: Pitch could not be set for %s", m_pName);
			}
		}
	}

	float SoundInstance2DFMOD::GetVolume() const
	{
		return m_Volume;
	}

	float SoundInstance2DFMOD::GetPitch() const
	{
		return m_Pitch;
	}

	bool SoundInstance2DFMOD::IsPlaying()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pChannel, &isPlaying);
		return isPlaying;
	}

	void SoundInstance2DFMOD::RecreateHandleIfNeeded()
	{
		if (!IsPlaying())
		{
			FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pSoundEffect->GetHandle(), nullptr, true, &m_pChannel);
			FMOD_Channel_SetMode(m_pChannel, m_Mode);

			FMOD_Channel_SetVolume(m_pChannel, m_Volume);
			FMOD_Channel_SetPitch(m_pChannel, m_Pitch);
		}
	}
}