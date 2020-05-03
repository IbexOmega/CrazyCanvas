#pragma once

#include "FMOD.h"
#include "Audio/API/IMusic.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class MusicFMOD : public IMusic
	{
	public:
		MusicFMOD(const IAudioDevice* pAudioDevice);
		~MusicFMOD();

		virtual bool Init(const MusicDesc *pDesc) override final;

		virtual void Play() override final;
		virtual void Pause() override final;
		virtual void Toggle() override final;
		
		virtual void SetVolume(float volume)	override final;
		virtual void SetPitch(float pitch)		override final;
		
		FORCEINLINE virtual float GetVolume()	const override final { return m_Volume; }
		FORCEINLINE virtual float GetPitch()	const override final { return m_Pitch; }

		FORCEINLINE FMOD_SOUND* GetHandle() { return m_pHandle; }
		FORCEINLINE FMOD_CHANNEL* GetChannel() { return m_pChannel; }

	private:
		//Engine
		const AudioDeviceFMOD* m_pAudioDevice	= nullptr;

		//FMOD
		FMOD_SOUND*		m_pHandle				= nullptr;
		FMOD_CHANNEL*	m_pChannel				= nullptr;

		//Local
		const char*		m_pName					= "";
		float			m_Volume				= 1.0f;
		float			m_Pitch					= 1.0f;
	};
}
