#include "Audio/AudioAPI.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	IAudioDevice* AudioAPI::s_pAudioDevice = nullptr;

	bool AudioAPI::Init()
	{

		AudioDeviceDesc audioDeviceDesc = {};
		audioDeviceDesc.pName					= "Main AudioDeviceFMOD";
		audioDeviceDesc.Debug					= true;
		audioDeviceDesc.MaxNumAudioListeners	= 1;
		audioDeviceDesc.MaxWorldSize			= 200;

		s_pAudioDevice = CreateAudioDevice(EAudioAPI::FMOD, audioDeviceDesc);

		return true;
	}

	bool AudioAPI::Release()
	{
		SAFEDELETE(s_pAudioDevice)
		return true;
	}

	void AudioAPI::Tick()
	{
		s_pAudioDevice->Tick();
	}
}
