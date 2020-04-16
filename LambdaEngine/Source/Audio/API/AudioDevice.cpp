#include "Audio/API/IAudioDevice.h"

#include "Audio/FMOD/AudioDeviceFMOD.h"

namespace LambdaEngine
{
	IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc)
	{
		if (api == EAudioAPI::FMOD)
		{
			AudioDeviceFMOD* pDevice = DBG_NEW AudioDeviceFMOD();
			if (pDevice->Init(desc))
			{
				return pDevice;
			}
			else
			{
				return nullptr;
			}
		}
		else if (api == EAudioAPI::LAMBDA)
		{
			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}
}
