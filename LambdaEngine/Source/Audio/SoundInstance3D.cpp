#include "Audio/SoundInstance3D.h"
#include "Audio/SoundEffect3D.h"
#include "Audio/AudioDevice.h"
#include "Audio/SoundHelper.h"
#include "Audio/Audio.h"

#include "Log/Log.h"

#include <functional>

namespace LambdaEngine
{
	SoundInstance3D::SoundInstance3D(const AudioDevice* pAudioDevice) :
		m_pAudioDevice(pAudioDevice),
		m_pSoundEffect(nullptr),
		m_pChannel(nullptr),
		m_Mode(FMOD_DEFAULT),
		m_Position(0.0f),
		m_Volume(1.0f),
		m_Pitch(1.0f)
	{
	}

	SoundInstance3D::~SoundInstance3D()
	{
	}

	bool SoundInstance3D::Init(const SoundInstance3DDesc& desc)
	{
		m_pName = desc.pName;

		if (desc.pSoundEffect == nullptr)
		{
			LOG_WARNING("[SoundInstance3D]: Init failed for %s, pSoundEffect can't be nullptr", m_pName);
			return false;
		}

		m_pSoundEffect	= desc.pSoundEffect;
		
		m_Mode = FMOD_3D;

		if (desc.Flags & ESoundModeFlags::SOUND_MODE_LOOPING)
		{
			m_Mode |= FMOD_LOOP_NORMAL;
		}

		return true;
	}

	void SoundInstance3D::Play()
	{
		RecreateHandleIfNeeded();
		
		FMOD_Channel_SetPaused(m_pChannel, 0);
	}

	void SoundInstance3D::Pause()
	{
		if (IsPlaying())
		{
			FMOD_Channel_SetPaused(m_pChannel, 1);
		}
	}

	void SoundInstance3D::Stop()
	{
		if (IsPlaying())
		{
			FMOD_Channel_Stop(m_pChannel);
		}
	}

	void SoundInstance3D::Toggle()
	{
		RecreateHandleIfNeeded();

		FMOD_BOOL paused = 0;
		FMOD_Channel_GetPaused(m_pChannel, &paused);
		FMOD_Channel_SetPaused(m_pChannel, (paused ^ 0x1));
	}

	void SoundInstance3D::SetPosition(const glm::vec3& position)
	{
		m_Position = position;

		if (IsPlaying())
		{
			FMOD_VECTOR fmodPosition = { position.x, position.y, position.z };

			if (FMOD_Channel_Set3DAttributes(m_pChannel, &fmodPosition, nullptr) != FMOD_OK)
			{
				D_LOG_WARNING("[SoundInstance3D]: 3D Attributes could not be set for %s", m_pName);
			}
		}
	}

	void SoundInstance3D::SetVolume(float volume)
	{
		m_Volume = volume;

		if (IsPlaying())
		{
			if (FMOD_Channel_SetVolume(m_pChannel, volume) != FMOD_OK)
			{
				D_LOG_WARNING("[SoundInstance3D]: Volume could not be set for %s", m_pName);
			}
		}
	}

	void SoundInstance3D::SetPitch(float pitch)
	{
		m_Pitch = pitch;

		if (IsPlaying())
		{
			if (FMOD_Channel_SetPitch(m_pChannel, pitch) != FMOD_OK)
			{
				D_LOG_WARNING("[SoundInstance3D]: Pitch could not be set for %s", m_pName);
			}
		}
	}

	const glm::vec3& SoundInstance3D::GetPosition()
	{
		return m_Position;
	}

	float SoundInstance3D::GetVolume()
	{
		return m_Volume;
	}

	float SoundInstance3D::GetPitch()
	{
		return m_Pitch;
	}

	bool SoundInstance3D::IsPlaying()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pChannel, &isPlaying);
		return isPlaying;
	}

	void SoundInstance3D::RecreateHandleIfNeeded()
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