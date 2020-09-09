#include "Audio/API/IAudioDevice.h"

#include "Audio/FMOD/AudioDeviceFMOD.h"

namespace LambdaEngine
{
	IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc)
	{
		IAudioDevice* pDevice;

		if (api == EAudioAPI::FMOD)
		{
			pDevice = DBG_NEW AudioDeviceFMOD();
		}
		else
		{
			return nullptr;
		}

		if (pDevice->Init(&desc))
		{
			return pDevice;
		}
		else
		{
			return nullptr;
		}
	}
}
