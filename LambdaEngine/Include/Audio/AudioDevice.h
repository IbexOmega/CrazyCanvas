#pragma once

#include "LambdaEngine.h"

#include "Sound.h"

class FMOD_SYSTEM;

namespace LambdaEngine
{
	struct AudioDeviceDesc
	{
		bool Debug = true;
	};

	class AudioDevice
	{
	public:
		DECL_REMOVE_COPY(AudioDevice);
		DECL_REMOVE_MOVE(AudioDevice);

		AudioDevice();
		~AudioDevice();

		bool Init(const AudioDeviceDesc& desc);

		void Tick();

		Sound* CreateSound();

	public:
		FMOD_SYSTEM* pSystem;
	};
}