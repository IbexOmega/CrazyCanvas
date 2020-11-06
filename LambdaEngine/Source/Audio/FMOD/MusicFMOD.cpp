#include "Audio/FMOD/MusicFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	MusicFMOD::MusicFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice))
	{
	}

	MusicFMOD::~MusicFMOD()
	{
		FMOD_Channel_Stop(m_pChannel);
		FMOD_Sound_Release(m_pHandle);
		m_pHandle	= nullptr;
		m_pChannel	= nullptr;
	}

	bool MusicFMOD::Init(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_Name = pDesc->Filepath;

		FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

		if (FMOD_System_CreateSound(m_pAudioDevice->pSystem, pDesc->Filepath.c_str(), mode, &soundCreateInfo, &m_pHandle) != FMOD_OK)
		{
			LOG_WARNING("[MusicFMOD]: Music \"%s\" could not be initialized", pDesc->Filepath.c_str());
			return false;
		}

		if (FMOD_System_PlaySound(m_pAudioDevice->pSystem, m_pHandle, nullptr, false, &m_pChannel) != FMOD_OK)
		{
			LOG_WARNING("[MusicFMOD]: Music \"%s\" could not be played", pDesc->Filepath.c_str());
			return false;
		}

		return true;
	}

	void MusicFMOD::Play()
	{
		FMOD_Channel_SetPaused(m_pChannel, 0);
	}

	void MusicFMOD::Pause()
	{
		FMOD_Channel_SetPaused(m_pChannel, 1);
	}

	void MusicFMOD::Toggle()
	{
		FMOD_BOOL paused = 0;
		FMOD_Channel_GetPaused(m_pChannel, &paused);
		FMOD_Channel_SetPaused(m_pChannel, (paused ^ 0x1));
	}

	void MusicFMOD::SetVolume(float volume)
	{
		m_Volume = volume;

		if (FMOD_Channel_SetVolume(m_pChannel, volume) != FMOD_OK)
		{
			D_LOG_WARNING("[MusicFMOD]: Volume could not be set for %s", m_Name.c_str());
		}
	}

	void MusicFMOD::SetPitch(float pitch)
	{
		m_Pitch = pitch;

		if (FMOD_Channel_SetPitch(m_pChannel, pitch) != FMOD_OK)
		{
			D_LOG_WARNING("[MusicFMOD]: Pitch could not be set for %s", m_Name.c_str());
		}
	}
}