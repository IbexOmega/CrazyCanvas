#pragma once

#include "AudioDevice.h"

namespace LambdaEngine
{
	class AudioDevice;

	class LAMBDA_API AudioSystem
	{
	public:
		DECL_STATIC_CLASS(AudioSystem);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static AudioDevice* GetDevice()
		{
			return s_pAudioDevice;
		}

	private:
		static AudioDevice* s_pAudioDevice;
	};
}
