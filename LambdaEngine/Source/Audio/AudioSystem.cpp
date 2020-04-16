#include "Audio/AudioSystem.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	IAudioDevice*	AudioSystem::s_pAudioDevice = nullptr;

	bool AudioSystem::Init()
	{

		AudioDeviceDesc audioDeviceDesc = {};
		audioDeviceDesc.pName					= "Main AudioDeviceFMOD";
		audioDeviceDesc.Debug					= true;
		audioDeviceDesc.MaxNumAudioListeners	= 1;
		audioDeviceDesc.MaxWorldSize			= 200;

		s_pAudioDevice = CreateAudioDevice(EAudioAPI::FMOD, audioDeviceDesc);

		return true;
	}

	bool AudioSystem::Release()
	{
		SAFEDELETE(s_pAudioDevice)
		return true;
	}

	void AudioSystem::Tick()
	{
		s_pAudioDevice->Tick();
	}
}
