#include "Audio/AudioDevice.h"
#include "Audio/Audio.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS severity, const char* pFile, int line, const char* pFunc, const char* pMessage)
	{
		if (severity & FMOD_DEBUG_LEVEL_WARNING)
		{
			LOG_WARNING("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}
		else if (severity & FMOD_DEBUG_LEVEL_ERROR)
		{
			LOG_ERROR("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}
		else if (!(severity & FMOD_DEBUG_LEVEL_LOG))
		{
			D_LOG_INFO("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}

		return FMOD_OK;
	}

	AudioDevice::AudioDevice() : 
		pSystem(nullptr)
	{
		
	}

	AudioDevice::~AudioDevice()
	{
		if (pSystem != nullptr)
		{
			if (FMOD_System_Release(pSystem) != FMOD_OK)
			{
				LOG_ERROR("[AudioDevice]: FMOD System could not be released");
			}
			else
			{
				D_LOG_MESSAGE("[AudioDevice]: FMOD System released successfully");
			}
		}
		
	}

	bool AudioDevice::Init(const AudioDeviceDesc& desc)
	{
		if (desc.Debug)
		{
			FMOD_DEBUG_FLAGS debugLevel		= FMOD_DEBUG_LEVEL_LOG;
			FMOD_DEBUG_MODE debugMode		= FMOD_DEBUG_MODE_CALLBACK;

			if (FMOD_Debug_Initialize(debugLevel, debugMode, DebugCallback, nullptr) != FMOD_OK)
			{
				LOG_WARNING("[AudioDevice]: FMOD Debug Mode could not be initialized");
				return false;
			}
		}

		if (FMOD_System_Create(&pSystem) != FMOD_OK)
		{
			LOG_ERROR("[AudioDevice]: FMOD System could not be created");
			return false;
		}

		int numChannels				= 512;
		FMOD_INITFLAGS initFlags	= FMOD_INIT_NORMAL;

		if (FMOD_System_Init(pSystem, numChannels, initFlags, nullptr) != FMOD_OK)
		{
			LOG_ERROR("[AudioDevice]: FMOD System could not be created");
			return false;
		}

		D_LOG_MESSAGE("[AudioDevice]: Successfully initialized!");

		return true;
	}

	void AudioDevice::Tick()
	{
		FMOD_System_Update(pSystem);
	}

	Sound* AudioDevice::CreateSound()
	{
		return new Sound(this);
	}

	
};