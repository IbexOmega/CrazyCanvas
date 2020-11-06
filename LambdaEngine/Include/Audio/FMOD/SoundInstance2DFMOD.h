#pragma once

#include "FMOD.h"
#include "Audio/API/ISoundInstance2D.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;
	class SoundEffect2DFMOD;

	typedef unsigned int FMOD_MODE;

	class SoundInstance2DFMOD : public ISoundInstance2D
	{
	public:
		SoundInstance2DFMOD(const IAudioDevice* pAudioDevice);
		~SoundInstance2DFMOD();

		virtual bool Init(const SoundInstance2DDesc* pDesc) override final;

		virtual void Play() override final;
		virtual void Pause() override final;
		virtual void Stop() override final;
		virtual void Toggle() override final;

		virtual void SetVolume(float volume) override final;
		virtual void SetPitch(float pitch) override final;

		virtual float GetVolume()				const override final;
		virtual float GetPitch()				const override final;

	private:
		bool IsPlaying();
		void RecreateHandleIfNeeded();

	private:
		//Engine
		const AudioDeviceFMOD* m_pAudioDevice;
		SoundEffect3DFMOD* m_pSoundEffect;

		//FMOD
		FMOD_CHANNEL* m_pChannel;
		FMOD_MODE			m_Mode;

		//Local
		const char* m_pName;
		float				m_Volume;
		float				m_Pitch;
	};
}