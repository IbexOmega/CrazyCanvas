#include "Audio/API/IAudioDevice.h"

#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

namespace LambdaEngine
{
	IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc)
	{
		IAudioDevice* pDevice;
		
		if (api == EAudioAPI::FMOD)
		{
			pDevice = DBG_NEW AudioDeviceFMOD();
		}
		else if (api == EAudioAPI::LAMBDA)
		{
			pDevice = DBG_NEW AudioDeviceLambda();	
		}
		else
		{
			return nullptr;
		}

		if (pDevice->Init(desc))
		{
			return pDevice;
		}
		else
		{
			return nullptr;
		}
	}
}
