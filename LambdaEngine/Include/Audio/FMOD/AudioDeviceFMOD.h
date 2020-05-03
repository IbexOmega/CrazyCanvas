#pragma once

#include "FMOD.h"
#include "Audio/API/IAudioDevice.h"

namespace LambdaEngine
{
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

		virtual void UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc) override final;

		virtual uint32				CreateAudioListener()									override final;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)		override final;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)	override final;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)		override final;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)		override final;

		virtual void SetMasterVolume(float volume) override final;

		virtual float GetMasterVolume() const override final;

	public:
		FMOD_SYSTEM* pSystem;

	private:
		const char*		m_pName;

		uint32			m_MaxNumAudioListeners	= 0;
		uint32			m_NumAudioListeners		= 0;

		FMOD_SOUND*		m_pMusicHandle			= nullptr;
		FMOD_CHANNEL*	m_pMusicChannel			= nullptr;
	};
}