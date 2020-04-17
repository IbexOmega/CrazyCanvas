#pragma once

#include "FMOD.h"
#include "Audio/API/ISoundInstance3D.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;
	class SoundEffect3DFMOD;

	typedef unsigned int FMOD_MODE;

	class SoundInstance3DFMOD : public ISoundInstance3D
	{
	public:
		SoundInstance3DFMOD(const IAudioDevice* pAudioDevice);
		~SoundInstance3DFMOD();

		virtual bool Init(const SoundInstance3DDesc& desc) override final;

		virtual void Play() override final;
		virtual void Pause() override final;
		virtual void Stop() override final;
		virtual void Toggle() override final;
		
		virtual void SetPosition(const glm::vec3& position) override final;
		virtual void SetVolume(float volume) override final;
		virtual void SetPitch(float pitch) override final;
		
		virtual const glm::vec3& GetPosition() override final;
		virtual float GetVolume() override final;
		virtual float GetPitch() override final;
		
	private:
		bool IsPlaying();
		void RecreateHandleIfNeeded();

	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;
		SoundEffect3DFMOD*		m_pSoundEffect;

		//FMOD
		FMOD_CHANNEL*		m_pChannel;
		FMOD_MODE			m_Mode;

		//Local
		const char*			m_pName;
		glm::vec3			m_Position;
		float				m_Volume;
		float				m_Pitch;
	};
}