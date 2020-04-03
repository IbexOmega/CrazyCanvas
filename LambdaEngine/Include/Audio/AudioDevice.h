#pragma once

#include "LambdaEngine.h"

class FMOD_SYSTEM;

namespace LambdaEngine
{
	class AudioListener;
	class SoundEffect3D;
	class SoundInstance3D;

	struct AudioDeviceDesc
	{
		bool Debug					= true;
		uint32 MaxNumAudioListeners	= 1;
	};

	class LAMBDA_API AudioDevice
	{
	public:
		DECL_REMOVE_COPY(AudioDevice);
		DECL_REMOVE_MOVE(AudioDevice);

		AudioDevice();
		~AudioDevice();

		bool Init(const AudioDeviceDesc& desc);

		void Tick();

		AudioListener*		CreateAudioListener();
		SoundEffect3D*		CreateSound();
		SoundInstance3D*	CreateSoundInstance();

	public:
		FMOD_SYSTEM* pSystem;

	private:
		uint32 m_MaxNumAudioListeners;
		uint32 m_NumAudioListeners;
	};
}