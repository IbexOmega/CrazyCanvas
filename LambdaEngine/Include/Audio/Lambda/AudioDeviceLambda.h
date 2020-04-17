#pragma once

#include "PortAudio.h"
#include "Audio/API/IAudioDevice.h"

namespace LambdaEngine
{
	class AudioDeviceLambda : public IAudioDevice
	{
		struct PortAudioData
		{
			float LeftPhase;
			float RightPhase;
		};
		
	public:
		AudioDeviceLambda();
		~AudioDeviceLambda();

		virtual bool Init(const AudioDeviceDesc& desc) override final;

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

	private:
		const char* m_pName;
	};
}