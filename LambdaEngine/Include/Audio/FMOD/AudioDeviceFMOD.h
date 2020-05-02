#pragma once

#include "FMOD.h"
#include "Audio/API/IAudioDevice.h"

namespace LambdaEngine
{
	class AudioListenerFMOD;
	class SoundEffect3DFMOD;
	class SoundInstance3DFMOD;
	class AudioGeometryFMOD;
	class ReverbSphereFMOD;

	class AudioDeviceFMOD : public IAudioDevice
	{
	public:
		AudioDeviceFMOD();
		~AudioDeviceFMOD();

		virtual bool Init(const AudioDeviceDesc* pDesc) override final;

		virtual void Tick() override final;

		virtual bool LoadMusic(const char* pFilepath) override final;
		virtual void PlayMusic() override final;
		virtual void PauseMusic() override final;
		virtual void ToggleMusic() override final;


		virtual IAudioListener*		CreateAudioListener()		override final;
		virtual ISoundEffect3D*		CreateSoundEffect()			const override final;
		virtual ISoundInstance3D*	CreateSoundInstance()		const override final;
		virtual IAudioGeometry*		CreateAudioGeometry()		const override final;
		virtual IReverbSphere*		CreateReverbSphere()		const override final;

	public:
		FMOD_SYSTEM* pSystem;

	private:
		const char*		m_pName;

		uint32			m_MaxNumAudioListeners;
		uint32			m_NumAudioListeners;

		FMOD_SOUND*		m_pMusicHandle;
		FMOD_CHANNEL*	m_pMusicChannel;
	};
}