#include "Audio/FMOD/SoundInstance3DFMOD.h"
#include "Audio/FMOD/SoundEffect3DFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundInstance3DFMOD::SoundInstance3DFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice)),
		m_pSoundEffect(nullptr),
		m_pChannel(nullptr),
		m_Mode(FMOD_DEFAULT),
		m_Position(0.0f),
		m_Volume(1.0f),
		m_Pitch(1.0f)
	{
	}

	SoundInstance3DFMOD::~SoundInstance3DFMOD()
	{
	}

	bool SoundInstance3DFMOD::Init(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;

		if (pDesc->pSoundEffect == nullptr)
		{
			LOG_WARNING("[SoundInstance3DFMOD]: Init failed for %s, pSoundEffect can't be nullptr", m_pName);
			return false;
		}

		m_pSoundEffect	= reinterpret_cast<SoundEffect3DFMOD*>(pDesc->pSoundEffect);
		
		m_Mode = FMOD_3D;

		if (pDesc->Flags & FSoundModeFlags::SOUND_MODE_LOOPING)
		{
			m_Mode |= FMOD_LOOP_NORMAL;
		}

		m_Position = pDesc->Position;
		m_Volume = pDesc->Volume;

		return true;
	}

	void SoundInstance3DFMOD::Play()
	{
		RecreateHandleIfNeeded();
		
		FMOD_Channel_SetPaused(m_pChannel, 0);
	}

	void SoundInstance3DFMOD::Pause()
	{
		if (IsPlaying())
		{
			FMOD_Channel_SetPaused(m_pChannel, 1);
		}
	}

	void SoundInstance3DFMOD::Stop()
	{
		if (IsPlaying())
		{
			FMOD_Channel_Stop(m_pChannel);
		}
	}

	void SoundInstance3DFMOD::Toggle()
	{
		RecreateHandleIfNeeded();

		FMOD_BOOL paused = 0;
		FMOD_Channel_GetPaused(m_pChannel, &paused);
		FMOD_Channel_SetPaused(m_pChannel, (paused ^ 0x1));
	}

	void SoundInstance3DFMOD::SetPosition(const glm::vec3& position)
	{
		m_Position = position;

		if (IsPlaying())
		{
			FMOD_VECTOR fmodPosition = { position.x, position.y, position.z };

			if (FMOD_Channel_Set3DAttributes(m_pChannel, &fmodPosition, nullptr) != FMOD_OK)
			{
				D_LOG_WARNING("[SoundInstance3DFMOD]: 3D Attributes could not be set for %s", m_pName);
			}
		}
	}

	void SoundInstance3DFMOD::SetVolume(float volume)
	{
		m_Volume = volume;

		if (IsPlaying())
		{
			if (FMOD_Channel_SetVolume(m_pChannel, volume) != FMOD_OK)
			{
				D_LOG_WARNING("[SoundInstance3DFMOD]: Volume could not be set for %s", m_pName);
			}
		}
	}

	void SoundInstance3DFMOD::SetPitch(float pitch)
	{
		m_Pitch = pitch;

		if (IsPlaying())
		{
			if (FMOD_Channel_SetPitch(m_pChannel, pitch) != FMOD_OK)
			{
				D_LOG_WARNING("[SoundInstance3DFMOD]: Pitch could not be set for %s", m_pName);
			}
		}
	}

	const glm::vec3& SoundInstance3DFMOD::GetPosition() const
	{
		return m_Position;
	}

	float SoundInstance3DFMOD::GetVolume() const
	{
		return m_Volume;
	}

	float SoundInstance3DFMOD::GetPitch() const
	{
		return m_Pitch;
	}

	bool SoundInstance3DFMOD::IsPlaying()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pChannel, &isPlaying);
		return isPlaying;
	}

	void SoundInstance3DFMOD::RecreateHandleIfNeeded()
	{
		if (!IsPlaying())
		{
			FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pSoundEffect->GetHandle(), nullptr, true, &m_pChannel);
			FMOD_Channel_SetMode(m_pChannel, m_Mode);

			FMOD_VECTOR fmodPosition = { m_Position.x, m_Position.y, m_Position.z };

			FMOD_Channel_Set3DAttributes(m_pChannel, &fmodPosition, nullptr);
			FMOD_Channel_SetVolume(m_pChannel, m_Volume);
			FMOD_Channel_SetPitch(m_pChannel, m_Pitch);
		}
	}
}