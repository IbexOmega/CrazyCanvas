#pragma once

#include "PortAudio.h"
#include "Audio/API/ISoundInstance3D.h"

namespace LambdaEngine
{
	class IAudioDevice;
	
	class SoundInstance3DLambda : public ISoundInstance3D
	{
	public:
		SoundInstance3DLambda(const IAudioDevice* pAudioDevice);
		~SoundInstance3DLambda();

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
		void LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer);
		
	private:
		/*
		* This routine will be called by the PortAudio engine when audio is needed.
		* It may called at interrupt level on some machines so don't do anything
		*  that could mess up the system like calling malloc() or free().
		*/ 
		static int32 PortAudioCallback(
			const void* pInputBuffer,
			void* pOutputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* pTimeInfo,
			PaStreamCallbackFlags statusFlags,
			void* pUserData);

	private:
		PaStream* m_pStream;
		
		int16* m_pWaveForm;
		uint32 m_SampleCount;
		uint32 m_CurrentSample;
		uint32 m_ChannelCount;
	};
}
