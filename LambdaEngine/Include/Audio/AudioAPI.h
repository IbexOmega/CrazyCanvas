#pragma once
#include "LambdaEngine.h"
#include "Audio/API/IAudioDevice.h"

namespace LambdaEngine
{
	class LAMBDA_API AudioAPI
	{
	public:
		DECL_STATIC_CLASS(AudioAPI);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static IAudioDevice* GetDevice()
		{
			return s_pAudioDevice;
		}

	private:
		static IAudioDevice* s_pAudioDevice;
	};
}
